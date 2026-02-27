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
    std::lock_guard<std::mutex> lockGen(m_genResultMutex);
    m_chunks.clear();
    m_meshResults.clear();
    m_genResults.clear();
    m_generatingChunks.clear();
    m_updateQueue.clear();
    m_scheduledSet.clear();
    m_fluidLevels.clear();
    m_fireAge.clear();
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

    clear();

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

    // 0. Collect generation results from background threads
    {
        std::lock_guard<std::mutex> lock(m_genResultMutex);
        for (auto& res : m_genResults) {
            uint64_t key = getChunkKey(res.x, res.z);
            auto data = std::make_unique<ChunkData>();
            data->x = res.x;
            data->z = res.z;
            data->chunk = std::move(res.chunk);
            data->mesh = std::make_unique<GLMesh>();
            data->dirty = true;
            
            // Schedule fluid updates
            for (uint32_t packed : data->chunk->m_fluidUpdates) {
                int lx = (packed >> 16) & 0xFF;
                int ly = (packed >> 8) & 0xFF;
                int lz = packed & 0xFF;
                int wx = res.x * Chunk::SizeX + lx;
                int wz = res.z * Chunk::SizeZ + lz;
                uint8_t b = data->chunk->get(lx, ly, lz);
                scheduleBlockUpdate(wx, ly, wz, (b == BLOCK_LAVA) ? LAVA_TICK_DELAY : WATER_TICK_DELAY);
            }
            data->chunk->m_fluidUpdates.clear();

            m_chunks[key] = std::move(data);
            m_generatingChunks.erase(key);
            m_newlyGeneratedChunks.push_back({res.x, res.z});
        }
        m_genResults.clear();
    }

    // 1. Load new chunks (dispatch to background threads)
    int chunksDispatchedThisFrame = 0;
    const int MAX_CHUNKS_DISPATCHED_PER_FRAME = 4;

    // Collect missing chunks and sort by distance to player
    struct MissingChunk { int x, z; float distSq; };
    std::vector<MissingChunk> missingChunks;

    for (int z = pz - m_renderDistance; z <= pz + m_renderDistance; ++z) {
        for (int x = px - m_renderDistance; x <= px + m_renderDistance; ++x) {
            uint64_t key = getChunkKey(x, z);
            if (m_chunks.find(key) == m_chunks.end() && m_generatingChunks.find(key) == m_generatingChunks.end()) {
                float dx = (float)x - (float)px;
                float dz = (float)z - (float)pz;
                missingChunks.push_back({x, z, dx * dx + dz * dz});
            }
        }
    }

    std::sort(missingChunks.begin(), missingChunks.end(),
        [](const MissingChunk& a, const MissingChunk& b) { return a.distSq < b.distSq; });

    for (const auto& mc : missingChunks) {
        if (chunksDispatchedThisFrame >= MAX_CHUNKS_DISPATCHED_PER_FRAME) {
            break; // Skip dispatching more chunks this frame to maintain FPS
        }

        uint64_t key = getChunkKey(mc.x, mc.z);
        m_generatingChunks.insert(key);
        
        if (scheduler) {
            int cx = mc.x;
            int cz = mc.z;
            // Copy noise to avoid race conditions
            FastNoiseLite noiseCopy = noise;
            scheduler->enqueue([this, cx, cz, noiseCopy, seed, freq, baseHeight]() mutable {
                auto chunk = std::make_unique<Chunk>();
                chunk->generateTerrain(noiseCopy, seed, freq, baseHeight, cx * Chunk::SizeX, cz * Chunk::SizeZ);
                
                std::lock_guard<std::mutex> lock(m_genResultMutex);
                m_genResults.push_back({cx, cz, std::move(chunk)});
            });
        } else {
            // Fallback to synchronous
            auto chunk = std::make_unique<Chunk>();
            chunk->generateTerrain(noise, seed, freq, baseHeight, mc.x * Chunk::SizeX, mc.z * Chunk::SizeZ);
            
            auto data = std::make_unique<ChunkData>();
            data->x = mc.x;
            data->z = mc.z;
            data->chunk = std::move(chunk);
            data->mesh = std::make_unique<GLMesh>();
            data->dirty = true;

            // Schedule fluid updates
            for (uint32_t packed : data->chunk->m_fluidUpdates) {
                int lx = (packed >> 16) & 0xFF;
                int ly = (packed >> 8) & 0xFF;
                int lz = packed & 0xFF;
                int wx = mc.x * Chunk::SizeX + lx;
                int wz = mc.z * Chunk::SizeZ + lz;
                uint8_t b = data->chunk->get(lx, ly, lz);
                scheduleBlockUpdate(wx, ly, wz, (b == BLOCK_LAVA) ? LAVA_TICK_DELAY : WATER_TICK_DELAY);
            }
            data->chunk->m_fluidUpdates.clear();

            m_chunks[key] = std::move(data);
            m_generatingChunks.erase(key);
            m_newlyGeneratedChunks.push_back({mc.x, mc.z});
        }
        chunksDispatchedThisFrame++;
    }

    // Generation-placed fluid (oceans, rivers, waterfalls, volcano lava) is
    // STATIC — it does not simulate until the player disturbs it.  The setBlock()
    // adjacent-fluid wake-up handles activating nearby fluid when the player
    // breaks or places a block next to water/lava.  This matches Minecraft behaviour.

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

        uint64_t bkey = getBlockKey(x, y, z);

        if (blockIsFluid(type)) {
            // New fluid source placed by player — no level entry = source (level 0)
            m_fluidLevels.erase(bkey);
            scheduleBlockUpdate(x, y, z, (type == BLOCK_LAVA) ? LAVA_TICK_DELAY : WATER_TICK_DELAY);
        } else if (type == BLOCK_FIRE) {
            scheduleBlockUpdate(x, y, z, FIRE_TICK_DELAY);
        } else {
            // Non-fluid/fire block: clean up any stale simulation state
            m_fluidLevels.erase(bkey);
            m_fireAge.erase(bkey);
            // Wake up adjacent fluid blocks so evaporation/retraction can run.
            // Without this, flowing blocks never recheck when their source is removed.
            const int adx[] = {1,-1,0,0,0,0};
            const int ady[] = {0,0,0,0,1,-1};
            const int adz[] = {0,0,1,-1,0,0};
            for (int i = 0; i < 6; i++) {
                uint8_t nb = getBlock(x+adx[i], y+ady[i], z+adz[i]);
                if (blockIsFluid(nb))
                    scheduleBlockUpdate(x+adx[i], y+ady[i], z+adz[i],
                        blockIsLava(nb) ? LAVA_TICK_DELAY : WATER_TICK_DELAY);
            }
        }
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
    // Fluids and foliage are non-solid for collision.
    return block != BLOCK_AIR &&
           !blockIsFluid(block) &&
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

// ---------------------------------------------------------------------------
// Fluid & Fire tick system
// ---------------------------------------------------------------------------

void World::scheduleBlockUpdate(int x, int y, int z, int delayTicks) {
    // Hard queue cap — prevents unbounded memory growth when many fluid blocks exist.
    static constexpr size_t MAX_QUEUE_SIZE = 100000;
    if (m_updateQueue.size() >= MAX_QUEUE_SIZE) return;

    uint64_t key = getBlockKey(x, y, z);
    if (m_scheduledSet.count(key)) return; // Already queued — drop duplicate
    m_scheduledSet.insert(key);
    m_updateQueue.push_back({x, y, z, delayTicks});
}

// Place a fluid block at (x,y,z) with a given flow level (0=source, 1-7=flowing).
// setFluidBlock bypasses the auto-scheduling in setBlock (we manage scheduling
// explicitly in processFluidUpdate) by writing directly to chunk then updating level.
void World::setFluidBlock(int x, int y, int z, uint8_t fluidType, int level, int delay) {
    // Write the block type
    int cx = (int)std::floor((float)x / Chunk::SizeX);
    int cz = (int)std::floor((float)z / Chunk::SizeZ);
    uint64_t ckey = getChunkKey(cx, cz);
    auto it = m_chunks.find(ckey);
    if (it == m_chunks.end()) return;

    int lx = x - cx * Chunk::SizeX;
    int lz = z - cz * Chunk::SizeZ;
    if (lx < 0 || lx >= Chunk::SizeX || lz < 0 || lz >= Chunk::SizeZ ||
        y < 0 || y >= Chunk::SizeY) return;

    it->second->chunk->set(lx, y, lz, fluidType);
    it->second->dirty = true;

    // Mark edge neighbours dirty
    if (lx == 0) { auto n = m_chunks.find(getChunkKey(cx-1, cz)); if (n != m_chunks.end()) n->second->dirty = true; }
    if (lx == Chunk::SizeX-1) { auto n = m_chunks.find(getChunkKey(cx+1, cz)); if (n != m_chunks.end()) n->second->dirty = true; }
    if (lz == 0) { auto n = m_chunks.find(getChunkKey(cx, cz-1)); if (n != m_chunks.end()) n->second->dirty = true; }
    if (lz == Chunk::SizeZ-1) { auto n = m_chunks.find(getChunkKey(cx, cz+1)); if (n != m_chunks.end()) n->second->dirty = true; }

    // Update fluid level
    uint64_t bkey = getBlockKey(x, y, z);
    if (level <= 0) {
        m_fluidLevels.erase(bkey);
    } else {
        m_fluidLevels[bkey] = static_cast<uint8_t>(level);
    }

    // Schedule tick for the new fluid block
    int effectiveDelay = (delay >= 0) ? delay
                       : ((fluidType == BLOCK_LAVA) ? LAVA_TICK_DELAY : WATER_TICK_DELAY);
    scheduleBlockUpdate(x, y, z, effectiveDelay);
}

// Returns true if a fluid-vs-fluid interaction occurred and caller should not
// place the fluid at (x,y,z).
bool World::applyFluidInteraction(int x, int y, int z, bool placingWater) {
    uint8_t cur = getBlock(x, y, z);

    if (placingWater) {
        if (blockIsLava(cur)) {
            if (getFluidLevel(x, y, z) == 0) {
                // Water flows into lava source → Obsidian
                setBlock(x, y, z, BLOCK_OBSIDIAN);
            } else {
                // Water flows into flowing lava → Cobblestone
                setBlock(x, y, z, BLOCK_COBBLESTONE);
            }
            return true;
        }
    } else {
        if (blockIsWater(cur)) {
            // Lava flows into any water → Stone
            setBlock(x, y, z, BLOCK_STONE);
            return true;
        }
    }
    return false;
}

void World::processFluidUpdate(int x, int y, int z, bool isLavaType) {
    uint8_t block = getBlock(x, y, z);

    // Abort if the block was already replaced by something else
    if (isLavaType) {
        if (!blockIsLava(block)) return;
    } else {
        if (!blockIsWater(block)) return;
    }

    const int level    = getFluidLevel(x, y, z);
    const int maxLevel = isLavaType ? 4 : 7;  // Lava: 4 levels (Minecraft overworld), Water: 7
    const int baseDelay = isLavaType ? LAVA_TICK_DELAY : WATER_TICK_DELAY;

    const int dx[] = {1, -1, 0, 0};
    const int dz[] = {0, 0, 1, -1};
    const uint8_t fluidType = isLavaType ? BLOCK_LAVA : BLOCK_WATER;

    // ------------------------------------------------------------------
    // 1. Gravity — check block directly below first
    // ------------------------------------------------------------------
    if (y > 0) {
        uint8_t below = getBlock(x, y - 1, z);
        // Only treat as an active gravity step if below can actually receive more
        // fluid.  If below is already the same fluid at same-or-lower level it is
        // "full" — fall through to horizontal spread instead of looping forever.
        // Falling fluid is level 8. Source is level 0.
        bool isSameFluid = (below == fluidType);
        bool belowAlreadyFull = isSameFluid && (getFluidLevel(x, y - 1, z) == 0 || getFluidLevel(x, y - 1, z) == 8);
        if (blockIsReplaceable(below) && !belowAlreadyFull) {
            if (!applyFluidInteraction(x, y - 1, z, !isLavaType)) {
                setFluidBlock(x, y - 1, z, fluidType, 8, baseDelay); // 8 = falling
            }
            // In Minecraft, if water can flow down, it DOES NOT flow horizontally!
            return;
        }
    }

    // Guard: a source block directly above falling water (level 8) has a
    // fall column still forming below.  Don't spread sideways yet — water
    // placed in air should fall straight down first, then spread once settled.
    // Cliff-edge sources are unaffected: their y-1 is solid stone, not water.
    if (!isLavaType && level == 0 && y > 0) {
        uint8_t belowBlk = getBlock(x, y - 1, z);
        if (blockIsWater(belowBlk) && getFluidLevel(x, y - 1, z) == 8) {
            scheduleBlockUpdate(x, y, z, WATER_TICK_DELAY);
            return;
        }
    }

    // ------------------------------------------------------------------
    // 2. Minecraft "descent check" — before spreading horizontally, find
    //    which directions lead to a downward gap within the next block.
    //    If ANY direction leads down, spread ONLY toward those directions.
    //    This replicates the Minecraft behavior where water rushes toward
    //    holes and cliff edges instead of spreading equally in all directions.
    // ------------------------------------------------------------------
    bool canDescend[4] = {false, false, false, false};
    bool anyCanDescend = false;
    if (y > 0 && (level < maxLevel || level == 8)) {
        for (int i = 0; i < 4; i++) {
            const int nx = x + dx[i], nz = z + dz[i];
            if (!blockIsReplaceable(getBlock(nx, y, nz))) continue;
            if (blockIsReplaceable(getBlock(nx, y - 1, nz))) {
                canDescend[i] = true;
                anyCanDescend = true;
            }
        }
    }

    // ------------------------------------------------------------------
    // 3. Horizontal spread — stagger each direction by a fixed offset so
    //    all 4 neighbours don't activate simultaneously (prevents waves).
    //    Water: directions fire at baseDelay + 0,3,6,9 ticks.
    //    Level-based scaling: each level further from the source adds 1
    //    extra tick so water "slows" naturally as it spreads further.
    // ------------------------------------------------------------------
    if (level < maxLevel || level == 8) {
        int newLevel = (level == 8) ? 1 : level + 1;
        // Water stagger: fixed 3-tick gap so directions are distinctly
        // separated (was baseDelay/4 = 1 — too tight, caused wave flooding).
        // Lava stagger: baseDelay/5 to keep it slow but sequential.
        const int stagger = isLavaType ? (baseDelay / 5) : 3;
        // Level scaling: deeper levels (further from source) spread slightly
        // slower — simulates natural flow attenuation.
        const int levelPenalty = (newLevel > 3) ? (newLevel - 3) : 0;

        for (int i = 0; i < 4; i++) {
            // Descent filter: if any neighbour leads downward, ONLY flow toward
            // those neighbours. This makes fluid rush to holes/edges first,
            // exactly like Minecraft's horizontal-flow behaviour.
            if (anyCanDescend && !canDescend[i]) continue;

            int nx = x + dx[i], nz = z + dz[i];
            uint8_t neighbor = getBlock(nx, y, nz);

            // State check: only spread if the neighbour is worse (higher level)
            // If it's a different fluid, we must attempt to spread so interaction (e.g. cobblestone) occurs.
            bool isSameFluidNeighbor = (neighbor == fluidType);
            int neighborLevel = isSameFluidNeighbor ? getFluidLevel(nx, y, nz) : 9;
            if (newLevel >= neighborLevel) continue;

            if (blockIsReplaceable(neighbor)) {
                if (!applyFluidInteraction(nx, y, nz, !isLavaType)) {
                    // Each direction gets a distinctly staggered delay + level attenuation
                    int neighborDelay = baseDelay + i * stagger + levelPenalty;
                    setFluidBlock(nx, y, nz, fluidType, newLevel, neighborDelay);
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // 4. Source-block creation (water only — "infinite water" mechanic)
    // ------------------------------------------------------------------
    if (!isLavaType && level > 0) {
        uint8_t belowCheck = (y > 0) ? getBlock(x, y - 1, z) : BLOCK_BEDROCK;
        if (!blockIsReplaceable(belowCheck)) {
            int srcCount = 0;
            for (int i = 0; i < 4; i++) {
                uint8_t nb = getBlock(x + dx[i], y, z + dz[i]);
                if (nb == BLOCK_WATER && getFluidLevel(x + dx[i], y, z + dz[i]) == 0)
                    srcCount++;
            }
            if (srcCount >= 2) {
                m_fluidLevels.erase(getBlockKey(x, y, z));
                scheduleBlockUpdate(x, y, z, WATER_TICK_DELAY);
                return;
            }
        }
    }

    // ------------------------------------------------------------------
    // 5. Evaporation — remove flowing blocks no longer fed by a source
    // ------------------------------------------------------------------
    if (level > 0) {
        bool fed = false;
        for (int i = 0; i < 4; i++) {
            uint8_t nb = getBlock(x + dx[i], y, z + dz[i]);
            bool sameFluid = isLavaType ? blockIsLava(nb) : blockIsWater(nb);
            if (sameFluid && getFluidLevel(x + dx[i], y, z + dz[i]) < level) {
                fed = true; break;
            }
        }
        if (!fed && y + 1 < Chunk::SizeY) {
            uint8_t above = getBlock(x, y + 1, z);
            if (isLavaType ? blockIsLava(above) : blockIsWater(above)) fed = true;
        }
        if (!fed) {
            setBlock(x, y, z, BLOCK_AIR);
        }
    }
}

void World::processFireUpdate(int x, int y, int z, bool isRaining) {
    if (getBlock(x, y, z) != BLOCK_FIRE) return;

    uint64_t key = getBlockKey(x, y, z);

    if (isRaining) {
        setBlock(x, y, z, BLOCK_AIR);
        m_fireAge.erase(key);
        return;
    }

    const int dx[] = {1, -1, 0, 0, 0, 0};
    const int dy[] = {0, 0, 0, 0, 1, -1};
    const int dz[] = {0, 0, 1, -1, 0, 0};

    // Water adjacency extinguishes fire
    for (int i = 0; i < 6; i++) {
        if (blockIsWater(getBlock(x + dx[i], y + dy[i], z + dz[i]))) {
            setBlock(x, y, z, BLOCK_AIR);
            m_fireAge.erase(key);
            return;
        }
    }

    uint8_t age = m_fireAge.count(key) ? m_fireAge.at(key) : 0;
    m_fireAge[key] = ++age;

    bool hasFlammable = false;
    for (int i = 0; i < 6; i++) {
        if (blockIsFlammable(getBlock(x + dx[i], y + dy[i], z + dz[i]))) {
            hasFlammable = true; break;
        }
    }

    if (!hasFlammable || age > 15) {
        setBlock(x, y, z, BLOCK_AIR);
        m_fireAge.erase(key);
        return;
    }

    std::uniform_int_distribution<int> roll(0, 99);

    // Burn adjacent fuel blocks
    for (int i = 0; i < 6; i++) {
        int nx = x + dx[i], ny = y + dy[i], nz = z + dz[i];
        uint8_t nb = getBlock(nx, ny, nz);
        if (blockBurnRate(nb) > 0 && roll(m_rng) < blockBurnRate(nb)) {
            setBlock(nx, ny, nz, BLOCK_AIR);
        }
    }

    // Spread fire into nearby air blocks adjacent to flammable blocks (3x3x4 box)
    for (int sy = 0; sy <= 3; sy++) {
        for (int sx = -1; sx <= 1; sx++) {
            for (int sz = -1; sz <= 1; sz++) {
                int cx = x + sx, cy = y + sy, cz = z + sz;
                if (getBlock(cx, cy, cz) != BLOCK_AIR) continue;
                for (int i = 0; i < 6; i++) {
                    int flam = blockFlammability(getBlock(cx + dx[i], cy + dy[i], cz + dz[i]));
                    if (flam > 0 && roll(m_rng) < flam / 4) {
                        setBlock(cx, cy, cz, BLOCK_FIRE);
                        scheduleBlockUpdate(cx, cy, cz, FIRE_TICK_DELAY);
                        break;
                    }
                }
            }
        }
    }

    // Reschedule self
    scheduleBlockUpdate(x, y, z, FIRE_TICK_DELAY);
}

void World::tick(bool isRaining) {
    m_tickCounter++;

    // Age all queued updates; collect ready ones
    std::vector<ScheduledUpdate> ready;
    std::deque<ScheduledUpdate>  remaining;
    ready.reserve(m_updateQueue.size());

    for (auto& upd : m_updateQueue) {
        if (--upd.delayTicks <= 0) {
            m_scheduledSet.erase(getBlockKey(upd.x, upd.y, upd.z));
            ready.push_back(upd);
        } else {
            remaining.push_back(upd);
        }
    }
    m_updateQueue = std::move(remaining);

    // Time-slice: process up to MAX_FLUID_UPDATES_PER_TICK ready items
    int processed = 0;
    for (auto& upd : ready) {
        if (processed >= MAX_FLUID_UPDATES_PER_TICK) {
            // Defer to the next tick to maintain FPS while clearing the backlog as fast as possible.
            scheduleBlockUpdate(upd.x, upd.y, upd.z, 1);
            continue;
        }
        uint8_t block = getBlock(upd.x, upd.y, upd.z);
        if (blockIsWater(block)) {
            processFluidUpdate(upd.x, upd.y, upd.z, false);
        } else if (blockIsLava(block)) {
            processFluidUpdate(upd.x, upd.y, upd.z, true);
        } else if (block == BLOCK_FIRE) {
            processFireUpdate(upd.x, upd.y, upd.z, isRaining);
        }
        processed++;
    }

    // Random block ticks (ambient fire spread)
    if (!m_chunks.empty()) {
        const int RANDOM_TICKS = 3;
        std::uniform_int_distribution<size_t> chunkDist(0, m_chunks.size() - 1);
        std::uniform_int_distribution<int>    xDist(0, Chunk::SizeX - 1);
        std::uniform_int_distribution<int>    yDist(0, Chunk::SizeY - 1);
        std::uniform_int_distribution<int>    zDist(0, Chunk::SizeZ - 1);

        for (int i = 0; i < RANDOM_TICKS; i++) {
            auto it = m_chunks.begin();
            std::advance(it, chunkDist(m_rng));
            if (it == m_chunks.end()) continue;
            int lx = xDist(m_rng), ly = yDist(m_rng), lz = zDist(m_rng);
            if (it->second->chunk->get(lx, ly, lz) == BLOCK_FIRE) {
                int wx = it->second->x * Chunk::SizeX + lx;
                int wz = it->second->z * Chunk::SizeZ + lz;
                processFireUpdate(wx, ly, wz, isRaining);
            }
        }
    }
}
