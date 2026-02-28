#include "BiomeRegistry.hpp"
#include "Chunk.hpp"

// ---------------------------------------------------------------------------
//  Biome data tables.
//  Array index == BiomeType enum value (BIOME_POLAR=0 … BIOME_MOUNTAINS=8).
// ---------------------------------------------------------------------------

// clang-format off
static const BiomeDef kBiomeDefs[] = {
    // name         surf              sub               hudRGB           fogRGB              treeF  flwrF  snow   cact
    { "Polar",      BLOCK_SNOW,       BLOCK_STONE,      210, 230, 255,   0.75f,0.82f,0.92f,  0.00f, 0.00f, true,  false },
    { "Snowy",      BLOCK_SNOW,       BLOCK_DIRT,       200, 220, 245,   0.70f,0.78f,0.90f,  0.04f, 0.01f, true,  false },
    { "Plains",     BLOCK_GRASS,      BLOCK_DIRT,       100, 225,  80,   0.55f,0.70f,0.90f,  0.05f, 0.08f, false, false },
    { "Savanna",    BLOCK_GRASS,      BLOCK_DIRT,       220, 195,  80,   0.80f,0.75f,0.55f,  0.02f, 0.03f, false, false },
    { "Desert",     BLOCK_SAND,       BLOCK_SAND,       240, 210, 100,   0.88f,0.78f,0.55f,  0.00f, 0.00f, false, true  },
    { "Jungle",     BLOCK_GRASS,      BLOCK_DIRT,        30, 185,  50,   0.45f,0.62f,0.40f,  0.25f, 0.05f, false, false },
    { "Ashworld",   BLOCK_ASH,        BLOCK_BASALT,     180,  90,  55,   0.30f,0.22f,0.20f,  0.00f, 0.00f, false, false },
    { "Ocean",      BLOCK_SAND,       BLOCK_SAND,        80, 140, 240,   0.20f,0.40f,0.75f,  0.00f, 0.00f, false, false },
    { "Mountains",  BLOCK_STONE,      BLOCK_STONE,      190, 195, 210,   0.60f,0.65f,0.75f,  0.03f, 0.01f, false, false },
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
