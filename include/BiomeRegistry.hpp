#pragma once
// ---------------------------------------------------------------------------
//  BiomeRegistry — central data table for all biome properties.
//
//  TDD §1 specification:
//    "BiomeRegistry.h / .cpp — Stores data tables for block types, flora,
//     and weather per biome."
//
//  Usage:
//    const BiomeDef& def = BiomeRegistry::get(BIOME_PLAINS);
//    uint8_t surf = def.surfaceBlock;
//    const char* name = def.name;
//    // imgui HUD colour: IM_COL32(def.hudR, def.hudG, def.hudB, 220)
// ---------------------------------------------------------------------------
#include <cstdint>
#include "Chunk.hpp"   // BiomeType enum

struct BiomeDef {
    const char* name;            // Display name shown in HUD

    // ── Surface block types ────────────────────────────────────────────────
    uint8_t surfaceBlock;        // Top-most non-fluid block
    uint8_t subSurfaceBlock;     // y=h-1, y=h-2, y=h-3 block

    // ── HUD colour (RGBA bytes 0-255) ─────────────────────────────────────
    uint8_t hudR, hudG, hudB;    // Biome label colour in the in-world HUD

    // ── Fog / sky tint ─────────────────────────────────────────────────────
    float fogR, fogG, fogB;      // Linear 0..1 fog colour to blend into render

    // ── Flora parameters ─────────────────────────────────────────────────
    float treeFreq;              // 0..1 probability per surface cell of a tree
    float flowerFreq;            // 0..1 probability per surface cell of a flower
    bool  hasSnowLayer;          // Whether a BLOCK_SNOW_LAYER is placed on surface
    bool  hasCacti;              // Whether cacti generate here
};

class BiomeRegistry {
public:
    // Returns the BiomeDef for `biome`. If biome is out of range returns
    // the PLAINS fallback definition.
    static const BiomeDef& get(BiomeType biome);

    // Convenience: return just the display name.
    static const char* name(BiomeType biome);
};
