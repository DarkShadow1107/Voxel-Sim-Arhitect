#pragma once
#include <string>
#include <vector>
#include <cstdint>

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
    BLOCK_COUNT
};

struct BlockInfo {
    uint8_t id;
    std::string name;
    int tx, ty;
};

namespace BlockData {
    inline BlockInfo getInfo(uint8_t id) {
        switch(id) {
            case BLOCK_DIRT: return {id, "Dirt", 0, 0};
            case BLOCK_GRASS: return {id, "Grass", 1, 0};
            case BLOCK_STONE: return {id, "Stone", 3, 0};
            case BLOCK_WATER: return {id, "Water", 4, 0};
            case BLOCK_LAVA: return {id, "Lava", 5, 0};
            case BLOCK_WOOD: return {id, "Oak Log", 6, 0};
            case BLOCK_LEAVES: return {id, "Oak Leaves", 7, 0};
            case BLOCK_SAND: return {id, "Sand", 8, 0};
            case BLOCK_SNOW: return {id, "Snow", 9, 0};
            case BLOCK_BEDROCK: return {id, "Bedrock", 10, 0};
            case BLOCK_FLOWER_RED: return {id, "Red Flower", 11, 0};
            case BLOCK_FLOWER_BLUE: return {id, "Blue Flower", 12, 0};
            case BLOCK_TALL_GRASS: return {id, "Tall Grass", 13, 0};
            case BLOCK_GLASS: return {id, "Glass", 14, 0};
            case BLOCK_COAL_ORE: return {id, "Coal Ore", 0, 1};
            case BLOCK_IRON_ORE: return {id, "Iron Ore", 1, 1};
            case BLOCK_GOLD_ORE: return {id, "Gold Ore", 2, 1};
            case BLOCK_DIAMOND_ORE: return {id, "Diamond Ore", 3, 1};
            case BLOCK_BIRCH_WOOD: return {id, "Birch Log", 4, 1};
            case BLOCK_BIRCH_LEAVES: return {id, "Birch Leaves", 5, 1};
            case BLOCK_CHERRY_WOOD: return {id, "Cherry Log", 6, 1};
            case BLOCK_CHERRY_LEAVES: return {id, "Cherry Leaves", 7, 1};
            case BLOCK_COBBLESTONE: return {id, "Cobblestone", 8, 1};
            case BLOCK_MOSSY_STONE: return {id, "Mossy Stone", 9, 1};
            case BLOCK_OAK_PLANKS: return {id, "Oak Planks", 10, 1};
            case BLOCK_BRICKS: return {id, "Bricks", 11, 1};
            case BLOCK_ICE: return {id, "Ice", 12, 1};
            default: return {0, "Air", 0, 0};
        }
    }
}
