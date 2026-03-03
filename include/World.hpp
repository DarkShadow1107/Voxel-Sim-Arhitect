#pragma once

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <queue>
#include <vector>
#include <memory>
#include <future>
#include <mutex>
#include <random>
#include <string>
#include <filesystem>
#include "Chunk.hpp"
#include "GLMesh.hpp"
#include "Math.hpp"

struct WorldMetadata {
    char name[64] = "New World";
    int seed = 1337;
    float frequency = 0.02f;
    int baseHeight = 10;
    float worldTime = 6000.0f;
    uint32_t version = 3;
    // v2+: last known player position — allows restoring position on reload.
    // Old saves (version == 1) have these as zero from the former reserved[] block.
    float playerX = 0.0f;
    float playerY = 0.0f;
    float playerZ = 0.0f;
    // v3 extension fields — fit exactly in the old reserved[20] slot so the
    // on-disk struct size (116 bytes) is unchanged and v2 saves load cleanly
    // (these bytes were zero-initialised in all prior saves = safe defaults).
    float    playerYaw    = 0.0f;  // [0-3]  camera look-yaw  (degrees)
    float    playerPitch  = 0.0f;  // [4-7]  camera look-pitch (degrees)
    uint8_t  gameMode     = 0;     // [8]    0 = Creative, 1 = Survival
    uint8_t  pad[3]       = {};    // [9-11] alignment padding — reserved for future use
    uint32_t playerLevel  = 0;     // [12-15] player XP level
    uint32_t blocksPlaced = 0;     // [16-19] lifetime blocks placed
    // total v3 extension = 4+4+1+3+4+4 = 20 bytes — same size as old reserved[20]
};

struct WorldSaveInfo {
    std::string filename;
    std::string displayName;
    size_t chunkCount = 0;
    WorldMetadata metadata;
};

struct ChunkData {
    std::unique_ptr<Chunk> chunk;
    std::unique_ptr<GLMesh> mesh;
    bool dirty = true;
    bool meshing = false;
    int x, z;
};

class World {
public:
    World();
    ~World();

    void update(const Vec3& playerPos, class FastNoiseLite& noise, int seed, float freq, int baseHeight, class TaskScheduler* scheduler = nullptr);
    void tick(bool isRaining = false);
    void render(const class Shader& shader, const Mat4& viewProj);

    void clear();
    // Chunk-cache management (per-world isolation)
    void setCacheDir(const std::string& dir);        // Pre-assign before first update()
    void clearCacheDir();                            // Delete folder (orphan cleanup)
    // Restore chunk cache from the saved .vsa file. Called on "Don't Save" exit
    // to overwrite any in-session modifications with the last properly saved state.
    void restoreCacheFromSave(const std::string& filename);
    const std::string& getCacheDir() const { return m_cacheDir; }
    void save(const std::string& filename, const WorldMetadata& meta);
    bool load(const std::string& filename, WorldMetadata& metaOut);

    static std::vector<WorldSaveInfo> listSaves(const std::string& directory = "saves");

    void setBlock(int x, int y, int z, uint8_t type);
    uint8_t getBlock(int x, int y, int z) const;
    bool isSolid(int x, int y, int z) const;
    
    bool isChunkLoaded(int x, int z) const {
        int cx = (int)std::floor((float)x / Chunk::SizeX);
        int cz = (int)std::floor((float)z / Chunk::SizeZ);
        return m_chunks.find(getChunkKey(cx, cz)) != m_chunks.end();
    }
    
    Chunk* getChunk(int x, int z) const {
        uint64_t key = getChunkKey(x, z);
        auto it = m_chunks.find(key);
        if (it != m_chunks.end()) return it->second->chunk.get();
        return nullptr;
    }

    struct RaycastResult {
        bool hit = false;
        int x, y, z;
        int nx, ny, nz; // Normal of the face hit
    };
    RaycastResult raycast(const Vec3& origin, const Vec3& direction, float maxDist);

    const std::vector<std::pair<int, int>>& getNewChunks() const { return m_newlyGeneratedChunks; }
    void clearNewChunks() { m_newlyGeneratedChunks.clear(); }

    const std::vector<std::pair<int, int>>& getRemovedChunks() const { return m_removedChunks; }

    int getRenderDistance() const { return m_renderDistance; }
    void setRenderDistance(int d) { m_renderDistance = d; }
    size_t getChunkCount() const { return m_chunks.size(); }

    // Schedule a fluid/fire block update to process after delayTicks ticks.
    // Prevents duplicate scheduling for the same block coordinate.
    void scheduleBlockUpdate(int x, int y, int z, int delayTicks);

private:
    std::unordered_map<uint64_t, std::unique_ptr<ChunkData>> m_chunks;
    std::unordered_set<uint64_t> m_generatingChunks;
    std::vector<std::pair<int, int>> m_newlyGeneratedChunks;
    std::vector<std::pair<int, int>> m_removedChunks;
    int m_renderDistance = 4;

    struct GenResult {
        int x, z;
        std::unique_ptr<Chunk> chunk;
    };
    std::vector<GenResult> m_genResults;
    std::mutex m_genResultMutex;

    struct MeshResult {
        uint64_t key;
        std::vector<Vertex> vertices;
    };
    std::vector<MeshResult> m_meshResults;
    std::mutex m_resultMutex;

    static uint64_t getChunkKey(int x, int z) {
        return ((uint64_t)x & 0xFFFFFFFF) | (((uint64_t)z & 0xFFFFFFFF) << 32);
    }

    // Pack world-space block coords into a 64-bit key for the scheduled-update set.
    // Layout (non-overlapping): x[21 bits 0-20] | y[8 bits 21-28] | z[21 bits 29-49]
    // Covers ±1,048,576 blocks per horizontal axis and 0-127 vertical — no collisions
    // for any reachable world coordinate.  The old layout had x bits 20-31 overlapping
    // with y bits, causing false-positive duplicate detection at large X coordinates.
    static uint64_t getBlockKey(int x, int y, int z) {
        const uint64_t ux = (uint64_t)(uint32_t)x & 0x1FFFFFull;  // low 21 bits
        const uint64_t uy = (uint64_t)(y & 0xFF);                   // low  8 bits
        const uint64_t uz = (uint64_t)(uint32_t)z & 0x1FFFFFull;  // low 21 bits
        return ux | (uy << 21) | (uz << 29);
    }

    // -----------------------------------------------------------------------
    // Block-update tick queue  — min-heap ordered by absolute fireTick
    // -----------------------------------------------------------------------
    struct ScheduledUpdate {
        int x, y, z;
        int fireTick;   // absolute m_tickCounter value when this update fires
    };
    struct ScheduledUpdateCmp {
        bool operator()(const ScheduledUpdate& a, const ScheduledUpdate& b) const {
            return a.fireTick > b.fireTick;  // min-heap: smallest fireTick at top
        }
    };

    std::priority_queue<ScheduledUpdate,
                        std::vector<ScheduledUpdate>,
                        ScheduledUpdateCmp>      m_updateQueue;
    std::unordered_set<uint64_t>                 m_scheduledSet;   // Prevents duplicate entries
    // Fluid level map: key = getBlockKey(x,y,z), value = level 0..7
    // 0 = source block (default, no entry needed), 1-7 = flowing at that depletion.
    // Only BLOCK_WATER / BLOCK_LAVA block coordinates have entries here.
    std::unordered_map<uint64_t, uint8_t> m_fluidLevels;
    std::unordered_map<uint64_t, uint8_t> m_fireAge; // fire block age (0-15)
    std::mt19937                   m_rng{ 12345 };
    int                            m_tickCounter = 0;

    static constexpr int MAX_FLUID_UPDATES_PER_TICK = 200;   // tighter cap — prevents frame burst

    // Tick delay constants (in game ticks at 20 Hz)
    // Water:  8 ticks = 0.40 s per horizontal spread step  (was 5 = 0.25 s — felt too fast)
    // Lava:  30 ticks = 1.50 s per spread step             (unchanged, lava is already slow)
    // Fire:  20 ticks = 1.00 s per spread step             (unchanged)
    static constexpr int WATER_TICK_DELAY = 8;
    static constexpr int LAVA_TICK_DELAY  = 30;
    static constexpr int FIRE_TICK_DELAY  = 20;

    // Fluid level accessors using m_fluidLevels
    // Returns 0 (source) if no entry exists for this coordinate
    int getFluidLevel(int x, int y, int z) const {
        auto it = m_fluidLevels.find(getBlockKey(x, y, z));
        return (it != m_fluidLevels.end()) ? it->second : 0;
    }
    // Write a fluid block + its level in one step.
    // delay: ticks until the first update (-1 = auto-choose from fluid type)
    void setFluidBlock(int x, int y, int z, uint8_t fluidType, int level, int delay = -1);
    // Remove the fluid level entry (call after setBlock(x,y,z,BLOCK_AIR))
    void clearFluidLevel(int x, int y, int z) {
        m_fluidLevels.erase(getBlockKey(x, y, z));
    }

    // Fluid simulation helpers
    void processFluidUpdate(int x, int y, int z, bool isLavaType);
    void processFireUpdate(int x, int y, int z, bool isRaining);
    bool applyFluidInteraction(int x, int y, int z, bool placingWater);

    // -----------------------------------------------------------------------
    // Per-chunk disk cache
    // Explored chunks are written to disk on unload and read back on re-entry
    // instead of re-running expensive terrain generation every time.
    // Cache is scoped to the world seed (chunk_cache/<seed>/c_X_Z.bin).
    // -----------------------------------------------------------------------
    std::string m_cacheDir;  // lazily initialised on first update(); empty = not yet set

    std::string getChunkCachePath(int cx, int cz) const;
    // Called from background threads — reads only from m_cacheDir (immutable after init).
    // Returns true and fills *out with voxel data if a valid cache file exists.
    bool loadChunkFromCache(int cx, int cz, Chunk* out) const;
    // Called on the main thread during chunk unload.
    void saveChunkToCache(int cx, int cz, Chunk* chunk) const;

    // Maximum number of GLMesh uploads (glBufferData / glBufferSubData) per
    // update() call.  Each upload stalls the main thread while the GPU finishes
    // with the old buffer, so spreading them over multiple frames keeps latency
    // flat regardless of how many mesh results are pending.
    // Raised from 3→5: the extra 2 uploads cost ~1ms more but significantly
    // reduce the backlog when entering a large new area.
    static constexpr int MAX_MESH_UPLOADS_PER_FRAME  = 5;
    // Maximum mesh tasks allowed in-flight at once across all background threads.
    // Raised from 4→6 to fill thread pool better when player moves through cached terrain.
    static constexpr int MAX_CONCURRENT_MESH_TASKS   = 6;
};
