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
    BLOCK_SNOW_LAYER = 29,
    BLOCK_OBSIDIAN = 70      // registered in Registry.cpp; no live lava in generation
    // Fluid levels are stored separately in World::m_fluidLevels
    // (all 256 block IDs are occupied by the Minecraft block registry)
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

// ---------------------------------------------------------------------------
// Fluid & Fire helper functions
// Fluid flow levels (0=source, 1-7=flowing) are stored in World::m_fluidLevels.
// All helpers below operate only on the block type byte.
// ---------------------------------------------------------------------------

// Returns true if the block is water (source or any flow level)
inline bool blockIsWater(uint8_t b) {
    return b == BLOCK_WATER;
}

// Returns true if the block is lava (source or any flow level)
inline bool blockIsLava(uint8_t b) {
    return b == BLOCK_LAVA;
}

// Returns true if the block is any fluid
inline bool blockIsFluid(uint8_t b) {
    return b == BLOCK_WATER || b == BLOCK_LAVA;
}

// Returns true if the block can be replaced by a flowing fluid
inline bool blockIsReplaceable(uint8_t b) {
    return b == BLOCK_AIR || blockIsFluid(b) ||
           b == BLOCK_FLOWER_RED || b == BLOCK_FLOWER_BLUE || b == BLOCK_TALL_GRASS;
}

// Returns true if the block type is flammable
inline bool blockIsFlammable(uint8_t b) {
    switch (b) {
        case BLOCK_WOOD:         return true;
        case BLOCK_BIRCH_WOOD:   return true;
        case BLOCK_CHERRY_WOOD:  return true;
        case BLOCK_OAK_PLANKS:   return true;
        case BLOCK_LEAVES:       return true;
        case BLOCK_BIRCH_LEAVES: return true;
        case BLOCK_CHERRY_LEAVES:return true;
        case BLOCK_TALL_GRASS:   return true;
        case BLOCK_FLOWER_RED:   return true;
        case BLOCK_FLOWER_BLUE:  return true;
        default:                 return false;
    }
}

// Returns flammability 0-100 (chance per random tick to catch fire from adjacent flame)
inline int blockFlammability(uint8_t b) {
    switch (b) {
        case BLOCK_TALL_GRASS:   return 100;
        case BLOCK_FLOWER_RED:   return 100;
        case BLOCK_FLOWER_BLUE:  return 100;
        case BLOCK_LEAVES:       return 60;
        case BLOCK_BIRCH_LEAVES: return 60;
        case BLOCK_CHERRY_LEAVES:return 60;
        case BLOCK_WOOD:         return 20;
        case BLOCK_BIRCH_WOOD:   return 20;
        case BLOCK_CHERRY_WOOD:  return 20;
        case BLOCK_OAK_PLANKS:   return 20;
        default:                 return 0;
    }
}

// Returns burn rate 0-100 (chance per random tick to be destroyed by adjacent fire)
inline int blockBurnRate(uint8_t b) {
    switch (b) {
        case BLOCK_TALL_GRASS:   return 100;
        case BLOCK_FLOWER_RED:   return 100;
        case BLOCK_FLOWER_BLUE:  return 100;
        case BLOCK_LEAVES:       return 30;
        case BLOCK_BIRCH_LEAVES: return 30;
        case BLOCK_CHERRY_LEAVES:return 30;
        case BLOCK_WOOD:         return 5;
        case BLOCK_BIRCH_WOOD:   return 5;
        case BLOCK_CHERRY_WOOD:  return 5;
        case BLOCK_OAK_PLANKS:   return 5;
        default:                 return 0;
    }
}

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
    // Second-pass: place river water, waterfall sources, and volcanic lava.
    // Must be called after generateTerrain() and generateCaves().
    void generateWaterbodies(FastNoiseLite& noise, int seed, float frequency, int baseHeight, int offsetX, int offsetZ);

    static BiomeType getBiomeAt(FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise, float wx, float wz);

    std::vector<uint32_t> m_fluidUpdates; // Stores (x << 16) | (y << 8) | z for fluids that need initial block updates

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
