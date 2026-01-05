#include "Chunk.hpp"
#include "FastNoiseLite.h"
#include "StructureGenerator.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

// Initialize the static pool allocator for up to 4096 chunks
PoolAllocator Chunk::s_allocator(kVoxelCount * sizeof(uint8_t), 4096);

Chunk::Chunk() {
    m_voxels = static_cast<uint8_t*>(s_allocator.allocate());
    if (m_voxels) {
        std::memset(m_voxels, 0, kVoxelCount);
    } else {
        std::cerr << "FATAL ERROR: Chunk allocation failed! Pool exhausted." << std::endl;
        // Safety check: if pool is exhausted, some parts of the game may crash
        // In a real scenario, we might want to use a fallback or force a garbage collection
    }
}

Chunk::~Chunk() {
    if (m_voxels) {
        s_allocator.deallocate(m_voxels);
    }
}

uint8_t Chunk::get(int x, int y, int z) const {
    if (!m_voxels || x < 0 || y < 0 || z < 0 || x >= SizeX || y >= SizeY || z >= SizeZ) {
        return 0;
    }
    return m_voxels[(size_t)idx(x, y, z)];
}

void Chunk::set(int x, int y, int z, uint8_t v) {
    if (!m_voxels || x < 0 || y < 0 || z < 0 || x >= SizeX || y >= SizeY || z >= SizeZ) {
        return;
    }
    m_voxels[(size_t)idx(x, y, z)] = v;
}

void Chunk::generateTerrain(FastNoiseLite& noise, int seed, float frequency, int baseHeight, int offsetX, int offsetZ) {
    if (!m_voxels) return;

    noise.SetSeed(seed);
    noise.SetFrequency(frequency);

    std::memset(m_voxels, 0, kVoxelCount);

    FastNoiseLite mountainNoise;
    mountainNoise.SetSeed(seed + 1);
    mountainNoise.SetFrequency(frequency * 0.5f);
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite riverNoise;
    riverNoise.SetSeed(seed + 5);
    riverNoise.SetFrequency(std::max(0.0025f, frequency * 0.10f));
    riverNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(seed + 10);
    continentalNoise.SetFrequency(frequency * 0.08f); // Even lower for larger landmasses
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(seed + 20);
    biomeNoise.SetFrequency(frequency * 0.05f); // Much lower frequency for huge biomes
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite mountainMixNoise;
    mountainMixNoise.SetSeed(seed + 30);
    mountainMixNoise.SetFrequency(frequency * 0.15f); // More frequent mountain patches
    mountainMixNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    const int seaLevel = 12;

    for (int z = 0; z < SizeZ; ++z) {
        for (int x = 0; x < SizeX; ++x) {
            float wx = (float)(x + offsetX);
            float wz = (float)(z + offsetZ);
            
            const float n = noise.GetNoise(wx, wz);
            const float mn = mountainNoise.GetNoise(wx, wz);
            const float cn = continentalNoise.GetNoise(wx, wz);
            const float bn = biomeNoise.GetNoise(wx, wz);
            const float mix = mountainMixNoise.GetNoise(wx, wz);
            
            const float h01 = (n + 1.0f) * 0.5f;
            const float mh01 = (mn + 1.0f) * 0.5f;
            
            // Base height with some variation
            int h = baseHeight + (int)(h01 * 15.0f);
            
            // Biome determination (Larger Polar/Snowy regions)
            // bn < -0.2: Polar/Ice
            // -0.2 < bn < 0.1: Snowy/Tundra
            // 0.1 < bn < 0.4: Plains/Flat
            // 0.4 < bn < 0.55: Desert
            // 0.55 < bn < 0.75: Jungle
            // bn > 0.75: Volcano/Mountainous
            
            bool isPolar = (bn < -0.2f);
            bool isSnowy = (bn >= -0.2f && bn < 0.1f);
            bool isPlains = (bn >= 0.1f && bn < 0.4f);
            bool isDesert = (bn >= 0.4f && bn < 0.55f);
            bool isJungle = (bn >= 0.55f && bn < 0.75f);
            bool isVolcano = (bn >= 0.75f);

            // Mix mountains: only some areas have high mountains
            // Volcanoes ALWAYS get height boost
            if (mix > 0.0f || isVolcano) {
                if (mh01 > 0.4f || isVolcano) {
                    float mountainFactor = (mh01 - 0.4f) * 90.0f;
                    if (isVolcano) mountainFactor = 80.0f + mh01 * 40.0f; // Stronger volcano height
                    h += (int)mountainFactor;
                }
            } else if (isPlains) {
                h = baseHeight + (int)(h01 * 5.0f); // Very flat plains
            }

            // Continental/Sea logic
            if (cn < -0.35f) {
                float seaDepth = std::abs(cn + 0.35f) * 35.0f;
                h = (int)std::max(1.0f, (float)seaLevel - seaDepth);
            }

            // Rivers
            const float rv = std::abs(riverNoise.GetNoise(wx, wz));
            const float riverWidth = 0.07f;
            const float riverStrength = std::clamp((riverWidth - rv) / riverWidth, 0.0f, 1.0f);
            if (riverStrength > 0.0f && !isDesert && !isPolar) {
                const int riverBed = seaLevel - 3;
                const int targetH = (int)std::round((1.0f - riverStrength) * (float)seaLevel + riverStrength * (float)riverBed);
                h = std::min(h, std::clamp(targetH, 1, SizeY - 1));
            }

            h = std::clamp(h, 1, SizeY - 1);

            for (int y = 0; y < SizeY; ++y) {
                if (y == 0) {
                    set(x, y, z, BLOCK_BEDROCK);
                } else if (y <= h) {
                    uint8_t type = BLOCK_DIRT;
            if (y == h) {
                        if (isPolar) {
                            type = (y < seaLevel + 1) ? BLOCK_ICE : BLOCK_SNOW;
                            // Add some ice patches on top of snow
                            if (y > seaLevel + 5 && (rand() % 100 < 5)) type = BLOCK_ICE;
                        }
                        else if (isSnowy) type = BLOCK_SNOW;
                        else if (isDesert) type = BLOCK_SAND;
                        else if (y > 105) type = BLOCK_SNOW; // Snowy peaks
                        else if (y < seaLevel + 2) type = BLOCK_SAND;
                        else type = BLOCK_GRASS;
                        
                        if (isVolcano && y > 70) type = BLOCK_STONE;
                    }
                    else if (y < h - 5) {
                        type = BLOCK_STONE;
                        if (isVolcano && y > 50 && (rand() % 100 < 8)) type = BLOCK_LAVA;
                        // Add some ores
                        int r = rand() % 1000;
                        if (y < 30 && r < 5) type = BLOCK_DIAMOND_ORE;
                        else if (y < 50 && r < 15) type = BLOCK_GOLD_ORE;
                        else if (y < 70 && r < 30) type = BLOCK_IRON_ORE;
                        else if (r < 50) type = BLOCK_COAL_ORE;
                    }
                    
                    set(x, y, z, type);
                } else if (y < seaLevel) {
                    if (isPolar) set(x, y, z, (y > seaLevel - 2) ? BLOCK_ICE : BLOCK_WATER);
                    else set(x, y, z, BLOCK_WATER);
                }
            }

            // Volcano Crater - More prominent
            if (isVolcano && h > 90) {
                float dx = (float)x - (float)SizeX/2.0f;
                float dz = (float)z - (float)SizeZ/2.0f;
                float distSq = dx*dx + dz*dz;
                if (distSq < 25.0f) {
                    for(int y=h; y>h-20; --y) {
                        if (y > 0) set(x, y, z, BLOCK_LAVA);
                    }
                }
            }

            // Flora & Structures
            if (h > seaLevel && h < 100) {
                int r = rand() % 2000;
                if (r < 18) {
                    float treeNoise = noise.GetNoise((float)wx * 0.1f, (float)wz * 0.1f);
                    if (isJungle) {
                        if (r < 15) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    } else if (isDesert) {
                        if (r < 4) StructureGenerator::generateCactus(this, x, h + 1, z);
                    } else if (isSnowy || isPolar) {
                        if (r < 6) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_SNOW);
                    } else if (!isPlains || r < 5) {
                        if (treeNoise > 0.4f) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_CHERRY_WOOD, BLOCK_CHERRY_LEAVES);
                        else if (treeNoise < -0.4f) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_BIRCH_WOOD, BLOCK_BIRCH_LEAVES);
                        else if (r < 8) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    }
                }
                else if (r == 100 && !isDesert && !isSnowy && !isVolcano && !isPolar) {
                    StructureGenerator::generateSmallHouse(this, x, h + 1, z);
                }
                else if (r == 101 && (isJungle || isVolcano)) {
                    StructureGenerator::generateRuins(this, x, h + 1, z);
                }
                else if (r < 30 && !isDesert && !isSnowy && !isPolar) set(x, h + 1, z, BLOCK_FLOWER_RED);
                else if (r < 40 && !isDesert && !isSnowy && !isPolar) set(x, h + 1, z, BLOCK_FLOWER_BLUE);
                else if (r < 100 && !isDesert && !isPolar) set(x, h + 1, z, BLOCK_TALL_GRASS);
            }
        }
    }

    generateCaves(noise, seed);
}

void Chunk::generateCaves(FastNoiseLite& noise, int seed) {
    if (!m_voxels) return;
    FastNoiseLite caveNoise;
    caveNoise.SetSeed(seed + 2);
    caveNoise.SetFrequency(0.05f);
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    for (int y = 0; y < SizeY; ++y) {
        for (int z = 0; z < SizeZ; ++z) {
            for (int x = 0; x < SizeX; ++x) {
                if (get(x, y, z) == BLOCK_STONE) {
                    float n = caveNoise.GetNoise((float)x, (float)y, (float)z);
                    if (n > 0.6f) {
                        set(x, y, z, BLOCK_AIR);
                        // Add lava at bottom of caves
                        if (y < 5) set(x, y, z, BLOCK_LAVA);
                    }
                }
            }
        }
    }
}
