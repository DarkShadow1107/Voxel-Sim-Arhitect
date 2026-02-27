#pragma once

#include <cstdint>
#include "Math.hpp"

class Chunk;

class StructureGenerator {
public:
    static void generateTree(Chunk* chunk, int x, int y, int z, uint8_t woodType, uint8_t leafType);
    static void generateCactus(Chunk* chunk, int x, int y, int z);
    static void generateSmallHouse(Chunk* chunk, int x, int y, int z);
    static void generateVillage(Chunk* chunk, int x, int y, int z);
    static void generateRuins(Chunk* chunk, int x, int y, int z);
    static void generateVolcanoVent(Chunk* chunk, int x, int y, int z);
};
