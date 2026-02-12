#include "World.hpp"
#include "MeshBuilder.hpp"
#include "Shader.hpp"
#include "FastNoiseLite.h"
#include "TaskScheduler.hpp"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <filesystem>

World::World() {}
World::~World() {}

void World::clear() {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_chunks.clear();
    m_meshResults.clear();
}

void World::save(const std::string& filename, const WorldMetadata& meta) {
    // Ensure directory exists
    std::filesystem::path filepath(filename);
    if (filepath.has_parent_path()) {
        std::filesystem::create_directories(filepath.parent_path());
    }

    std::ofstream out(filename, std::ios::binary);
    if (!out) return;

    // Magic header
    const char magic[4] = {'V', 'S', 'A', '1'};
    out.write(magic, 4);

    // Metadata
    out.write((const char*)&meta, sizeof(WorldMetadata));

    // Chunk count + data
    size_t count = m_chunks.size();
    out.write((const char*)&count, sizeof(count));
    for (auto& [key, data] : m_chunks) {
        out.write((const char*)&data->x, sizeof(int));
        out.write((const char*)&data->z, sizeof(int));
        out.write((const char*)data->chunk->getData(), Chunk::SizeX * Chunk::SizeY * Chunk::SizeZ);
    }
}

bool World::load(const std::string& filename, WorldMetadata& metaOut) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return false;

    // Check for magic header (new format vs old format)
    char magic[4] = {};
    in.read(magic, 4);

    bool newFormat = (magic[0] == 'V' && magic[1] == 'S' && magic[2] == 'A' && magic[3] == '1');

    m_chunks.clear();

    if (newFormat) {
        // New format: magic + metadata + chunks
        in.read((char*)&metaOut, sizeof(WorldMetadata));

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
            data->mesh = std::make_unique<GLMesh>();
            in.read((char*)data->chunk->getData(), Chunk::SizeX * Chunk::SizeY * Chunk::SizeZ);
            data->dirty = true;
            m_chunks[getChunkKey(x, z)] = std::move(data);
        }
    } else {
        // Old format: first 4 bytes were part of size_t count
        // Seek back and read as old format
        in.seekg(0, std::ios::beg);

        // Set default metadata
        std::strncpy(metaOut.name, "Legacy World", sizeof(metaOut.name) - 1);
        metaOut.name[sizeof(metaOut.name) - 1] = '\0';

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
            data->mesh = std::make_unique<GLMesh>();
            in.read((char*)data->chunk->getData(), Chunk::SizeX * Chunk::SizeY * Chunk::SizeZ);
            data->dirty = true;
            m_chunks[getChunkKey(x, z)] = std::move(data);
        }
    }

    return true;
}

std::vector<WorldSaveInfo> World::listSaves(const std::string& directory) {
    std::vector<WorldSaveInfo> saves;

    if (!std::filesystem::exists(directory)) return saves;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".vsa" && ext != ".dat") continue;

        WorldSaveInfo info;
        info.filename = entry.path().string();

        std::ifstream in(info.filename, std::ios::binary);
        if (!in) continue;

        char magic[4] = {};
        in.read(magic, 4);

        if (magic[0] == 'V' && magic[1] == 'S' && magic[2] == 'A' && magic[3] == '1') {
            in.read((char*)&info.metadata, sizeof(WorldMetadata));
            in.read((char*)&info.chunkCount, sizeof(size_t));
            info.displayName = info.metadata.name;
        } else {
            // Old format
            in.seekg(0, std::ios::beg);
            in.read((char*)&info.chunkCount, sizeof(size_t));
            info.displayName = entry.path().stem().string();
            std::strncpy(info.metadata.name, info.displayName.c_str(), sizeof(info.metadata.name) - 1);
            info.metadata.name[sizeof(info.metadata.name) - 1] = '\0';
        }

        saves.push_back(std::move(info));
    }

    // Sort by name
    std::sort(saves.begin(), saves.end(), [](const WorldSaveInfo& a, const WorldSaveInfo& b) {
        return a.displayName < b.displayName;
    });

    return saves;
}

void World::update(const Vec3& playerPos, FastNoiseLite& noise, int seed, float freq, int baseHeight, TaskScheduler* scheduler) {
    m_newlyGeneratedChunks.clear();
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
                
                m_newlyGeneratedChunks.push_back({x, z});
                
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

    // 4. Dispatch new meshing tasks (prioritize closest chunks)
    if (scheduler) {
        // Collect dirty chunks and sort by distance to player
        struct DirtyChunk { uint64_t key; Chunk* ptr; float distSq; };
        std::vector<DirtyChunk> dirtyChunks;
        for (auto& pair : m_chunks) {
            if (pair.second->dirty && !pair.second->meshing) {
                float dx = (float)pair.second->x - (float)px;
                float dz = (float)pair.second->z - (float)pz;
                dirtyChunks.push_back({pair.first, pair.second->chunk.get(), dx * dx + dz * dz});
            }
        }
        std::sort(dirtyChunks.begin(), dirtyChunks.end(),
            [](const DirtyChunk& a, const DirtyChunk& b) { return a.distSq < b.distSq; });

        int dispatched = 0;
        for (auto& dc : dirtyChunks) {
            auto it = m_chunks.find(dc.key);
            if (it == m_chunks.end()) continue;
            it->second->meshing = true;
            uint64_t key = dc.key;
            Chunk* chunkPtr = dc.ptr;

            scheduler->enqueue([this, key, chunkPtr]() {
                std::vector<Vertex> verts = MeshBuilder::buildGreedyMesh(*chunkPtr);
                std::lock_guard<std::mutex> lock(m_resultMutex);
                m_meshResults.push_back({key, std::move(verts)});
            });

            if (++dispatched > 6) break;
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

// ---------------------------------------------------------------------------
// Frustum culling helpers
// ---------------------------------------------------------------------------

struct FrustumPlane {
    float a, b, c, d;
};

struct Frustum {
    FrustumPlane planes[6]; // left, right, bottom, top, near, far
};

// Extract 6 frustum planes from a column-major view-projection matrix using
// the Griggs-Hartmann method. Planes point inward (positive half-space is
// inside the frustum). Each plane is normalized so that distance tests give
// world-space distances.
static Frustum extractFrustum(const Mat4& vp) {
    // For column-major storage, row i, column j = vp.m[j*4 + i].
    // Shorthand: r(i,j) = vp.m[j*4 + i]
    #define R(i, j) vp.m[(j)*4 + (i)]

    Frustum f;

    // Left:   row3 + row0
    f.planes[0] = { R(3,0) + R(0,0), R(3,1) + R(0,1), R(3,2) + R(0,2), R(3,3) + R(0,3) };
    // Right:  row3 - row0
    f.planes[1] = { R(3,0) - R(0,0), R(3,1) - R(0,1), R(3,2) - R(0,2), R(3,3) - R(0,3) };
    // Bottom: row3 + row1
    f.planes[2] = { R(3,0) + R(1,0), R(3,1) + R(1,1), R(3,2) + R(1,2), R(3,3) + R(1,3) };
    // Top:    row3 - row1
    f.planes[3] = { R(3,0) - R(1,0), R(3,1) - R(1,1), R(3,2) - R(1,2), R(3,3) - R(1,3) };
    // Near:   row3 + row2
    f.planes[4] = { R(3,0) + R(2,0), R(3,1) + R(2,1), R(3,2) + R(2,2), R(3,3) + R(2,3) };
    // Far:    row3 - row2
    f.planes[5] = { R(3,0) - R(2,0), R(3,1) - R(2,1), R(3,2) - R(2,2), R(3,3) - R(2,3) };

    #undef R

    // Normalize each plane so (a,b,c) is unit length.
    for (int i = 0; i < 6; ++i) {
        float len = std::sqrt(f.planes[i].a * f.planes[i].a +
                              f.planes[i].b * f.planes[i].b +
                              f.planes[i].c * f.planes[i].c);
        if (len > 0.0f) {
            float inv = 1.0f / len;
            f.planes[i].a *= inv;
            f.planes[i].b *= inv;
            f.planes[i].c *= inv;
            f.planes[i].d *= inv;
        }
    }

    return f;
}

// Test an axis-aligned bounding box against the frustum.
// Returns true if the AABB is at least partially inside (i.e. should be rendered).
// Uses the "p-vertex" technique: for each plane, pick the AABB corner most
// aligned with the plane normal. If that corner is behind the plane, the
// entire box is outside.
static bool isAABBInFrustum(const Frustum& frustum,
                            float minX, float minY, float minZ,
                            float maxX, float maxY, float maxZ) {
    for (int i = 0; i < 6; ++i) {
        const FrustumPlane& p = frustum.planes[i];

        // Select the p-vertex (the corner furthest in the direction of the normal).
        float px = (p.a >= 0.0f) ? maxX : minX;
        float py = (p.b >= 0.0f) ? maxY : minY;
        float pz = (p.c >= 0.0f) ? maxZ : minZ;

        // If the p-vertex is behind this plane, the AABB is fully outside.
        if (p.a * px + p.b * py + p.c * pz + p.d < 0.0f) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

void World::render(const Shader& shader, const Mat4& viewProj) {
    Frustum frustum = extractFrustum(viewProj);

    int rendered = 0;
    for (auto& pair : m_chunks) {
        float offsetX = (float)(pair.second->x * Chunk::SizeX);
        float offsetZ = (float)(pair.second->z * Chunk::SizeZ);

        // Chunk AABB in world space.
        float minX = offsetX;
        float minY = 0.0f;
        float minZ = offsetZ;
        float maxX = offsetX + (float)Chunk::SizeX;
        float maxY = (float)Chunk::SizeY;
        float maxZ = offsetZ + (float)Chunk::SizeZ;

        // Skip chunks entirely outside the view frustum.
        if (!isAABBInFrustum(frustum, minX, minY, minZ, maxX, maxY, maxZ))
            continue;

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
