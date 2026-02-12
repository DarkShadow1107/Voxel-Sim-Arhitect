#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

struct WorldGenSettings {
    // Preset name
    char name[64] = "Default";

    // --- Terrain ---
    int   seed             = 12345;
    float frequency        = 0.01f;
    int   octaves          = 6;
    float lacunarity       = 2.0f;
    float gain             = 0.5f;
    float baseHeight       = 64.0f;
    float terrainAmplitude = 48.0f;
    float terrainScale     = 1.0f;

    // --- Biomes ---
    float biomeScale           = 256.0f;
    float temperatureInfluence = 0.5f;
    float moistureInfluence    = 0.5f;
    float biomeBlend           = 8.0f;

    // --- Mountains ---
    float mountainFrequency = 0.005f;
    float mountainHeight    = 80.0f;
    float mountainSharpness = 2.0f;
    float mountainSnowLine  = 120.0f;

    // --- Volcanos ---
    int   volcanoCount   = 2;
    float volcanoRadius  = 40.0f;
    float volcanoHeight  = 90.0f;
    float lavaDepth      = 10.0f;
    float eruptionChance = 0.05f;

    // --- Lakes & Water ---
    int   lakeCount      = 8;
    float lakeMinSize    = 10.0f;
    float lakeMaxSize    = 40.0f;
    float lakeDepth      = 8.0f;
    float lakeShoreWidth = 3.0f;
    float oceanLevel     = 62.0f;
    float riverWidth     = 4.0f;
    float riverFrequency = 0.002f;

    // --- Caves ---
    float caveFrequency     = 0.04f;
    float caveThreshold     = 0.55f;
    float caveScale         = 1.0f;
    float oreFrequency      = 0.02f;
    float stalactiteDensity = 0.1f;

    // --- Vegetation ---
    float treeDensity   = 0.3f;
    float grassDensity  = 0.6f;
    float flowerDensity = 0.1f;
    int   treeHeightMin = 5;
    int   treeHeightMax = 12;
    float bushDensity   = 0.15f;

    // --- Structures ---
    float villageFrequency   = 0.001f;
    float dungeonFrequency   = 0.005f;
    float mineshaftFrequency = 0.003f;
    float templeFrequency    = 0.0008f;

    // --- Erosion ---
    int   erosionIterations = 50000;
    float erosionStrength   = 0.3f;
    float sedimentCapacity  = 4.0f;
    float thermalErosion    = 0.01f;
};

class WorldEditor {
public:
    void show(bool* open);

private:
    // Initialization
    bool m_initialized = false;
    void initDefaults();

    // Tab state
    int m_selectedTab = 0;

    // Search filter
    char m_searchFilter[64] = "";

    // Presets
    std::vector<WorldGenSettings> m_presets;
    int m_activePresetIdx = 0;

    // Undo / Redo
    std::vector<WorldGenSettings> m_undoStack;
    std::vector<WorldGenSettings> m_redoStack;
    static constexpr int kMaxUndo = 50;
    bool m_dirty = false;

    void pushUndo();
    void undo();
    void redo();

    // Save / Load presets to file
    void savePresetsToFile(const char* path) const;
    bool loadPresetsFromFile(const char* path);
    static constexpr const char* kPresetsFile = "world_presets.dat";

    // Terrain preview generation
    void generatePreviewHeights(const WorldGenSettings& s, float* out, int count) const;

    // Save notification
    float m_saveNotifyTimer = 0.0f;

    // Clipboard
    bool m_hasClipboard = false;
    WorldGenSettings m_clipboard{};
};
