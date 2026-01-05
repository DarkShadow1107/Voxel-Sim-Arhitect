#pragma once

#include <cstdint>
#include <vector>
#include "Modules/BlockData.hpp"

class FastNoiseLite;

class Chunk {
public:
    static constexpr int SizeX = 32;
    static constexpr int SizeY = 128; // Increased for deep caves
    static constexpr int SizeZ = 32;

    Chunk();

    uint8_t get(int x, int y, int z) const;
    void set(int x, int y, int z, uint8_t v);

    uint8_t* getData() { return m_voxels.data(); }

    void generateTerrain(FastNoiseLite& noise, int seed, float frequency, int baseHeight, int offsetX, int offsetZ);
    void generateCaves(FastNoiseLite& noise, int seed);

private:
    static constexpr int kVoxelCount = SizeX * SizeY * SizeZ;
    std::vector<uint8_t> m_voxels;

    static int idx(int x, int y, int z) { 
        if (x < 0 || x >= SizeX || y < 0 || y >= SizeY || z < 0 || z >= SizeZ) return -1;
        return x + SizeX * (y + SizeY * z); 
    }
};
