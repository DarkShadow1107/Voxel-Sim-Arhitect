#include "StructureGenerator.hpp"
#include "Chunk.hpp"
#include <cstdlib>

void StructureGenerator::generateTree(Chunk* chunk, int x, int y, int z, uint8_t woodType, uint8_t leafType) {
    int height = 4 + rand() % 3;
    for (int i = 0; i < height; ++i) {
        chunk->set(x, y + i, z, woodType);
    }

    for (int ly = -2; ly <= 2; ++ly) {
        for (int lx = -2; lx <= 2; ++lx) {
            for (int lz = -2; lz <= 2; ++lz) {
                if (lx * lx + ly * ly + lz * lz <= 6) {
                    if (chunk->get(x + lx, y + height + ly, z + lz) == 0) {
                        chunk->set(x + lx, y + height + ly, z + lz, leafType);
                    }
                }
            }
        }
    }
}

void StructureGenerator::generateCactus(Chunk* chunk, int x, int y, int z) {
    int height = 2 + rand() % 2;
    for (int i = 0; i < height; ++i) {
        chunk->set(x, y + i, z, BLOCK_TALL_GRASS); // Using tall grass as placeholder for now
    }
}

void StructureGenerator::generateSmallHouse(Chunk* chunk, int x, int y, int z) {
    int w = 5, h = 4, d = 5;
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            for (int dz = 0; dz < d; ++dz) {
                bool wall = (dx == 0 || dx == w - 1 || dz == 0 || dz == d - 1);
                if (dy == 0) chunk->set(x + dx, y + dy, z + dz, BLOCK_OAK_PLANKS);
                else if (dy == h - 1) chunk->set(x + dx, y + dy, z + dz, BLOCK_OAK_PLANKS);
                else if (wall) {
                    if (dx == w / 2 && dz == 0 && dy < 3) chunk->set(x + dx, y + dy, z + dz, 0); // Door
                    else chunk->set(x + dx, y + dy, z + dz, BLOCK_COBBLESTONE);
                }
            }
        }
    }
}

void StructureGenerator::generateVillage(Chunk* chunk, int x, int y, int z) {
    // Generate a small village with a few houses and paths
    int numHouses = 3 + rand() % 3;
    for (int i = 0; i < numHouses; ++i) {
        int hx = x + (rand() % 20 - 10);
        int hz = z + (rand() % 20 - 10);
        // Find ground level
        int hy = y;
        while (hy > 0 && chunk->get(hx, hy - 1, hz) == 0) hy--;
        while (hy < Chunk::SizeY && chunk->get(hx, hy, hz) != 0) hy++;
        
        if (hy > 0 && hy < Chunk::SizeY - 5) {
            generateSmallHouse(chunk, hx, hy, hz);
        }
    }
    
    // Generate some paths
    for (int i = 0; i < 20; ++i) {
        int px = x + (rand() % 20 - 10);
        int pz = z + (rand() % 20 - 10);
        int py = y;
        while (py > 0 && chunk->get(px, py - 1, pz) == 0) py--;
        while (py < Chunk::SizeY && chunk->get(px, py, pz) != 0) py++;
        
        if (py > 0 && py < Chunk::SizeY) {
            chunk->set(px, py - 1, pz, BLOCK_DIRT); // Path block placeholder
        }
    }
}

void StructureGenerator::generateRuins(Chunk* chunk, int x, int y, int z) {
    for (int i = 0; i < 15; ++i) {
        int rx = rand() % 5;
        int ry = rand() % 3;
        int rz = rand() % 5;
        chunk->set(x + rx, y + ry, z + rz, (rand() % 2 == 0) ? BLOCK_COBBLESTONE : BLOCK_MOSSY_STONE);
    }
}

void StructureGenerator::generateVolcanoVent(Chunk* chunk, int x, int y, int z) {
    for (int i = 0; i < 5; ++i) {
        chunk->set(x, y - i, z, BLOCK_LAVA);
    }
}
