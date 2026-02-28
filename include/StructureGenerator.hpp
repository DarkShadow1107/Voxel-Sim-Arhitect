#pragma once

#include <cstdint>
#include "Math.hpp"

class Chunk;

class StructureGenerator {
public:
    static void generateTree(Chunk* chunk, int x, int y, int z, uint8_t woodType, uint8_t leafType);
    static void generatePineTree(Chunk* chunk, int x, int y, int z);
    static void generateMegaTree(Chunk* chunk, int x, int y, int z);
    static void generateCactus(Chunk* chunk, int x, int y, int z);
    static void generateSmallHouse(Chunk* chunk, int x, int y, int z);
    static void generateLargeHouse(Chunk* chunk, int x, int y, int z);
    static void generateVillageWell(Chunk* chunk, int x, int y, int z);
    static void generateFarmPlot(Chunk* chunk, int x, int y, int z, int w, int d);
    static void generatePath(Chunk* chunk, int x1, int z1, int x2, int z2);
    static void generateVillage(Chunk* chunk, int x, int y, int z);
    static void generateRuins(Chunk* chunk, int x, int y, int z);
    static void generateVolcanoVent(Chunk* chunk, int x, int y, int z);
    static void generateObsidianSpire(Chunk* chunk, int x, int y, int z);
};
