#include "BiomeRegistry.hpp"
#include "Chunk.hpp"

// ---------------------------------------------------------------------------
//  Biome data tables.
//  Array index == BiomeType enum value (BIOME_POLAR=0 … BIOME_MOUNTAINS=8).
// ---------------------------------------------------------------------------

// clang-format off
static const BiomeDef kBiomeDefs[] = {
    // name          surf               sub                hudRGB           fogRGB               treeF  flwrF  snow   cact
    { "Polar",       BLOCK_SNOW,        BLOCK_STONE,       185, 215, 255,   0.80f,0.88f,0.96f,   0.00f, 0.00f, true,  false },
    { "Snowy Taiga", BLOCK_SNOW,        BLOCK_DIRT,        180, 210, 250,   0.68f,0.78f,0.92f,   0.06f, 0.01f, true,  false },
    { "Plains",      BLOCK_GRASS,       BLOCK_DIRT,        120, 195,  80,   0.62f,0.78f,0.52f,   0.05f, 0.08f, false, false },
    { "Savanna",     BLOCK_GRASS,       BLOCK_DIRT,        215, 195,  70,   0.82f,0.76f,0.52f,   0.02f, 0.04f, false, false },
    { "Desert",      BLOCK_SAND,        BLOCK_SAND,        245, 215,  95,   0.90f,0.80f,0.58f,   0.00f, 0.00f, false, true  },
    { "Jungle",      BLOCK_GRASS,       BLOCK_DIRT,         20, 190,  45,   0.40f,0.60f,0.38f,   0.30f, 0.06f, false, false },
    { "Ashworld",    BLOCK_ASH,         BLOCK_BASALT,      185,  88,  50,   0.28f,0.20f,0.18f,   0.00f, 0.00f, false, false },
    { "Ocean",       BLOCK_SAND,        BLOCK_SAND,         60, 130, 240,   0.18f,0.38f,0.78f,   0.00f, 0.00f, false, false },
    { "Mountains",   BLOCK_STONE,       BLOCK_STONE,       195, 200, 220,   0.58f,0.64f,0.76f,   0.04f, 0.02f, false, false },
};
// clang-format on

static constexpr int kBiomeCount =
    static_cast<int>(sizeof(kBiomeDefs) / sizeof(kBiomeDefs[0]));

// ---------------------------------------------------------------------------

const BiomeDef& BiomeRegistry::get(BiomeType biome) {
    int idx = static_cast<int>(biome);
    if (idx < 0 || idx >= kBiomeCount) idx = BIOME_PLAINS; // safe fallback
    return kBiomeDefs[idx];
}

const char* BiomeRegistry::name(BiomeType biome) {
    return get(biome).name;
}
