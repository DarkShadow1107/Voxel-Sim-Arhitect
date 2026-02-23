#pragma once

#include <cstdint>
#include <vector>
#include "Allocator.hpp"

class FastNoiseLite;

enum BlockType : uint8_t {
    BLOCK_AIR = 0,
    BLOCK_DIRT = 1,
    BLOCK_GRASS = 2,
    BLOCK_STONE = 3,
    BLOCK_WATER = 4,
    BLOCK_LAVA = 5,
    BLOCK_WOOD = 6,
    BLOCK_LEAVES = 7,
    BLOCK_SAND = 8,
    BLOCK_SNOW = 9,
    BLOCK_BEDROCK = 10,
    BLOCK_FLOWER_RED = 11,
    BLOCK_FLOWER_BLUE = 12,
    BLOCK_TALL_GRASS = 13,
    BLOCK_GLASS = 14,
    BLOCK_COAL_ORE = 15,
    BLOCK_IRON_ORE = 16,
    BLOCK_GOLD_ORE = 17,
    BLOCK_DIAMOND_ORE = 18,
    BLOCK_BIRCH_WOOD = 19,
    BLOCK_BIRCH_LEAVES = 20,
    BLOCK_CHERRY_WOOD = 21,
    BLOCK_CHERRY_LEAVES = 22,
    BLOCK_COBBLESTONE = 23,
    BLOCK_MOSSY_STONE = 24,
    BLOCK_OAK_PLANKS = 25,
    BLOCK_BRICKS = 26,
    BLOCK_ICE = 27,
    BLOCK_FIRE = 28,
    BLOCK_SNOW_LAYER = 29
};

enum BiomeType : uint8_t {
    BIOME_POLAR = 0,
    BIOME_SNOWY,
    BIOME_PLAINS,
    BIOME_SAVANNA,
    BIOME_DESERT,
    BIOME_JUNGLE,
    BIOME_VOLCANO,
    BIOME_OCEAN
};

class Chunk {
public:
    static constexpr int SizeX = 32;
    static constexpr int SizeY = 128; // Increased for deep caves
    static constexpr int SizeZ = 32;

    Chunk();
    ~Chunk();

    static PoolAllocator& getAllocator() { return s_allocator; }

    uint8_t get(int x, int y, int z) const;
    void set(int x, int y, int z, uint8_t v);

    uint8_t* getData() { return m_voxels; }

    void generateTerrain(FastNoiseLite& noise, int seed, float frequency, int baseHeight, int offsetX, int offsetZ);
    void generateCaves(FastNoiseLite& noise, int seed);

    static BiomeType getBiomeAt(FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise, float wx, float wz);

private:
    static constexpr int kVoxelCount = SizeX * SizeY * SizeZ;
    uint8_t* m_voxels;
    
    // Global allocator for all chunk voxel data
    static PoolAllocator s_allocator;

    static int idx(int x, int y, int z) { 
        if (x < 0 || x >= SizeX || y < 0 || y >= SizeY || z < 0 || z >= SizeZ) return -1;
        return x + SizeX * (y + SizeY * z); 
    }
};
