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

BiomeType Chunk::getBiomeAt(FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise, float wx, float wz) {
    if (continentalNoise.GetNoise(wx, wz) < -0.35f) return BIOME_OCEAN;
    
    float bn = biomeNoise.GetNoise(wx, wz);
    if (bn < -0.2f) return BIOME_POLAR;
    if (bn < 0.05f) return BIOME_SNOWY;
    if (bn < 0.35f) return BIOME_PLAINS;
    if (bn < 0.50f) return BIOME_SAVANNA;
    if (bn < 0.65f) return BIOME_DESERT;
    if (bn < 0.85f) return BIOME_JUNGLE;
    return BIOME_VOLCANO;
}

void Chunk::generateTerrain(FastNoiseLite& noise, int seed, float frequency, int baseHeight, int offsetX, int offsetZ) {
    if (!m_voxels) return;

    // -----------------------------------------------------------------------
    // Noise setup — multiple layers for realistic terrain
    // -----------------------------------------------------------------------

    // 1. Base terrain (medium-scale undulations)
    noise.SetSeed(seed);
    noise.SetFrequency(frequency);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFractalLacunarity(2.0f);
    noise.SetFractalGain(0.5f);

    // 2. Mountain noise — FBm for large rounded peaks
    FastNoiseLite mountainNoise;
    mountainNoise.SetSeed(seed + 1);
    mountainNoise.SetFrequency(frequency * 0.55f);
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainNoise.SetFractalOctaves(4);
    mountainNoise.SetFractalLacunarity(2.0f);
    mountainNoise.SetFractalGain(0.5f);

    // 3. Ridge noise — sharp mountain ridges and canyons
    FastNoiseLite ridgeNoise;
    ridgeNoise.SetSeed(seed + 3);
    ridgeNoise.SetFrequency(frequency * 0.55f);
    ridgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    ridgeNoise.SetFractalOctaves(3);
    ridgeNoise.SetFractalLacunarity(2.0f);
    ridgeNoise.SetFractalGain(0.4f);

    // 4. Detail noise — high-frequency micro-variation
    FastNoiseLite detailNoise;
    detailNoise.SetSeed(seed + 4);
    detailNoise.SetFrequency(frequency * 3.5f);
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // 5. River noise — low-frequency for wide river systems
    FastNoiseLite riverNoise;
    riverNoise.SetSeed(seed + 5);
    riverNoise.SetFrequency(std::max(0.0020f, frequency * 0.09f));
    riverNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // 6. Continental noise — determines ocean vs land at macro scale
    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(seed + 10);
    continentalNoise.SetFrequency(frequency * 0.07f);
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentalNoise.SetFractalOctaves(3);

    // 7. Biome noise — large-scale temperature/humidity
    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(seed + 20);
    biomeNoise.SetFrequency(frequency * 0.04f);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeNoise.SetFractalOctaves(2);

    // 8. 3D ore noise — vein-shaped ore placement
    FastNoiseLite oreNoise;
    oreNoise.SetSeed(seed + 50);
    oreNoise.SetFrequency(0.08f);
    oreNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    std::memset(m_voxels, 0, kVoxelCount);

    const int seaLevel = 12;

    // -----------------------------------------------------------------------
    // Biome boundary thresholds (must match getBiomeAt())
    // -----------------------------------------------------------------------
    // bn < -0.20 → POLAR, < 0.05 → SNOWY, < 0.35 → PLAINS,
    // < 0.50 → SAVANNA, < 0.65 → DESERT, < 0.85 → JUNGLE, else VOLCANO
    static constexpr float kBiomeThr[] = { -0.20f, 0.05f, 0.35f, 0.50f, 0.65f, 0.85f };
    static constexpr float kBlendR     = 0.05f;  // blend radius at biome borders

    for (int z = 0; z < SizeZ; ++z) {
        for (int x = 0; x < SizeX; ++x) {
            float wx = (float)(x + offsetX);
            float wz = (float)(z + offsetZ);

            // Sample noise layers
            const float n   = noise.GetNoise(wx, wz);            // base  [-1,1]
            const float mn  = mountainNoise.GetNoise(wx, wz);    // mountain
            const float rn  = ridgeNoise.GetNoise(wx, wz);       // ridge [0,1] (Ridged)
            const float dn  = detailNoise.GetNoise(wx, wz);      // detail
            const float cn  = continentalNoise.GetNoise(wx, wz); // continental
            const float bn  = biomeNoise.GetNoise(wx, wz);       // biome

            // Normalise to [0,1]
            const float h01 = (n  + 1.0f) * 0.5f;
            const float m01 = (mn + 1.0f) * 0.5f;
            const float r01 = rn;               // Ridged is already [0,1]
            const float d01 = (dn + 1.0f) * 0.5f;

            BiomeType biome = getBiomeAt(biomeNoise, continentalNoise, wx, wz);
            const bool isPolar   = (biome == BIOME_POLAR);
            const bool isSnowy   = (biome == BIOME_SNOWY);
            const bool isPlains  = (biome == BIOME_PLAINS);
            const bool isSavanna = (biome == BIOME_SAVANNA);
            const bool isDesert  = (biome == BIOME_DESERT);
            const bool isJungle  = (biome == BIOME_JUNGLE);
            const bool isVolcano = (biome == BIOME_VOLCANO);
            const bool isOcean   = (biome == BIOME_OCEAN);

            // ------------------------------------------------------------------
            // Per-biome height function (float for smooth blending)
            // ------------------------------------------------------------------
            float fh;
            const float bh = (float)baseHeight;

            if (isOcean) {
                // Continental shelf: deeper farther from coast
                float seaDepth = (std::abs(cn + 0.35f)) * 30.0f + 2.0f;
                fh = std::max(1.0f, (float)seaLevel - seaDepth + d01 * 2.0f);
            } else if (isVolcano) {
                // Dramatic cone: base hill + ridge peak
                float coneH  = m01 * 100.0f;
                float peakH  = r01 * 30.0f;
                fh = bh + 20.0f + coneH + peakH;
                fh = std::min(fh, (float)(SizeY - 10));
            } else if (isPolar) {
                // Nearly flat, slight undulation
                fh = bh + h01 * 4.0f + d01 * 1.5f;
            } else if (isSnowy) {
                // Rolling hills with occasional ridges
                fh = bh + h01 * 14.0f + r01 * 12.0f * std::max(0.0f, m01 - 0.55f) + d01 * 2.0f;
            } else if (isPlains) {
                // Very flat — only detail noise contributes
                fh = bh + h01 * 2.5f + d01 * 1.5f;
            } else if (isSavanna) {
                // Flat plains punctuated by steep mesa formations
                float mesaRaw  = std::max(0.0f, m01 - 0.70f) / 0.30f; // 0..1
                float mesaH    = mesaRaw * mesaRaw * 28.0f;            // squared for sharp edge
                // Hard cap the mesa top so it's table-flat
                if (mesaH > 14.0f) mesaH = 14.0f + (mesaH - 14.0f) * 0.15f;
                fh = bh + h01 * 5.0f + mesaH + d01 * 1.5f;
            } else if (isDesert) {
                // Dune-like: base + ridge noise for crests
                fh = bh + h01 * 8.0f + r01 * 7.0f * m01 + d01 * 2.0f;
            } else if (isJungle) {
                // Lush hilly terrain, moderate ridges
                fh = bh + h01 * 16.0f + r01 * 15.0f * std::max(0.0f, m01 - 0.40f) + d01 * 2.5f;
            } else {
                // Generic land (fallback)
                fh = bh + h01 * 14.0f + d01 * 2.0f;
            }

            // ------------------------------------------------------------------
            // Biome transition blending — smooth-step toward a neutral height
            // when we're close to any biome boundary
            // ------------------------------------------------------------------
            if (!isOcean) {
                float neutralH = bh + h01 * 10.0f; // neutral blended height
                for (float thr : kBiomeThr) {
                    float dist = std::abs(bn - thr);
                    if (dist < kBlendR) {
                        float t = dist / kBlendR;                      // 0 near boundary, 1 away
                        t = t * t * (3.0f - 2.0f * t);                // smoothstep
                        fh = fh * t + neutralH * (1.0f - t);
                        break;
                    }
                }

                // Continental coastal push-down: ease land to seaLevel near ocean edge
                if (cn < 0.08f && cn > -0.35f) {
                    float coastT = std::clamp((cn + 0.35f) / 0.43f, 0.0f, 1.0f); // 0=ocean,1=land
                    coastT = coastT * coastT;  // ease in
                    float coastH = (float)seaLevel + 1.0f;
                    fh = fh * coastT + coastH * (1.0f - coastT);
                }
            }

            // ------------------------------------------------------------------
            // River carving — blend height toward river bed
            // ------------------------------------------------------------------
            if (!isDesert && !isPolar && !isOcean) {
                const float rv   = std::abs(riverNoise.GetNoise(wx, wz));
                const float rW   = 0.060f;
                const float rStr = std::clamp((rW - rv) / rW, 0.0f, 1.0f);
                if (rStr > 0.0f) {
                    const float riverBedH = (float)(seaLevel - 3);
                    fh = fh * (1.0f - rStr) + riverBedH * rStr;
                }
            }

            int h = std::clamp((int)fh, 1, SizeY - 1);

            // ------------------------------------------------------------------
            // Fill voxel column
            // ------------------------------------------------------------------
            for (int y = 0; y < SizeY; ++y) {
                if (y == 0) {
                    set(x, y, z, BLOCK_BEDROCK);
                    continue;
                }

                if (y > h) {
                    // Air above surface — ocean/polar fill handled below
                    if (y < seaLevel) {
                        if (isPolar) set(x, y, z, (y >= seaLevel - 1) ? BLOCK_ICE : BLOCK_WATER);
                        else         set(x, y, z, BLOCK_WATER);
                    }
                    continue;
                }

                // y <= h (solid)
                uint8_t type = BLOCK_STONE;

                if (y == h) {
                    // Surface block
                    if (isOcean) {
                        type = (h <= 3) ? BLOCK_STONE : BLOCK_SAND;
                    } else if (isPolar) {
                        type = (y <= seaLevel) ? BLOCK_ICE : BLOCK_SNOW;
                    } else if (isSnowy) {
                        type = (y <= seaLevel + 1) ? BLOCK_SAND : BLOCK_GRASS;
                        if (y > 90) type = BLOCK_SNOW;
                    } else if (isDesert) {
                        type = BLOCK_SAND;
                    } else if (isSavanna) {
                        // Mesa top: stone/sandstone strata; low ground: grass/sand
                        float mesaRaw = std::max(0.0f, m01 - 0.70f) / 0.30f;
                        if (mesaRaw > 0.5f) type = (((y / 3) % 2 == 0)) ? BLOCK_SAND : BLOCK_STONE;
                        else                type = (y <= seaLevel + 1) ? BLOCK_SAND : BLOCK_GRASS;
                    } else if (isJungle) {
                        type = (y <= seaLevel + 1) ? BLOCK_SAND : BLOCK_GRASS;
                    } else if (isVolcano) {
                        type = (y > 72) ? BLOCK_STONE : BLOCK_GRASS;
                    } else {
                        // Default temperate
                        if      (y > 110)            type = BLOCK_SNOW;
                        else if (y <= seaLevel + 2)  type = BLOCK_SAND;
                        else                         type = BLOCK_GRASS;
                    }
                } else if (y == h - 1 || y == h - 2 || y == h - 3) {
                    // Sub-surface layer
                    if (isDesert || isOcean) {
                        type = BLOCK_SAND;
                    } else if (isPolar) {
                        type = BLOCK_STONE;
                    } else if (isSavanna) {
                        float mesaRaw = std::max(0.0f, m01 - 0.70f) / 0.30f;
                        type = (mesaRaw > 0.5f) ? BLOCK_STONE : BLOCK_DIRT;
                    } else {
                        type = BLOCK_DIRT;
                    }
                } else {
                    // Deep underground — ore veins via 3D noise
                    float ov = oreNoise.GetNoise((float)(x + offsetX), (float)y, (float)(z + offsetZ));
                    float oa = std::abs(ov);

                    if        (y <  20 && oa > 0.72f) { type = BLOCK_DIAMOND_ORE; }
                    else if   (y <  40 && oa > 0.68f) { type = BLOCK_GOLD_ORE;    }
                    else if   (y <  65 && oa > 0.64f) { type = BLOCK_IRON_ORE;    }
                    else if   (y < 100 && oa > 0.60f) { type = BLOCK_COAL_ORE;    }
                    else                              { type = BLOCK_STONE;        }

                    // Volcano deep core: obsidian veins above y=52
                    if (isVolcano && y > 52 && oa > 0.65f) type = BLOCK_OBSIDIAN;
                }

                set(x, y, z, type);
            }

            // ------------------------------------------------------------------
            // Volcano crater — pre-fill lava (static, flows when disturbed)
            // ------------------------------------------------------------------
            if (isVolcano && h > 85) {
                float cdx = (float)x - (float)SizeX * 0.5f;
                float cdz = (float)z - (float)SizeZ * 0.5f;
                float distSq = cdx * cdx + cdz * cdz;
                // Narrow cone inner crater
                if (distSq < 20.0f) {
                    for (int y = h; y > h - 22 && y > 0; --y)
                        set(x, y, z, BLOCK_LAVA);
                }
            }

            // ------------------------------------------------------------------
            // Flora & structures
            // ------------------------------------------------------------------
            if (h > seaLevel && h < 100) {
                int r = rand() % 2000;
                float treeN = noise.GetNoise(wx * 0.08f, wz * 0.08f);

                if (r < 18) {
                    if (isJungle) {
                        if (r < 16) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    } else if (isDesert) {
                        if (r < 5)  StructureGenerator::generateCactus(this, x, h + 1, z);
                    } else if (isSavanna) {
                        float mesaRaw = std::max(0.0f, m01 - 0.70f) / 0.30f;
                        if (r < 5 && mesaRaw < 0.4f)
                            StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    } else if (isSnowy || isPolar) {
                        if (r < 7) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_SNOW);
                    } else if (!isPlains || r < 5) {
                        if      (treeN > 0.45f)  StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_CHERRY_WOOD, BLOCK_CHERRY_LEAVES);
                        else if (treeN < -0.45f) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_BIRCH_WOOD,  BLOCK_BIRCH_LEAVES);
                        else if (r < 10)         StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD,        BLOCK_LEAVES);
                    }
                } else if (r == 100 && !isDesert && !isSnowy && !isVolcano && !isPolar) {
                    StructureGenerator::generateSmallHouse(this, x, h + 1, z);
                } else if (r == 102 && isPlains) {
                    StructureGenerator::generateVillage(this, x, h + 1, z);
                } else if (r == 101 && (isJungle || isVolcano)) {
                    StructureGenerator::generateRuins(this, x, h + 1, z);
                } else if (r < 30 && !isDesert && !isSnowy && !isPolar) {
                    set(x, h + 1, z, BLOCK_FLOWER_RED);
                } else if (r < 45 && !isDesert && !isSnowy && !isPolar) {
                    set(x, h + 1, z, BLOCK_FLOWER_BLUE);
                } else if (r < 110 && !isDesert && !isPolar) {
                    set(x, h + 1, z, BLOCK_TALL_GRASS);
                } else if ((isSnowy || isPolar) && r < 1400) {
                    set(x, h + 1, z, BLOCK_SNOW_LAYER);
                }
            }
        }
    }

    generateCaves(noise, seed);
    generateWaterbodies(noise, seed, frequency, baseHeight, offsetX, offsetZ);
}

void Chunk::generateCaves(FastNoiseLite& noise, int seed) {
    if (!m_voxels) return;

    // -----------------------------------------------------------------------
    // Three overlapping cave layers to create varied underground geometry:
    //   Layer 1 — Large open caverns  (FBm, low freq, large blobs)
    //   Layer 2 — Narrow tunnels      (FBm range-threshold → tube cross-section)
    //   Layer 3 — Worm passages       (different axis orientation)
    // -----------------------------------------------------------------------

    // Layer 1: Large caverns
    FastNoiseLite cavernNoise;
    cavernNoise.SetSeed(seed + 2);
    cavernNoise.SetFrequency(0.035f);
    cavernNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    cavernNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    cavernNoise.SetFractalOctaves(2);
    cavernNoise.SetFractalLacunarity(2.0f);
    cavernNoise.SetFractalGain(0.5f);

    // Layer 2: Narrow tunnels — tube shape from double-sided threshold
    FastNoiseLite tunnelNoise;
    tunnelNoise.SetSeed(seed + 7);
    tunnelNoise.SetFrequency(0.055f);
    tunnelNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    tunnelNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    tunnelNoise.SetFractalOctaves(2);

    // Layer 3: Worm passages (different orientation via y/z swap)
    FastNoiseLite wormNoise;
    wormNoise.SetSeed(seed + 13);
    wormNoise.SetFrequency(0.060f);
    wormNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    for (int y = 1; y < SizeY - 1; ++y) {
        // Caverns become more common deeper underground
        float cavernThr = (y < 40) ? 0.52f : 0.58f;
        // Tunnels disabled above y=80 to preserve surface terrain
        bool tunnelsActive = (y < 80);

        for (int z = 0; z < SizeZ; ++z) {
            for (int x = 0; x < SizeX; ++x) {
                uint8_t cur = get(x, y, z);
                // Only carve stone (preserve ores, dirt, bedrock, etc.)
                if (cur != BLOCK_STONE && cur != BLOCK_DIRT) continue;

                // Layer 1 — large cavern blob
                float cv = cavernNoise.GetNoise((float)x, (float)y, (float)z);
                if (cv > cavernThr) {
                    set(x, y, z, BLOCK_AIR);
                    // Underground lake: fill carved voids at/below water table with water
                    if (y <= 10 && y > 1)
                        set(x, y, z, BLOCK_WATER);
                    continue;
                }

                if (!tunnelsActive) continue;

                // Layer 2 — narrow tunnel (tube via range test)
                float tv = tunnelNoise.GetNoise((float)x, (float)y, (float)z);
                float ta = std::abs(tv);
                if (ta > 0.62f && ta < 0.76f) {
                    set(x, y, z, BLOCK_AIR);
                    if (y <= 10 && y > 1)
                        set(x, y, z, BLOCK_WATER);
                    continue;
                }

                // Layer 3 — worm passage (swap y↔z axis for perpendicular tunnels)
                float wv = wormNoise.GetNoise((float)x, (float)z, (float)y);
                float wa = std::abs(wv);
                if (wa > 0.65f && wa < 0.78f) {
                    set(x, y, z, BLOCK_AIR);
                    if (y <= 10 && y > 1)
                        set(x, y, z, BLOCK_WATER);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// generateWaterbodies — second-pass after terrain+caves
//
// Pre-fills rivers, highland lakes, waterfalls, desert oases and volcano lava
// so they appear immediately on generation.  These blocks are STATIC — they
// only start simulating when the player interacts near them (like Minecraft).
//
// 1. River water      : fills river channels with BLOCK_WATER up to seaLevel.
// 2. Highland lake    : fills enclosed depressions above seaLevel.
// 3. Waterfall        : detects cliff edges, pre-fills falling column.
// 4. Desert oasis     : small water holes in desert terrain.
// 5. Volcano lava     : lava source blocks at high-peak columns.
// ---------------------------------------------------------------------------
void Chunk::generateWaterbodies(FastNoiseLite& noise, int seed, float frequency,
                                int /*baseHeight*/, int offsetX, int offsetZ) {
    if (!m_voxels) return;

    const int seaLevel = 12;

    FastNoiseLite riverNoise;
    riverNoise.SetSeed(seed + 5);
    riverNoise.SetFrequency(std::max(0.0020f, frequency * 0.09f));
    riverNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(seed + 10);
    continentalNoise.SetFrequency(frequency * 0.07f);
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentalNoise.SetFractalOctaves(3);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(seed + 20);
    biomeNoise.SetFrequency(frequency * 0.04f);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeNoise.SetFractalOctaves(2);

    FastNoiseLite oasisNoise;
    oasisNoise.SetSeed(seed + 60);
    oasisNoise.SetFrequency(frequency * 1.5f);
    oasisNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Pre-compute surface height (topmost non-air/non-fluid block) per column
    int heights[SizeX][SizeZ] = {};
    for (int z2 = 0; z2 < SizeZ; ++z2)
        for (int x2 = 0; x2 < SizeX; ++x2)
            for (int y = SizeY - 1; y >= 1; --y) {
                uint8_t b = get(x2, y, z2);
                if (b != BLOCK_AIR && b != BLOCK_WATER && b != BLOCK_LAVA) {
                    heights[x2][z2] = y; break;
                }
            }

    const int ddx[4] = {1, -1, 0, 0};
    const int ddz[4] = {0, 0, 1, -1};

    for (int z = 0; z < SizeZ; ++z) {
        for (int x = 0; x < SizeX; ++x) {
            float wx = (float)(x + offsetX);
            float wz = (float)(z + offsetZ);

            BiomeType biome = getBiomeAt(biomeNoise, continentalNoise, wx, wz);
            const bool isDesert  = (biome == BIOME_DESERT);
            const bool isPolar   = (biome == BIOME_POLAR);
            const bool isOcean   = (biome == BIOME_OCEAN);
            const bool isVolcano = (biome == BIOME_VOLCANO);

            const int h = heights[x][z];

            // ------------------------------------------------------------------
            // 1. River water — fill channel from carved bed up to seaLevel
            // ------------------------------------------------------------------
            if (!isDesert && !isPolar && !isOcean) {
                const float rv   = std::abs(riverNoise.GetNoise(wx, wz));
                const float rW   = 0.060f;
                const float rStr = std::clamp((rW - rv) / rW, 0.0f, 1.0f);
                if (rStr > 0.0f && h > 0 && h < seaLevel) {
                    for (int wy = h + 1; wy <= seaLevel && wy < SizeY; ++wy)
                        set(x, wy, z, BLOCK_WATER);
                }
            }

            // ------------------------------------------------------------------
            // 2. Highland lake — fill enclosed depressions above seaLevel
            //    A column is a candidate if it is lower than ALL 4 neighbours.
            // ------------------------------------------------------------------
            if (!isDesert && !isPolar && !isOcean && h > seaLevel + 2 && h < 60) {
                bool isDepression = true;
                int  minNeighbor  = SizeY;
                for (int d = 0; d < 4; ++d) {
                    int nx2 = x + ddx[d], nz2 = z + ddz[d];
                    if (nx2 < 0 || nx2 >= SizeX || nz2 < 0 || nz2 >= SizeZ) {
                        isDepression = false; break;
                    }
                    int nh = heights[nx2][nz2];
                    if (nh <= h) { isDepression = false; break; }
                    if (nh < minNeighbor) minNeighbor = nh;
                }
                if (isDepression) {
                    int lakeTop = std::min(minNeighbor - 1, h + 3);
                    for (int wy = h + 1; wy <= lakeTop && wy < SizeY; ++wy)
                        set(x, wy, z, BLOCK_WATER);
                }
            }

            // ------------------------------------------------------------------
            // 3. Waterfall — source block on cliff top + pre-filled falling column
            //    so the waterfall is visible immediately on chunk load.
            // ------------------------------------------------------------------
            if (!isDesert && !isPolar && !isOcean && !isVolcano && h > seaLevel + 10) {
                int maxDrop = 0, dropDX = 0, dropDZ = 0;
                for (int d = 0; d < 4; ++d) {
                    int nx2 = x + ddx[d], nz2 = z + ddz[d];
                    if (nx2 < 0 || nx2 >= SizeX || nz2 < 0 || nz2 >= SizeZ) continue;
                    int drop = h - heights[nx2][nz2];
                    if (drop > maxDrop) { maxDrop = drop; dropDX = ddx[d]; dropDZ = ddz[d]; }
                }

                if (maxDrop >= 5) {
                    float wfN = noise.GetNoise(wx * 4.0f, wz * 4.0f);
                    if (wfN > 0.35f) {
                        // True source block atop the cliff
                        if (h + 1 < SizeY) set(x, h + 1, z, BLOCK_WATER);

                        // Pre-fill cliff-face column (AIR blocks only) so fall is
                        // visible immediately — gravity fires before horizontal
                        // spread so there is no cascade/flood.
                        const int cliffX  = x + dropDX;
                        const int cliffZ  = z + dropDZ;
                        const int colTop  = h;
                        const int colLen  = std::min(maxDrop - 1, 10);
                        const int colBot  = colTop - colLen + 1;
                        for (int fy = colTop; fy >= colBot; --fy) {
                            if (fy >= 1 && fy < SizeY &&
                                get(cliffX, fy, cliffZ) == BLOCK_AIR)
                                set(cliffX, fy, cliffZ, BLOCK_WATER);
                        }
                    }
                }
            }

            // ------------------------------------------------------------------
            // 4. Desert oasis — small water hole surrounded by sand
            // ------------------------------------------------------------------
            if (isDesert && h > seaLevel + 2 && h < 40) {
                float ov = oasisNoise.GetNoise(wx, wz);
                if (ov > 0.80f) {
                    // Replace the top surface block with water
                    if (h > 0 && h < SizeY)
                        set(x, h, z, BLOCK_WATER);
                }
            }

            // ------------------------------------------------------------------
            // 5. Volcano lava source at high-peak columns
            // ------------------------------------------------------------------
            if (isVolcano && h > 98) {
                float lvN = noise.GetNoise(wx * 2.0f, wz * 2.0f);
                if (lvN > 0.82f && h + 1 < SizeY)
                    set(x, h + 1, z, BLOCK_LAVA);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Schedule fluid updates for adjacent-to-air fluid blocks.
    //
    // OCEAN water is NEVER scheduled — it is fully static on load.
    // The setBlock() wake-up handles activation when a player digs adjacent.
    // Rivers, lakes, waterfalls, and lava sources ARE scheduled.
    // -----------------------------------------------------------------------
    BiomeType colBiome[SizeX][SizeZ];
    for (int z2 = 0; z2 < SizeZ; ++z2)
        for (int x2 = 0; x2 < SizeX; ++x2)
            colBiome[x2][z2] = getBiomeAt(biomeNoise, continentalNoise,
                                           (float)(x2 + offsetX), (float)(z2 + offsetZ));

    m_fluidUpdates.clear();
    for (int y = 1; y < SizeY - 1; ++y) {
        for (int z = 0; z < SizeZ; ++z) {
            for (int x = 0; x < SizeX; ++x) {
                uint8_t b = get(x, y, z);
                if (b != BLOCK_WATER && b != BLOCK_LAVA) continue;

                // Ocean water stays static
                if (b == BLOCK_WATER && colBiome[x][z] == BIOME_OCEAN) continue;

                // Schedule only if adjacent to air (otherwise it's fully enclosed)
                bool needsUpdate = false;
                if (                             get(x,     y - 1, z    ) == BLOCK_AIR) needsUpdate = true;
                else if (x > 0           &&      get(x - 1, y,     z    ) == BLOCK_AIR) needsUpdate = true;
                else if (x < SizeX - 1   &&      get(x + 1, y,     z    ) == BLOCK_AIR) needsUpdate = true;
                else if (z > 0           &&      get(x,     y,     z - 1) == BLOCK_AIR) needsUpdate = true;
                else if (z < SizeZ - 1   &&      get(x,     y,     z + 1) == BLOCK_AIR) needsUpdate = true;

                if (needsUpdate)
                    m_fluidUpdates.push_back((x << 16) | (y << 8) | z);
            }
        }
    }
}
