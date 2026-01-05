#include "World.hpp"
#include "MeshBuilder.hpp"
#include "Shader.hpp"
#include "FastNoiseLite.h"
#include "TaskScheduler.hpp"
#include <algorithm>
#include <fstream>

World::World() {}
World::~World() {}

void World::clear() {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_chunks.clear();
    m_meshResults.clear();
}

void World::save(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return;
    size_t count = m_chunks.size();
    out.write((char*)&count, sizeof(count));
    for (auto& [key, data] : m_chunks) {
        out.write((char*)&data->x, sizeof(int));
        out.write((char*)&data->z, sizeof(int));
        out.write((char*)data->chunk->getData(), Chunk::SizeX * Chunk::SizeY * Chunk::SizeZ);
    }
}

void World::load(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return;
    m_chunks.clear();
    size_t count;
    in.read((char*)&count, sizeof(count));
    for (size_t i = 0; i < count; ++i) {
        int x, z;
        in.read((char*)&x, sizeof(int));
        in.read((char*)&z, sizeof(int));
        auto data = std::make_unique<ChunkData>();
        data->x = x;
        data->z = z;
        data->chunk = std::make_unique<Chunk>();
        in.read((char*)data->chunk->getData(), Chunk::SizeX * Chunk::SizeY * Chunk::SizeZ);
        data->dirty = true;
        m_chunks[getChunkKey(x, z)] = std::move(data);
    }
}

void World::update(const Vec3& playerPos, FastNoiseLite& noise, int seed, float freq, int baseHeight, TaskScheduler* scheduler) {
    int px = (int)std::floor(playerPos.x / Chunk::SizeX);
    int pz = (int)std::floor(playerPos.z / Chunk::SizeZ);

    // 1. Load new chunks
    for (int z = pz - m_renderDistance; z <= pz + m_renderDistance; ++z) {
        for (int x = px - m_renderDistance; x <= px + m_renderDistance; ++x) {
            uint64_t key = getChunkKey(x, z);
            if (m_chunks.find(key) == m_chunks.end()) {
                auto data = std::make_unique<ChunkData>();
                data->x = x;
                data->z = z;
                data->chunk = std::make_unique<Chunk>();
                data->mesh = std::make_unique<GLMesh>();
                
                data->chunk->generateTerrain(noise, seed, freq, baseHeight, x * Chunk::SizeX, z * Chunk::SizeZ);
                data->dirty = true;
                m_chunks[key] = std::move(data);
            }
        }
    }

    // 2. Unload far chunks
    for (auto it = m_chunks.begin(); it != m_chunks.end();) {
        int dx = std::abs(it->second->x - px);
        int dz = std::abs(it->second->z - pz);
        if (dx > m_renderDistance + 1 || dz > m_renderDistance + 1) {
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }

    // 3. Collect mesh results from background threads
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        for (auto& res : m_meshResults) {
            auto it = m_chunks.find(res.key);
            if (it != m_chunks.end()) {
                it->second->mesh->upload(res.vertices);
                it->second->meshing = false;
                it->second->dirty = false;
            }
        }
        m_meshResults.clear();
    }

    // 4. Dispatch new meshing tasks
    if (scheduler) {
        int dispatched = 0;
        for (auto& pair : m_chunks) {
            if (pair.second->dirty && !pair.second->meshing) {
                pair.second->meshing = true;
                uint64_t key = pair.first;
                Chunk* chunkPtr = pair.second->chunk.get();
                
                scheduler->enqueue([this, key, chunkPtr]() {
                    std::vector<Vertex> verts = MeshBuilder::buildGreedyMesh(*chunkPtr);
                    std::lock_guard<std::mutex> lock(m_resultMutex);
                    m_meshResults.push_back({key, std::move(verts)});
                });

                if (++dispatched > 4) break; 
            }
        }
    } else {
        // Fallback to synchronous meshing
        int remeshed = 0;
        for (auto& pair : m_chunks) {
            if (pair.second->dirty) {
                std::vector<Vertex> verts = MeshBuilder::buildGreedyMesh(*pair.second->chunk);
                pair.second->mesh->upload(verts);
                pair.second->dirty = false;
                if (++remeshed > 2) break; 
            }
        }
    }
}

void World::render(const Shader& shader, const Mat4& viewProj) {
    int rendered = 0;
    for (auto& pair : m_chunks) {
        float offsetX = (float)(pair.second->x * Chunk::SizeX);
        float offsetZ = (float)(pair.second->z * Chunk::SizeZ);
        
        Mat4 model = translate({offsetX, 0.0f, offsetZ});
        shader.setMat4("uModel", model);
        pair.second->mesh->draw();
        rendered++;
    }
}

void World::setBlock(int x, int y, int z, uint8_t type) {
    int cx = (int)std::floor((float)x / Chunk::SizeX);
    int cz = (int)std::floor((float)z / Chunk::SizeZ);
    uint64_t key = getChunkKey(cx, cz);

    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        int lx = x - cx * Chunk::SizeX;
        int lz = z - cz * Chunk::SizeZ;
        it->second->chunk->set(lx, y, lz, type);
        it->second->dirty = true;
        
        // Mark neighbors dirty if on edge
        if (lx == 0) { auto n = m_chunks.find(getChunkKey(cx - 1, cz)); if (n != m_chunks.end()) n->second->dirty = true; }
        if (lx == Chunk::SizeX - 1) { auto n = m_chunks.find(getChunkKey(cx + 1, cz)); if (n != m_chunks.end()) n->second->dirty = true; }
        if (lz == 0) { auto n = m_chunks.find(getChunkKey(cx, cz - 1)); if (n != m_chunks.end()) n->second->dirty = true; }
        if (lz == Chunk::SizeZ - 1) { auto n = m_chunks.find(getChunkKey(cx, cz + 1)); if (n != m_chunks.end()) n->second->dirty = true; }
    }
}

uint8_t World::getBlock(int x, int y, int z) const {
    int cx = (int)std::floor((float)x / Chunk::SizeX);
    int cz = (int)std::floor((float)z / Chunk::SizeZ);
    uint64_t key = getChunkKey(cx, cz);

    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second->chunk->get(x - cx * Chunk::SizeX, y, z - cz * Chunk::SizeZ);
    }
    return 0;
}

bool World::isSolid(int x, int y, int z) const {
    uint8_t block = getBlock(x, y, z);
    // Treat fluids and foliage as non-solid for collision.
    // Solid means blocks that should stop player/mobs.
    return block != BLOCK_AIR &&
           block != BLOCK_WATER &&
           block != BLOCK_LAVA &&
           block != BLOCK_FLOWER_RED &&
           block != BLOCK_FLOWER_BLUE &&
           block != BLOCK_TALL_GRASS;
}

World::RaycastResult World::raycast(const Vec3& origin, const Vec3& direction, float maxDist) {
    RaycastResult res;
    Vec3 pos = origin;
    Vec3 step = direction * 0.1f;
    float dist = 0.0f;

    int lastX = (int)std::floor(pos.x);
    int lastY = (int)std::floor(pos.y);
    int lastZ = (int)std::floor(pos.z);

    while (dist < maxDist) {
        int ix = (int)std::floor(pos.x);
        int iy = (int)std::floor(pos.y);
        int iz = (int)std::floor(pos.z);

        uint8_t block = getBlock(ix, iy, iz);
        if (block != 0) {
            res.hit = true;
            res.x = ix;
            res.y = iy;
            res.z = iz;
            res.nx = lastX - ix;
            res.ny = lastY - iy;
            res.nz = lastZ - iz;
            return res;
        }

        lastX = ix; lastY = iy; lastZ = iz;
        pos = pos + step;
        dist += 0.1f;
    }
    return res;
}
