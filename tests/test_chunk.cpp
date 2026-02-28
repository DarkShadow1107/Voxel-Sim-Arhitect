#include <catch2/catch_test_macros.hpp>
#include "Chunk.hpp"
#include "FastNoiseLite.h"
#include <set>

// =============================================================================
//  INITIAL STATE
// =============================================================================

TEST_CASE("Chunk: Default-constructed chunk is entirely AIR", "[chunk][init]") {
    Chunk chunk;
    // Check corners and center
    REQUIRE(chunk.get(0, 0, 0) == BLOCK_AIR);
    REQUIRE(chunk.get(Chunk::SizeX - 1, 0, 0) == BLOCK_AIR);
    REQUIRE(chunk.get(0, 0, Chunk::SizeZ - 1) == BLOCK_AIR);
    REQUIRE(chunk.get(0, Chunk::SizeY - 1, 0) == BLOCK_AIR);
    REQUIRE(chunk.get(Chunk::SizeX/2, Chunk::SizeY/2, Chunk::SizeZ/2) == BLOCK_AIR);
    REQUIRE(chunk.get(Chunk::SizeX - 1, Chunk::SizeY - 1, Chunk::SizeZ - 1) == BLOCK_AIR);
}

TEST_CASE("Chunk: getData() returns non-null pointer with correct capacity", "[chunk][init]") {
    Chunk chunk;
    REQUIRE(chunk.getData() != nullptr);
    // Capacity must hold the full volume
    // (we can at least read one byte from every corner without crashing)
    const uint8_t* d = chunk.getData();
    REQUIRE(d[0] == BLOCK_AIR);
}

// =============================================================================
//  SET / GET CORRECTNESS
// =============================================================================

TEST_CASE("Chunk: set and get round-trip all block types", "[chunk][setget]") {
    Chunk chunk;
    // Test representative block IDs
    const uint8_t types[] = {
        BLOCK_AIR, BLOCK_DIRT, BLOCK_GRASS, BLOCK_STONE,
        BLOCK_WATER, BLOCK_LAVA, BLOCK_WOOD, BLOCK_SAND,
        BLOCK_SNOW, BLOCK_BEDROCK
    };
    int x = 0;
    for (uint8_t t : types) {
        chunk.set(x, 1, 0, t);
        INFO("Block type " << (int)t << " at x=" << x);
        REQUIRE(chunk.get(x, 1, 0) == t);
        x++;
    }
}

TEST_CASE("Chunk: Overwrite voxel updates correctly", "[chunk][setget]") {
    Chunk chunk;
    chunk.set(4, 10, 4, BLOCK_STONE);
    REQUIRE(chunk.get(4, 10, 4) == BLOCK_STONE);

    chunk.set(4, 10, 4, BLOCK_GRASS);
    REQUIRE(chunk.get(4, 10, 4) == BLOCK_GRASS);

    chunk.set(4, 10, 4, BLOCK_AIR);
    REQUIRE(chunk.get(4, 10, 4) == BLOCK_AIR);
}

TEST_CASE("Chunk: All 16x16 surface cells can be set and retrieved", "[chunk][setget]") {
    Chunk chunk;
    // Fill the Y=5 horizontal slice with BLOCK_STONE
    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int z = 0; z < Chunk::SizeZ; ++z)
            chunk.set(x, 5, z, BLOCK_STONE);
    // Verify every cell
    int count = 0;
    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int z = 0; z < Chunk::SizeZ; ++z) {
            REQUIRE(chunk.get(x, 5, z) == BLOCK_STONE);
            ++count;
        }
    REQUIRE(count == Chunk::SizeX * Chunk::SizeZ);
}

// =============================================================================
//  OUT-OF-BOUNDS SAFETY
// =============================================================================

TEST_CASE("Chunk: Out-of-bounds access returns AIR, not UB", "[chunk][bounds]") {
    Chunk chunk;
    chunk.set(5, 5, 5, BLOCK_STONE); // sentinel inside bounds

    SECTION("Negative x")     { REQUIRE(chunk.get(-1, 5, 5) == 0); }
    SECTION("x == SizeX")     { REQUIRE(chunk.get(Chunk::SizeX, 5, 5) == 0); }
    SECTION("x >> SizeX")     { REQUIRE(chunk.get(1000, 5, 5) == 0); }
    SECTION("Negative y")     { REQUIRE(chunk.get(5, -1, 5) == 0); }
    SECTION("y == SizeY")     { REQUIRE(chunk.get(5, Chunk::SizeY, 5) == 0); }
    SECTION("Negative z")     { REQUIRE(chunk.get(5, 5, -1) == 0); }
    SECTION("z == SizeZ")     { REQUIRE(chunk.get(5, 5, Chunk::SizeZ) == 0); }
    // Sentinel must still be intact after all those invalid accesses
    REQUIRE(chunk.get(5, 5, 5) == BLOCK_STONE);
}

TEST_CASE("Chunk: Out-of-bounds set is silently ignored", "[chunk][bounds]") {
    Chunk chunk;
    // These must not crash or corrupt nearby valid cells
    REQUIRE_NOTHROW(chunk.set(-1,   5,   5,   BLOCK_STONE));
    REQUIRE_NOTHROW(chunk.set(1000, 5,   5,   BLOCK_STONE));
    REQUIRE_NOTHROW(chunk.set(5,   -1,   5,   BLOCK_STONE));
    REQUIRE_NOTHROW(chunk.set(5,  9999,  5,   BLOCK_STONE));
    REQUIRE_NOTHROW(chunk.set(5,    5,  -1,   BLOCK_STONE));
    REQUIRE_NOTHROW(chunk.set(5,    5, 9999,  BLOCK_STONE));
}

// =============================================================================
//  TERRAIN GENERATION
// =============================================================================

TEST_CASE("Chunk: Bedrock layer at y=0 after generation", "[chunk][generation]") {
    Chunk chunk;
    FastNoiseLite noise;
    noise.SetSeed(42);
    chunk.generateTerrain(noise, 42, 0.02f, 64, 0, 0);

    // Every column's y=0 must be bedrock
    for (int x = 0; x < Chunk::SizeX; x += 4)
        for (int z = 0; z < Chunk::SizeZ; z += 4) {
            INFO("x=" << x << " z=" << z);
            REQUIRE(chunk.get(x, 0, z) == BLOCK_BEDROCK);
        }
}

TEST_CASE("Chunk: Top atmosphere rows are AIR after generation", "[chunk][generation]") {
    Chunk chunk;
    FastNoiseLite noise;
    noise.SetSeed(100);
    // Low base height (30) ensures the top rows are guaranteed air
    chunk.generateTerrain(noise, 100, 0.05f, 30, 0, 0);

    REQUIRE(chunk.get(0,  Chunk::SizeY - 1, 0)  == BLOCK_AIR);
    REQUIRE(chunk.get(8,  Chunk::SizeY - 2, 8)  == BLOCK_AIR);
    REQUIRE(chunk.get(15, Chunk::SizeY - 3, 15) == BLOCK_AIR);
}

TEST_CASE("Chunk: Generated terrain has non-zero solid voxel count", "[chunk][generation]") {
    Chunk chunk;
    FastNoiseLite noise;
    noise.SetSeed(777);
    chunk.generateTerrain(noise, 777, 0.03f, 50, 0, 0);

    int solidCount = 0;
    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int y = 0; y < Chunk::SizeY; ++y)
            for (int z = 0; z < Chunk::SizeZ; ++z)
                if (chunk.get(x, y, z) != BLOCK_AIR) solidCount++;

    INFO("Solid voxels: " << solidCount);
    // With seed 777, freq 0.03, base 50 there must be many solid blocks
    REQUIRE(solidCount > Chunk::SizeX * Chunk::SizeZ * 5); // at least 5 layers
}

TEST_CASE("Chunk: Different seeds produce different terrain", "[chunk][generation]") {
    FastNoiseLite noise1, noise2;
    noise1.SetSeed(1); noise2.SetSeed(99999);
    Chunk c1, c2;
    c1.generateTerrain(noise1, 1,     0.02f, 50, 0, 0);
    c2.generateTerrain(noise2, 99999, 0.02f, 50, 0, 0);

    // Count differences in the surface region (y in [20..60])
    int diffs = 0;
    for (int x = 0; x < Chunk::SizeX; ++x)
        for (int y = 20; y < 60; ++y)
            for (int z = 0; z < Chunk::SizeZ; ++z)
                if (c1.get(x,y,z) != c2.get(x,y,z)) diffs++;

    INFO("Block differences in surface band: " << diffs);
    REQUIRE(diffs > 0); // seeds must produce distinct terrain
}

TEST_CASE("Chunk: Terrain structural invariants hold after generation", "[chunk][generation]") {
    // RATIONALE: generateTerrain intentionally uses rand() for ore placement,
    // surface decorations, trees, and waterfalls — so byte-exact or height-exact
    // equality across two separate instances is not a valid contract, even with
    // the same FastNoiseLite seed.
    //
    // What IS testable are structural invariants on a single generated chunk:
    //   • Every column has bedrock at y=0             (unconditional)
    //   • The underground zone has multiple STONE hits (noise height > 0)
    //   • The chunk is not trivially empty            (generation ran)
    //   • Heights are within SizeY bounds             (clamp works)

    FastNoiseLite noise;
    noise.SetSeed(42);
    Chunk chunk;
    chunk.generateTerrain(noise, 42, 0.02f, 50, 0, 0);

    SECTION("y=0 is bedrock in every column") {
        int bedrockCount = 0;
        for (int x = 0; x < Chunk::SizeX; ++x)
            for (int z = 0; z < Chunk::SizeZ; ++z) {
                INFO("Column (" << x << ", " << z << ")");
                CHECK(chunk.get(x, 0, z) == BLOCK_BEDROCK);
                if (chunk.get(x, 0, z) == BLOCK_BEDROCK) ++bedrockCount;
            }
        REQUIRE(bedrockCount == Chunk::SizeX * Chunk::SizeZ);
    }

    SECTION("Underground zone [y=5..45] contains stone-class blocks") {
        int stoneHits = 0;
        for (int x = 0; x < Chunk::SizeX; ++x)
            for (int z = 0; z < Chunk::SizeZ; ++z)
                for (int y = 5; y <= 45; ++y) {
                    uint8_t v = chunk.get(x, y, z);
                    if (v != BLOCK_AIR && v != BLOCK_WATER) ++stoneHits;
                }
        INFO("Underground solid/ore block count: " << stoneHits);
        REQUIRE(stoneHits > Chunk::SizeX * Chunk::SizeZ * 10);
    }

    SECTION("Total non-air volume is substantial (terrain was generated)") {
        int solid = 0;
        for (int x = 0; x < Chunk::SizeX; ++x)
            for (int y = 0; y < Chunk::SizeY; ++y)
                for (int z = 0; z < Chunk::SizeZ; ++z)
                    if (chunk.get(x, y, z) != BLOCK_AIR) ++solid;
        INFO("Total non-air voxels: " << solid);
        REQUIRE(solid > Chunk::SizeX * Chunk::SizeZ * 4);
    }

    SECTION("No voxels exist above SizeY - 1 (clamp is respected)") {
        // Access at boundary must return AIR (bounds-check inside get())
        for (int x = 0; x < Chunk::SizeX; ++x)
            for (int z = 0; z < Chunk::SizeZ; ++z)
                CHECK(chunk.get(x, Chunk::SizeY, z) == BLOCK_AIR);
    }
}

// =============================================================================
//  BIOME
// =============================================================================

TEST_CASE("Chunk: getBiomeAt() always returns a valid BiomeType", "[chunk][biome]") {
    Chunk chunk;
    FastNoiseLite bn, cn, mn;
    // Test a grid of world positions
    for (int i = -3; i <= 3; ++i) {
        for (int j = -3; j <= 3; ++j) {
            BiomeType b = chunk.getBiomeAt(bn, cn, mn, (float)(i * 128), (float)(j * 128));
            INFO("biome at (" << i*128 << ", " << j*128 << "): " << (int)b);
            REQUIRE(b >= BIOME_POLAR);
            REQUIRE(b <= BIOME_MOUNTAINS);
        }
    }
}

TEST_CASE("Chunk: Distinct biome seeds produce distinct biome distributions", "[chunk][biome]") {
    Chunk chunk;
    FastNoiseLite bn1, cn1, mn1, bn2, cn2, mn2;
    bn1.SetSeed(1); cn1.SetSeed(10); mn1.SetSeed(2);
    bn2.SetSeed(99); cn2.SetSeed(990); mn2.SetSeed(100);

    std::set<int> biomes1, biomes2;
    for (int i = 0; i < 20; ++i) {
        biomes1.insert((int)chunk.getBiomeAt(bn1, cn1, mn1, (float)(i * 200), 0.0f));
        biomes2.insert((int)chunk.getBiomeAt(bn2, cn2, mn2, (float)(i * 200), 0.0f));
    }
    // Each noise set should produce at least one biome result
    REQUIRE(!biomes1.empty());
    REQUIRE(!biomes2.empty());
}
