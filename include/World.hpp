#pragma once

#include <unordered_map>
#include <unordered_set>
#include <deque>
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
    uint32_t version = 1;
    uint8_t reserved[32] = {};
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
    // Limited to ±2^20 blocks per axis (> 1 million blocks range) — sufficient for any world.
    static uint64_t getBlockKey(int x, int y, int z) {
        return ((uint64_t)(uint32_t)x) |
               ((uint64_t)(uint32_t)y << 20) |
               ((uint64_t)(uint32_t)z << 40);
    }

    // -----------------------------------------------------------------------
    // Block-update tick queue
    // -----------------------------------------------------------------------
    struct ScheduledUpdate {
        int x, y, z;
        int delayTicks;
    };

    std::deque<ScheduledUpdate>    m_updateQueue;
    std::unordered_set<uint64_t>   m_scheduledSet;   // Prevents duplicate entries
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
};
