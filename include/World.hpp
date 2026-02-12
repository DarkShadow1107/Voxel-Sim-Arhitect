#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <future>
#include <mutex>
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
    void render(const class Shader& shader, const Mat4& viewProj);

    void clear();
    void save(const std::string& filename, const WorldMetadata& meta);
    bool load(const std::string& filename, WorldMetadata& metaOut);

    static std::vector<WorldSaveInfo> listSaves(const std::string& directory = "saves");

    void setBlock(int x, int y, int z, uint8_t type);
    uint8_t getBlock(int x, int y, int z) const;
    bool isSolid(int x, int y, int z) const;
    
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

private:
    std::unordered_map<uint64_t, std::unique_ptr<ChunkData>> m_chunks;
    std::vector<std::pair<int, int>> m_newlyGeneratedChunks;
    int m_renderDistance = 4;

    struct MeshResult {
        uint64_t key;
        std::vector<Vertex> vertices;
    };
    std::vector<MeshResult> m_meshResults;
    std::mutex m_resultMutex;

    static uint64_t getChunkKey(int x, int z) {
        return ((uint64_t)x & 0xFFFFFFFF) | (((uint64_t)z & 0xFFFFFFFF) << 32);
    }
};
