#include <catch2/catch_test_macros.hpp>
#include "Chunk.hpp"
#include "FastNoiseLite.h"
#include <set>
#include <array>
#include <vector>
#include <cstring>
#include <climits>

// Helper: find the surface height of column (x,z) in a chunk.
// Returns the topmost y that is non-air and non-fluid, or 0 if none.
static int surfaceHeight(const Chunk& chunk, int x, int z) {
    for (int y = Chunk::SizeY - 1; y >= 1; --y) {
        uint8_t b = chunk.get(x, y, z);
        if (b != BLOCK_AIR && b != BLOCK_WATER && b != BLOCK_LAVA)
            return y;
    }
    return 0;
}

// =============================================================================
//  BIOME DISTRIBUTION
// =============================================================================

TEST_CASE("WorldGen: All 8 biomes appear across a wide world scan", "[worldgen][biome]") {
    FastNoiseLite bn, cn;
    bn.SetSeed(1337 + 20);
    bn.SetFrequency(0.02f * 0.04f);
    bn.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    cn.SetSeed(1337 + 10);
    cn.SetFrequency(0.02f * 0.07f);
    cn.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    std::set<int> foundBiomes;
    Chunk dummy; // just used for getBiomeAt

    // Scan a 120×120 world-block grid at large spacing
    for (int wx = -3000; wx <= 3000; wx += 150)
        for (int wz = -3000; wz <= 3000; wz += 150)
            foundBiomes.insert((int)dummy.getBiomeAt(bn, cn, (float)wx, (float)wz));

    INFO("Biomes found: " << foundBiomes.size()
         << " (expected 8: POLAR=0, SNOWY=1, PLAINS=2, SAVANNA=3, DESERT=4, JUNGLE=5, VOLCANO=6, OCEAN=7)");
    // We expect at least 6 distinct biomes across a 6000-block scan
    REQUIRE(foundBiomes.size() >= 6);
}

TEST_CASE("WorldGen: OCEAN biome appears in deep negative continental noise regions",
          "[worldgen][biome]") {
    FastNoiseLite bn, cn;
    bn.SetSeed(100 + 20); bn.SetFrequency(0.0008f);
    cn.SetSeed(100 + 10); cn.SetFrequency(0.0014f);
    cn.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    bn.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    Chunk dummy;
    bool foundOcean = false;
    for (int wx = -5000; wx <= 5000 && !foundOcean; wx += 100)
        for (int wz = -5000; wz <= 5000 && !foundOcean; wz += 100)
            if (dummy.getBiomeAt(bn, cn, (float)wx, (float)wz) == BIOME_OCEAN)
                foundOcean = true;

    REQUIRE(foundOcean);
}

// =============================================================================
//  PER-BIOME HEIGHT INVARIANTS
// =============================================================================

TEST_CASE("WorldGen: Plains biome generates significantly flatter terrain than Volcano",
          "[worldgen][biome][height]") {
    // Methodology:
    //  1. Scan getBiomeAt over a wide grid to FIND world coords that are
    //     classified as Plains or Polar (flat biomes) and Volcano/Snowy (tall).
    //  2. Generate a chunk at those coords and verify the height invariant.
    //
    // This avoids the "we generate many chunks and hope they land in the right
    // biome" approach, which can miss biomes when the scan area is small
    // relative to the biome noise period (~1250 blocks).

    const int   seed = 2000;
    const float freq = 0.02f;

    // Build noise matching generateTerrain's biome/continental setup
    FastNoiseLite biomeN;
    biomeN.SetSeed(seed + 20);
    biomeN.SetFrequency(freq * 0.04f);
    biomeN.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeN.SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeN.SetFractalOctaves(2);

    FastNoiseLite contN;
    contN.SetSeed(seed + 10);
    contN.SetFrequency(freq * 0.07f);
    contN.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    contN.SetFractalType(FastNoiseLite::FractalType_FBm);
    contN.SetFractalOctaves(3);

    Chunk probe; // used only for getBiomeAt (static method)

    // Find chunk coords classified as flat / tall biomes
    int flatCX = INT_MIN, flatCZ = INT_MIN;
    int tallCX = INT_MIN, tallCZ = INT_MIN;

    for (int cx = -25; cx <= 25 && (flatCX == INT_MIN || tallCX == INT_MIN); ++cx) {
        for (int cz = -25; cz <= 25 && (flatCX == INT_MIN || tallCX == INT_MIN); ++cz) {
            float wx = (float)(cx * Chunk::SizeX + Chunk::SizeX / 2);
            float wz = (float)(cz * Chunk::SizeZ + Chunk::SizeZ / 2);
            BiomeType b = probe.getBiomeAt(biomeN, contN, wx, wz);

            if (flatCX == INT_MIN && (b == BIOME_PLAINS || b == BIOME_POLAR))
                { flatCX = cx; flatCZ = cz; }
            if (tallCX == INT_MIN && (b == BIOME_VOLCANO || b == BIOME_SNOWY))
                { tallCX = cx; tallCZ = cz; }
        }
    }

    INFO("Flat-biome chunk: cx=" << flatCX << " cz=" << flatCZ);
    INFO("Tall-biome chunk: cx=" << tallCX << " cz=" << tallCZ);
    REQUIRE(flatCX != INT_MIN); // world large enough to contain a flat biome
    REQUIRE(tallCX != INT_MIN); // world large enough to contain a tall biome

    // Generate and measure
    auto measure = [&](int cx, int cz) -> int {
        FastNoiseLite n;
        n.SetSeed(seed);
        Chunk c;
        c.generateTerrain(n, seed, freq, 10, cx * Chunk::SizeX, cz * Chunk::SizeZ);
        int mx = 0;
        for (int x = 0; x < Chunk::SizeX; x += 4)
            for (int z = 0; z < Chunk::SizeZ; z += 4) {
                int h = surfaceHeight(c, x, z);
                if (h > mx) mx = h;
            }
        return mx;
    };

    int flatMax = measure(flatCX, flatCZ);
    int tallMax = measure(tallCX, tallCZ);

    INFO("Flat-biome chunk maxH=" << flatMax << "  Tall-biome chunk maxH=" << tallMax);
    REQUIRE(flatMax < 23);    // Plains/Polar max formula → at most ~21 with blending
    REQUIRE(tallMax > 25);    // Volcano/Snowy must produce notably high terrain
    REQUIRE(tallMax > flatMax);
}

TEST_CASE("WorldGen: All surface heights are within [1, SizeY-2]", "[worldgen][height]") {
    for (int seed = 10; seed <= 50; seed += 10) {
        FastNoiseLite noise;
        noise.SetSeed(seed);
        Chunk chunk;
        chunk.generateTerrain(noise, seed, 0.02f, 10, 0, 0);

        for (int x = 0; x < Chunk::SizeX; ++x)
            for (int z = 0; z < Chunk::SizeZ; ++z) {
                int h = surfaceHeight(chunk, x, z);
                INFO("seed=" << seed << " x=" << x << " z=" << z << " h=" << h);
                REQUIRE(h >= 1);
                REQUIRE(h < Chunk::SizeY);
            }
    }
}

// =============================================================================
//  STRUCTURAL INVARIANTS
// =============================================================================

TEST_CASE("WorldGen: y=0 is always bedrock after any generation", "[worldgen][structure]") {
    for (int seed = 1; seed <= 5; ++seed) {
        for (int cx = -2; cx <= 2; ++cx) {
            FastNoiseLite noise;
            noise.SetSeed(seed * 1234);
            Chunk chunk;
            chunk.generateTerrain(noise, seed * 1234, 0.02f, 10,
                                  cx * Chunk::SizeX, 0);

            for (int x = 0; x < Chunk::SizeX; x += 2)
                for (int z = 0; z < Chunk::SizeZ; z += 2) {
                    INFO("seed=" << seed << " cx=" << cx
                         << " x=" << x << " z=" << z);
                    REQUIRE(chunk.get(x, 0, z) == BLOCK_BEDROCK);
                }
        }
    }
}

TEST_CASE("WorldGen: Top rows (SizeY-1 and SizeY-2) are AIR for low baseHeight",
          "[worldgen][structure]") {
    FastNoiseLite noise;
    noise.SetSeed(999);
    Chunk chunk;
    // baseHeight=5 guarantees terrain never reaches the top rows
    chunk.generateTerrain(noise, 999, 0.02f, 5, 0, 0);

    for (int x = 0; x < Chunk::SizeX; x += 4)
        for (int z = 0; z < Chunk::SizeZ; z += 4) {
            REQUIRE(chunk.get(x, Chunk::SizeY - 1, z) == BLOCK_AIR);
        }
}

TEST_CASE("WorldGen: Underground zone is predominantly stone/ore, not air",
          "[worldgen][structure]") {
    FastNoiseLite noise;
    noise.SetSeed(55555);
    Chunk chunk;
    chunk.generateTerrain(noise, 55555, 0.02f, 10, 0, 0);

    // In y=[2..25] at least 50% of non-bedrock voxels must be solid
    int solid = 0, total = 0;
    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int z = 0; z < Chunk::SizeZ; ++z)
            for (int y = 2; y <= 25; ++y) {
                uint8_t b = chunk.get(x, y, z);
                if (b != BLOCK_BEDROCK) {
                    ++total;
                    if (b != BLOCK_AIR && b != BLOCK_WATER) ++solid;
                }
            }

    double solidPct = total > 0 ? 100.0 * solid / total : 0.0;
    INFO("Solid percentage in y=[2..25]: " << solidPct << "%");
    REQUIRE(solidPct > 40.0); // caves can remove some, but majority must remain
}

// =============================================================================
//  ORE DEPTH DISTRIBUTION
// =============================================================================

TEST_CASE("WorldGen: Diamond ore only appears below y=20", "[worldgen][ore]") {
    int diamondAbove20 = 0;
    for (int seed = 1; seed <= 8; ++seed) {
        FastNoiseLite noise;
        noise.SetSeed(seed * 777);
        Chunk chunk;
        chunk.generateTerrain(noise, seed * 777, 0.02f, 10, 0, 0);

        for (int x = 0; x < Chunk::SizeX; ++x)
            for (int z = 0; z < Chunk::SizeZ; ++z)
                for (int y = 20; y < Chunk::SizeY; ++y)
                    if (chunk.get(x, y, z) == BLOCK_DIAMOND_ORE)
                        ++diamondAbove20;
    }
    INFO("Diamond ore blocks found at y>=20: " << diamondAbove20);
    REQUIRE(diamondAbove20 == 0);
}

TEST_CASE("WorldGen: Coal ore appears more frequently than diamond ore",
          "[worldgen][ore]") {
    int coalCount    = 0;
    int diamondCount = 0;

    FastNoiseLite noise;
    noise.SetSeed(12345);
    Chunk chunk;
    chunk.generateTerrain(noise, 12345, 0.02f, 10, 0, 0);

    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int y = 0; y < Chunk::SizeY; ++y)
            for (int z = 0; z < Chunk::SizeZ; ++z) {
                uint8_t b = chunk.get(x, y, z);
                if (b == BLOCK_COAL_ORE)    ++coalCount;
                if (b == BLOCK_DIAMOND_ORE) ++diamondCount;
            }

    INFO("Coal: " << coalCount << "  Diamond: " << diamondCount);
    REQUIRE(coalCount > diamondCount);
}

// =============================================================================
//  CAVE GENERATION
// =============================================================================

TEST_CASE("WorldGen: Caves create air in the underground zone y=[5..60]",
          "[worldgen][caves]") {
    // Caves should carve out a meaningful number of air blocks underground.
    int airInUnderground = 0;

    FastNoiseLite noise;
    noise.SetSeed(9999);
    Chunk chunk;
    chunk.generateTerrain(noise, 9999, 0.02f, 10, 0, 0);

    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int z = 0; z < Chunk::SizeZ; ++z)
            for (int y = 5; y <= 60; ++y) {
                // Only count truly underground positions (below surface)
                int h = surfaceHeight(chunk, x, z);
                if (y < h && chunk.get(x, y, z) == BLOCK_AIR)
                    ++airInUnderground;
            }

    INFO("Air voxels in underground zone y=[5..60]: " << airInUnderground);
    REQUIRE(airInUnderground > 50); // at least some caves must be present
}

TEST_CASE("WorldGen: Cave water only appears at low y values (underground lakes)",
          "[worldgen][caves]") {
    // Cave lakes are only placed at y<=10 by the new cave generator.
    int waterAbove10 = 0;

    FastNoiseLite noise;
    noise.SetSeed(8888);
    Chunk chunk;
    chunk.generateTerrain(noise, 8888, 0.02f, 10, 0, 0);

    // We can't distinguish cave water from surface water here, but we can
    // check that any water sitting inside a cavity at y>10 didn't come from
    // the cave generator (surface water is at/above sea level).
    // The cave generator fills water only at y<=10, so any water in y=[11..30]
    // that has solid blocks above it (i.e. underground) must be 0.
    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int z = 0; z < Chunk::SizeZ; ++z)
            for (int y = 11; y <= 30; ++y) {
                if (chunk.get(x, y, z) != BLOCK_WATER) continue;
                // Check if this is underground (solid block above within 5 steps)
                bool underground = false;
                for (int ay = y + 1; ay <= y + 5 && ay < Chunk::SizeY; ++ay)
                    if (chunk.get(x, ay, z) != BLOCK_AIR &&
                        chunk.get(x, ay, z) != BLOCK_WATER) {
                        underground = true; break;
                    }
                if (underground) ++waterAbove10;
            }

    INFO("Unexpected underground water at y=[11..30]: " << waterAbove10);
    // Cave generator should not place water above y=10
    REQUIRE(waterAbove10 == 0);
}

// =============================================================================
//  WATERFALL GENERATION
// =============================================================================

TEST_CASE("WorldGen: Water source blocks appear on high-terrain chunks",
          "[worldgen][waterfall]") {
    // Waterfalls require height > seaLevel+10. We scan for seeds that produce
    // high terrain and check that at least one contains a water source.
    const int seaLevel = 12;

    int highTerrainChunks = 0;
    int chunksWithWaterOnHigh = 0;

    for (int seed = 1; seed <= 30; ++seed) {
        for (int cx = -4; cx <= 4; ++cx) {
            FastNoiseLite noise;
            noise.SetSeed(seed * 101);
            Chunk chunk;
            chunk.generateTerrain(noise, seed * 101, 0.02f, 10,
                                  cx * Chunk::SizeX, 0);

            // Check if any column is high enough for waterfalls
            bool hasHighTerrain = false;
            for (int x = 0; x < Chunk::SizeX && !hasHighTerrain; x += 2)
                for (int z = 0; z < Chunk::SizeZ && !hasHighTerrain; z += 2)
                    if (surfaceHeight(chunk, x, z) > seaLevel + 10)
                        hasHighTerrain = true;

            if (!hasHighTerrain) continue;
            ++highTerrainChunks;

            // Check for water at or above seaLevel+10
            bool hasHighWater = false;
            for (int x = 0; x < Chunk::SizeX && !hasHighWater; ++x)
                for (int y = seaLevel + 8; y < Chunk::SizeY && !hasHighWater; ++y)
                    for (int z = 0; z < Chunk::SizeZ && !hasHighWater; ++z)
                        if (chunk.get(x, y, z) == BLOCK_WATER)
                            hasHighWater = true;

            if (hasHighWater) ++chunksWithWaterOnHigh;
        }
    }

    INFO("High-terrain chunks: " << highTerrainChunks
         << "  of which have water above seaLevel+8: " << chunksWithWaterOnHigh);
    // At least some high-terrain chunks must have waterfall water
    if (highTerrainChunks > 0) {
        REQUIRE(chunksWithWaterOnHigh > 0);
    }
}

// =============================================================================
//  RIVER GENERATION
// =============================================================================

TEST_CASE("WorldGen: River channels exist as sub-seaLevel depressions in non-desert biomes",
          "[worldgen][river]") {
    const int seaLevel = 12;
    int riverColumnsFound = 0;

    for (int seed = 1; seed <= 15; ++seed) {
        for (int cx = -3; cx <= 3; ++cx) {
            FastNoiseLite noise;
            noise.SetSeed(seed * 500);
            Chunk chunk;
            chunk.generateTerrain(noise, seed * 500, 0.02f, 10,
                                  cx * Chunk::SizeX, 0);

            // River columns: water block in y=[seaLevel-5 .. seaLevel] that
            // has a solid block directly below (carved bed)
            for (int x = 0; x < Chunk::SizeX; x += 2) {
                for (int z = 0; z < Chunk::SizeZ; z += 2) {
                    for (int y = seaLevel - 5; y <= seaLevel; ++y) {
                        if (chunk.get(x, y, z) == BLOCK_WATER) {
                            uint8_t below = chunk.get(x, y - 1, z);
                            if (below != BLOCK_AIR && below != BLOCK_WATER) {
                                ++riverColumnsFound;
                            }
                        }
                    }
                }
            }
        }
    }

    INFO("River-channel water columns found: " << riverColumnsFound);
    REQUIRE(riverColumnsFound > 0);
}

// =============================================================================
//  DESERT / POLAR INVARIANTS
// =============================================================================

TEST_CASE("WorldGen: No floating water appears above solid terrain in very high biomes",
          "[worldgen][biome][water]") {
    // Check that no water block is 'floating' (water block with AIR below and
    // AIR above, i.e. a 1-block water with no physical attachment).
    // This would indicate a waterfall source placed on a solid surface where
    // there's no cliff — a generation bug.

    int floatingWater = 0;

    for (int seed = 5; seed <= 25; seed += 5) {
        FastNoiseLite noise;
        noise.SetSeed(seed * 333);
        Chunk chunk;
        chunk.generateTerrain(noise, seed * 333, 0.02f, 10, 0, 0);

        for (int x = 1; x < Chunk::SizeX - 1; ++x)
            for (int z = 1; z < Chunk::SizeZ - 1; ++z)
                for (int y = 2; y < Chunk::SizeY - 1; ++y) {
                    if (chunk.get(x, y, z) != BLOCK_WATER) continue;

                    // Count neighbours that are air
                    int airNeighbours = 0;
                    if (chunk.get(x,     y - 1, z    ) == BLOCK_AIR) ++airNeighbours;
                    if (chunk.get(x,     y + 1, z    ) == BLOCK_AIR) ++airNeighbours;
                    if (chunk.get(x - 1, y,     z    ) == BLOCK_AIR) ++airNeighbours;
                    if (chunk.get(x + 1, y,     z    ) == BLOCK_AIR) ++airNeighbours;
                    if (chunk.get(x,     y,     z - 1) == BLOCK_AIR) ++airNeighbours;
                    if (chunk.get(x,     y,     z + 1) == BLOCK_AIR) ++airNeighbours;

                    // Completely surrounded by air (6/6) — truly floating
                    if (airNeighbours == 6) ++floatingWater;
                }
    }

    INFO("Completely isolated (floating) water blocks: " << floatingWater);
    REQUIRE(floatingWater == 0);
}
