#include "WorldEditor.hpp"
#include "imgui.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

// ============================================================================
// Default presets
// ============================================================================

static WorldGenSettings makeDefaultPreset() {
    WorldGenSettings s{};
    strncpy(s.name, "Default", 63);
    s.seed = 12345; s.frequency = 0.01f; s.octaves = 6;
    s.lacunarity = 2.0f; s.gain = 0.5f; s.baseHeight = 64.0f;
    s.terrainAmplitude = 48.0f; s.terrainScale = 1.0f;
    s.biomeScale = 256.0f; s.temperatureInfluence = 0.5f;
    s.moistureInfluence = 0.5f; s.biomeBlend = 8.0f;
    s.mountainFrequency = 0.005f; s.mountainHeight = 80.0f;
    s.mountainSharpness = 2.0f; s.mountainSnowLine = 120.0f;
    s.volcanoCount = 2; s.volcanoRadius = 40.0f;
    s.volcanoHeight = 90.0f; s.lavaDepth = 10.0f; s.eruptionChance = 0.05f;
    s.lakeCount = 8; s.lakeMinSize = 10.0f; s.lakeMaxSize = 40.0f;
    s.lakeDepth = 8.0f; s.lakeShoreWidth = 3.0f; s.oceanLevel = 62.0f;
    s.riverWidth = 4.0f; s.riverFrequency = 0.002f;
    s.caveFrequency = 0.04f; s.caveThreshold = 0.55f; s.caveScale = 1.0f;
    s.oreFrequency = 0.02f; s.stalactiteDensity = 0.1f;
    s.treeDensity = 0.3f; s.grassDensity = 0.6f; s.flowerDensity = 0.1f;
    s.treeHeightMin = 5; s.treeHeightMax = 12; s.bushDensity = 0.15f;
    s.villageFrequency = 0.001f; s.dungeonFrequency = 0.005f;
    s.mineshaftFrequency = 0.003f; s.templeFrequency = 0.0008f;
    s.erosionIterations = 50000; s.erosionStrength = 0.3f;
    s.sedimentCapacity = 4.0f; s.thermalErosion = 0.01f;
    return s;
}

static WorldGenSettings makeFlatPreset() {
    WorldGenSettings s{};
    strncpy(s.name, "Flat World", 63);
    s.seed = 12345; s.frequency = 0.0f; s.octaves = 1;
    s.lacunarity = 1.0f; s.gain = 0.0f; s.baseHeight = 4.0f;
    s.terrainAmplitude = 0.0f; s.terrainScale = 1.0f;
    s.biomeScale = 512.0f; s.temperatureInfluence = 0.0f;
    s.moistureInfluence = 0.0f; s.biomeBlend = 0.0f;
    s.mountainFrequency = 0.0f; s.mountainHeight = 0.0f;
    s.mountainSharpness = 1.0f; s.mountainSnowLine = 200.0f;
    s.volcanoCount = 0; s.volcanoRadius = 0.0f;
    s.volcanoHeight = 0.0f; s.lavaDepth = 0.0f; s.eruptionChance = 0.0f;
    s.lakeCount = 0; s.lakeMinSize = 0.0f; s.lakeMaxSize = 0.0f;
    s.lakeDepth = 0.0f; s.lakeShoreWidth = 0.0f; s.oceanLevel = -1.0f;
    s.riverWidth = 0.0f; s.riverFrequency = 0.0f;
    s.caveFrequency = 0.0f; s.caveThreshold = 1.0f; s.caveScale = 1.0f;
    s.oreFrequency = 0.0f; s.stalactiteDensity = 0.0f;
    s.treeDensity = 0.0f; s.grassDensity = 0.8f; s.flowerDensity = 0.05f;
    s.treeHeightMin = 5; s.treeHeightMax = 8; s.bushDensity = 0.0f;
    s.villageFrequency = 0.0f; s.dungeonFrequency = 0.0f;
    s.mineshaftFrequency = 0.0f; s.templeFrequency = 0.0f;
    s.erosionIterations = 0; s.erosionStrength = 0.0f;
    s.sedimentCapacity = 0.0f; s.thermalErosion = 0.0f;
    return s;
}

static WorldGenSettings makeAmplifiedPreset() {
    WorldGenSettings s{};
    strncpy(s.name, "Amplified", 63);
    s.seed = 12345; s.frequency = 0.008f; s.octaves = 8;
    s.lacunarity = 2.2f; s.gain = 0.6f; s.baseHeight = 64.0f;
    s.terrainAmplitude = 128.0f; s.terrainScale = 1.5f;
    s.biomeScale = 200.0f; s.temperatureInfluence = 0.7f;
    s.moistureInfluence = 0.6f; s.biomeBlend = 12.0f;
    s.mountainFrequency = 0.01f; s.mountainHeight = 180.0f;
    s.mountainSharpness = 3.0f; s.mountainSnowLine = 150.0f;
    s.volcanoCount = 4; s.volcanoRadius = 50.0f;
    s.volcanoHeight = 140.0f; s.lavaDepth = 15.0f; s.eruptionChance = 0.08f;
    s.lakeCount = 12; s.lakeMinSize = 15.0f; s.lakeMaxSize = 60.0f;
    s.lakeDepth = 12.0f; s.lakeShoreWidth = 5.0f; s.oceanLevel = 62.0f;
    s.riverWidth = 6.0f; s.riverFrequency = 0.003f;
    s.caveFrequency = 0.05f; s.caveThreshold = 0.5f; s.caveScale = 1.2f;
    s.oreFrequency = 0.03f; s.stalactiteDensity = 0.15f;
    s.treeDensity = 0.4f; s.grassDensity = 0.7f; s.flowerDensity = 0.15f;
    s.treeHeightMin = 6; s.treeHeightMax = 18; s.bushDensity = 0.2f;
    s.villageFrequency = 0.001f; s.dungeonFrequency = 0.008f;
    s.mineshaftFrequency = 0.005f; s.templeFrequency = 0.001f;
    s.erosionIterations = 80000; s.erosionStrength = 0.4f;
    s.sedimentCapacity = 6.0f; s.thermalErosion = 0.015f;
    return s;
}

static WorldGenSettings makeIslandsPreset() {
    WorldGenSettings s{};
    strncpy(s.name, "Islands", 63);
    s.seed = 12345; s.frequency = 0.007f; s.octaves = 5;
    s.lacunarity = 2.0f; s.gain = 0.45f; s.baseHeight = 40.0f;
    s.terrainAmplitude = 35.0f; s.terrainScale = 0.8f;
    s.biomeScale = 180.0f; s.temperatureInfluence = 0.6f;
    s.moistureInfluence = 0.8f; s.biomeBlend = 10.0f;
    s.mountainFrequency = 0.003f; s.mountainHeight = 50.0f;
    s.mountainSharpness = 1.5f; s.mountainSnowLine = 200.0f;
    s.volcanoCount = 1; s.volcanoRadius = 30.0f;
    s.volcanoHeight = 70.0f; s.lavaDepth = 8.0f; s.eruptionChance = 0.03f;
    s.lakeCount = 3; s.lakeMinSize = 5.0f; s.lakeMaxSize = 20.0f;
    s.lakeDepth = 5.0f; s.lakeShoreWidth = 4.0f; s.oceanLevel = 58.0f;
    s.riverWidth = 3.0f; s.riverFrequency = 0.001f;
    s.caveFrequency = 0.03f; s.caveThreshold = 0.6f; s.caveScale = 0.8f;
    s.oreFrequency = 0.015f; s.stalactiteDensity = 0.08f;
    s.treeDensity = 0.5f; s.grassDensity = 0.7f; s.flowerDensity = 0.2f;
    s.treeHeightMin = 4; s.treeHeightMax = 10; s.bushDensity = 0.25f;
    s.villageFrequency = 0.0005f; s.dungeonFrequency = 0.003f;
    s.mineshaftFrequency = 0.002f; s.templeFrequency = 0.0005f;
    s.erosionIterations = 40000; s.erosionStrength = 0.35f;
    s.sedimentCapacity = 3.0f; s.thermalErosion = 0.02f;
    return s;
}

static WorldGenSettings makeMesaPreset() {
    WorldGenSettings s{};
    strncpy(s.name, "Mesa", 63);
    s.seed = 12345; s.frequency = 0.012f; s.octaves = 4;
    s.lacunarity = 1.8f; s.gain = 0.35f; s.baseHeight = 72.0f;
    s.terrainAmplitude = 30.0f; s.terrainScale = 1.2f;
    s.biomeScale = 300.0f; s.temperatureInfluence = 0.9f;
    s.moistureInfluence = 0.1f; s.biomeBlend = 4.0f;
    s.mountainFrequency = 0.008f; s.mountainHeight = 60.0f;
    s.mountainSharpness = 4.0f; s.mountainSnowLine = 200.0f;
    s.volcanoCount = 0; s.volcanoRadius = 0.0f;
    s.volcanoHeight = 0.0f; s.lavaDepth = 0.0f; s.eruptionChance = 0.0f;
    s.lakeCount = 2; s.lakeMinSize = 5.0f; s.lakeMaxSize = 15.0f;
    s.lakeDepth = 4.0f; s.lakeShoreWidth = 2.0f; s.oceanLevel = -1.0f;
    s.riverWidth = 2.0f; s.riverFrequency = 0.001f;
    s.caveFrequency = 0.03f; s.caveThreshold = 0.6f; s.caveScale = 0.9f;
    s.oreFrequency = 0.025f; s.stalactiteDensity = 0.05f;
    s.treeDensity = 0.02f; s.grassDensity = 0.1f; s.flowerDensity = 0.01f;
    s.treeHeightMin = 3; s.treeHeightMax = 6; s.bushDensity = 0.05f;
    s.villageFrequency = 0.0005f; s.dungeonFrequency = 0.004f;
    s.mineshaftFrequency = 0.006f; s.templeFrequency = 0.001f;
    s.erosionIterations = 70000; s.erosionStrength = 0.5f;
    s.sedimentCapacity = 2.0f; s.thermalErosion = 0.03f;
    return s;
}

static WorldGenSettings makeVolcanicPreset() {
    WorldGenSettings s{};
    strncpy(s.name, "Volcanic", 63);
    s.seed = 12345; s.frequency = 0.009f; s.octaves = 5;
    s.lacunarity = 2.1f; s.gain = 0.55f; s.baseHeight = 50.0f;
    s.terrainAmplitude = 60.0f; s.terrainScale = 1.3f;
    s.biomeScale = 200.0f; s.temperatureInfluence = 0.8f;
    s.moistureInfluence = 0.2f; s.biomeBlend = 6.0f;
    s.mountainFrequency = 0.012f; s.mountainHeight = 100.0f;
    s.mountainSharpness = 2.5f; s.mountainSnowLine = 180.0f;
    s.volcanoCount = 8; s.volcanoRadius = 55.0f;
    s.volcanoHeight = 120.0f; s.lavaDepth = 20.0f; s.eruptionChance = 0.15f;
    s.lakeCount = 4; s.lakeMinSize = 8.0f; s.lakeMaxSize = 25.0f;
    s.lakeDepth = 6.0f; s.lakeShoreWidth = 2.0f; s.oceanLevel = 45.0f;
    s.riverWidth = 3.0f; s.riverFrequency = 0.001f;
    s.caveFrequency = 0.06f; s.caveThreshold = 0.45f; s.caveScale = 1.3f;
    s.oreFrequency = 0.04f; s.stalactiteDensity = 0.2f;
    s.treeDensity = 0.05f; s.grassDensity = 0.15f; s.flowerDensity = 0.02f;
    s.treeHeightMin = 3; s.treeHeightMax = 7; s.bushDensity = 0.03f;
    s.villageFrequency = 0.0002f; s.dungeonFrequency = 0.007f;
    s.mineshaftFrequency = 0.004f; s.templeFrequency = 0.0005f;
    s.erosionIterations = 30000; s.erosionStrength = 0.2f;
    s.sedimentCapacity = 5.0f; s.thermalErosion = 0.005f;
    return s;
}

// ============================================================================
// Initialization
// ============================================================================

void WorldEditor::initDefaults() {
    m_presets.clear();
    m_presets.push_back(makeDefaultPreset());
    m_presets.push_back(makeFlatPreset());
    m_presets.push_back(makeAmplifiedPreset());
    m_presets.push_back(makeIslandsPreset());
    m_presets.push_back(makeMesaPreset());
    m_presets.push_back(makeVolcanicPreset());
    m_activePresetIdx = 0;
    m_undoStack.clear();
    m_redoStack.clear();
    m_dirty = false;

    // Try loading saved presets from disk
    loadPresetsFromFile(kPresetsFile);
}

// ============================================================================
// Undo / Redo
// ============================================================================

void WorldEditor::pushUndo() {
    if (m_activePresetIdx < 0 || m_activePresetIdx >= (int)m_presets.size()) return;
    m_undoStack.push_back(m_presets[m_activePresetIdx]);
    if ((int)m_undoStack.size() > kMaxUndo)
        m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
    m_dirty = true;
}

void WorldEditor::undo() {
    if (m_undoStack.empty()) return;
    if (m_activePresetIdx < 0 || m_activePresetIdx >= (int)m_presets.size()) return;
    m_redoStack.push_back(m_presets[m_activePresetIdx]);
    m_presets[m_activePresetIdx] = m_undoStack.back();
    m_undoStack.pop_back();
    m_dirty = !m_undoStack.empty();
}

void WorldEditor::redo() {
    if (m_redoStack.empty()) return;
    if (m_activePresetIdx < 0 || m_activePresetIdx >= (int)m_presets.size()) return;
    m_undoStack.push_back(m_presets[m_activePresetIdx]);
    m_presets[m_activePresetIdx] = m_redoStack.back();
    m_redoStack.pop_back();
    m_dirty = true;
}

// ============================================================================
// Save / Load presets
// ============================================================================

void WorldEditor::savePresetsToFile(const char* path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return;
    uint32_t count = (uint32_t)m_presets.size();
    out.write((const char*)&count, sizeof(count));
    for (auto& p : m_presets) {
        out.write((const char*)&p, sizeof(WorldGenSettings));
    }
}

bool WorldEditor::loadPresetsFromFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    uint32_t count = 0;
    in.read((char*)&count, sizeof(count));
    if (!in || count == 0 || count > 100) return false;
    std::vector<WorldGenSettings> loaded;
    for (uint32_t i = 0; i < count; i++) {
        WorldGenSettings s{};
        in.read((char*)&s, sizeof(WorldGenSettings));
        if (!in) return false;
        loaded.push_back(s);
    }
    m_presets = loaded;
    if (m_activePresetIdx >= (int)m_presets.size()) m_activePresetIdx = 0;
    return true;
}

// ============================================================================
// Preview terrain generation (simplified noise approximation)
// ============================================================================

void WorldEditor::generatePreviewHeights(const WorldGenSettings& s, float* out, int count) const {
    for (int i = 0; i < count; i++) {
        float x = (float)i * s.frequency * s.terrainScale * 3.0f;
        float val = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float maxAmp = 0.0f;
        for (int o = 0; o < s.octaves; o++) {
            float t = x * freq + s.seed * 0.137f;
            val += (std::sin(t * 1.7f + std::cos(t * 0.83f)) * 0.5f +
                    std::sin(t * 0.53f + 1.3f) * 0.3f +
                    std::cos(t * 2.1f - 0.7f) * 0.2f) * amp;
            maxAmp += amp;
            freq *= s.lacunarity;
            amp *= s.gain;
        }
        if (maxAmp > 0.0f) val /= maxAmp;

        // Add mountain influence
        float mx = (float)i * s.mountainFrequency * 5.0f + s.seed * 0.31f;
        float mountainVal = std::max(0.0f, std::sin(mx * 0.7f + std::cos(mx * 0.4f)));
        mountainVal = std::pow(mountainVal, s.mountainSharpness);
        float mountainContrib = mountainVal * s.mountainHeight / 256.0f;

        // Add volcano peaks
        float volcanoContrib = 0.0f;
        for (int v = 0; v < s.volcanoCount && v < 5; v++) {
            float vCenter = (float)count * (0.15f + 0.7f * (float)v / std::max(1, s.volcanoCount - 1));
            float vDist = std::abs((float)i - vCenter);
            float vRadius = s.volcanoRadius * (float)count / 400.0f;
            if (vDist < vRadius) {
                float vShape = 1.0f - vDist / vRadius;
                vShape = std::pow(vShape, 1.5f);
                float vPeak = vShape * s.volcanoHeight / 256.0f;
                // Crater dip
                if (vDist < vRadius * 0.15f) {
                    float craterDip = 1.0f - vDist / (vRadius * 0.15f);
                    vPeak -= craterDip * s.lavaDepth / 256.0f;
                }
                volcanoContrib = std::max(volcanoContrib, vPeak);
            }
        }

        float height = s.baseHeight / 256.0f + val * s.terrainAmplitude / 256.0f +
                        mountainContrib + volcanoContrib;
        out[i] = std::clamp(height, 0.0f, 1.0f);
    }
}

// ============================================================================
// Main show function
// ============================================================================

void WorldEditor::show(bool* open) {
    if (!*open) return;
    if (!m_initialized) {
        initDefaults();
        m_initialized = true;
    }

    ImGui::SetNextWindowSize(ImVec2(1100, 750), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("World Editor", open)) {
        ImGui::End();
        return;
    }

    // Save notification
    if (m_saveNotifyTimer > 0.0f) {
        m_saveNotifyTimer -= ImGui::GetIO().DeltaTime;
        float alpha = std::min(1.0f, m_saveNotifyTimer * 2.0f);
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        const char* msg = "Saved!";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        float nx = winPos.x + winSize.x - ts.x - 20;
        float ny = winPos.y + 8;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(nx - 8, ny - 4), ImVec2(nx + ts.x + 8, ny + ts.y + 4),
            IM_COL32(40, 120, 60, (int)(alpha * 200)), 4.0f);
        ImGui::GetWindowDrawList()->AddText(ImVec2(nx, ny),
            IM_COL32(255, 255, 255, (int)(alpha * 255)), msg);
    }

    // Keyboard shortcuts
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && m_dirty) {
        savePresetsToFile(kPresetsFile);
        m_dirty = false;
        m_saveNotifyTimer = 2.0f;
    }

    // Validate active preset index
    if (m_activePresetIdx < 0 || m_activePresetIdx >= (int)m_presets.size())
        m_activePresetIdx = 0;
    if (m_presets.empty()) { ImGui::End(); return; }

    // ========================================================================
    // Toolbar: Save / Discard / Undo / Redo
    // ========================================================================
    {
        ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Save", ImVec2(70, 28)) && m_dirty) {
            savePresetsToFile(kPresetsFile);
            m_dirty = false;
            m_saveNotifyTimer = 2.0f;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save all presets to disk (Ctrl+S)");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.7f, 0.3f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Discard", ImVec2(70, 28)) && m_dirty) {
            initDefaults();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Discard all unsaved changes and reload");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_undoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
        if (ImGui::Button("Undo", ImVec2(55, 28)) && !m_undoStack.empty()) undo();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Undo (Ctrl+Z)");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_redoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
        if (ImGui::Button("Redo", ImVec2(55, 28)) && !m_redoStack.empty()) redo();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Redo (Ctrl+Y)");
        ImGui::PopStyleColor();

        if (m_dirty) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " * Unsaved Changes");
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 220);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Preset: %s", m_presets[m_activePresetIdx].name);
    }
    ImGui::Separator();

    // ========================================================================
    // Left panel: Preset list
    // ========================================================================
    ImGui::BeginChild("##WE_PresetList", ImVec2(180, 0), true);
    {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "PRESETS");
        ImGui::Separator();

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##presetSearch", "Search...", m_searchFilter, sizeof(m_searchFilter));
        ImGui::Spacing();

        for (int i = 0; i < (int)m_presets.size(); i++) {
            // Apply search filter
            if (m_searchFilter[0] != '\0') {
                std::string nameLower = m_presets[i].name;
                std::string filterLower = m_searchFilter;
                for (auto& c : nameLower) c = (char)tolower(c);
                for (auto& c : filterLower) c = (char)tolower(c);
                if (nameLower.find(filterLower) == std::string::npos) continue;
            }
            ImGui::PushID(i);
            bool sel = (m_activePresetIdx == i);
            if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.28f, 0.56f, 1.0f, 0.5f));
            if (ImGui::Selectable(m_presets[i].name, sel)) {
                if (m_activePresetIdx != i) {
                    m_undoStack.clear();
                    m_redoStack.clear();
                }
                m_activePresetIdx = i;
            }
            if (sel) ImGui::PopStyleColor();
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("+ Add Preset", ImVec2(-1, 28))) {
            WorldGenSettings newPreset = makeDefaultPreset();
            char newName[64];
            snprintf(newName, 64, "Custom %d", (int)m_presets.size() + 1);
            strncpy(newPreset.name, newName, 63);
            m_presets.push_back(newPreset);
            m_activePresetIdx = (int)m_presets.size() - 1;
            m_dirty = true;
        }

        if (ImGui::Button("Duplicate", ImVec2(-1, 24))) {
            WorldGenSettings copy = m_presets[m_activePresetIdx];
            char dupName[64];
            snprintf(dupName, 64, "%s Copy", copy.name);
            strncpy(copy.name, dupName, 63);
            m_presets.push_back(copy);
            m_activePresetIdx = (int)m_presets.size() - 1;
            m_dirty = true;
        }

        // Copy / Paste
        float halfW = ImGui::GetContentRegionAvail().x * 0.5f - 2;
        if (ImGui::Button("Copy", ImVec2(halfW, 22))) {
            m_clipboard = m_presets[m_activePresetIdx];
            m_hasClipboard = true;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, m_hasClipboard ? ImVec4(0.3f, 0.6f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Paste", ImVec2(-1, 22)) && m_hasClipboard) {
            pushUndo();
            char keepName[64];
            strncpy(keepName, m_presets[m_activePresetIdx].name, 63);
            m_presets[m_activePresetIdx] = m_clipboard;
            strncpy(m_presets[m_activePresetIdx].name, keepName, 63);
        }
        ImGui::PopStyleColor();

        if ((int)m_presets.size() > 1) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("- Delete", ImVec2(-1, 24))) {
                m_presets.erase(m_presets.begin() + m_activePresetIdx);
                if (m_activePresetIdx >= (int)m_presets.size())
                    m_activePresetIdx = (int)m_presets.size() - 1;
                m_undoStack.clear();
                m_redoStack.clear();
                m_dirty = true;
            }
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // ========================================================================
    // Right panel: Tabbed editor
    // ========================================================================
    ImGui::BeginChild("##WE_RightSide", ImVec2(0, 0), false);
    {
        auto& s = m_presets[m_activePresetIdx];
        WorldGenSettings preEdit = s;

        // Tab bar
        const char* tabNames[] = {
            "Terrain", "Biomes", "Mountains & Volcanos", "Water Bodies",
            "Caves & Ores", "Vegetation", "Structures", "Erosion", "Presets"
        };
        const int tabCount = 9;

        if (ImGui::BeginTabBar("##WorldEditorTabs")) {
            for (int t = 0; t < tabCount; t++) {
                if (ImGui::BeginTabItem(tabNames[t])) {
                    m_selectedTab = t;
                    ImGui::Spacing();

                    // ========================================================
                    // TAB 0: Terrain
                    // ========================================================
                    if (t == 0) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "TERRAIN GENERATION");
                        ImGui::TextDisabled("Configure the base terrain noise and shape parameters.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragInt("Seed", &s.seed, 1.0f, 0, 999999);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("World generation seed. Same seed produces identical worlds.");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Randomize")) {
                            s.seed = std::rand() % 999999;
                        }

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Noise Parameters");
                        ImGui::Separator();

                        ImGui::DragFloat("Frequency", &s.frequency, 0.0005f, 0.001f, 0.1f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Base noise frequency. Lower = larger features, higher = more chaotic.");

                        ImGui::DragInt("Octaves", &s.octaves, 0.1f, 1, 8);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Number of noise layers. More octaves add finer detail.");
                        s.octaves = std::clamp(s.octaves, 1, 8);

                        ImGui::DragFloat("Lacunarity", &s.lacunarity, 0.01f, 1.0f, 4.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frequency multiplier per octave. Higher values add more fine detail.");

                        ImGui::DragFloat("Gain", &s.gain, 0.005f, 0.0f, 1.0f, "%.3f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Amplitude multiplier per octave. Controls the contribution of each layer.");

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Terrain Shape");
                        ImGui::Separator();

                        ImGui::DragFloat("Base Height", &s.baseHeight, 0.5f, 1.0f, 256.0f, "%.1f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Average ground level Y coordinate.");

                        ImGui::DragFloat("Terrain Amplitude", &s.terrainAmplitude, 0.5f, 0.0f, 200.0f, "%.1f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum height deviation from base height.");

                        ImGui::DragFloat("Terrain Scale", &s.terrainScale, 0.01f, 0.1f, 5.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overall horizontal scale. Values > 1 stretch features larger.");

                        // Terrain preview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Terrain Cross-Section Preview");
                        ImGui::TextDisabled("Approximate visualization of terrain with current settings.");
                        ImGui::Separator();
                        {
                            const int previewCount = 400;
                            float heights[previewCount];
                            generatePreviewHeights(s, heights, previewCount);
                            ImGui::PlotLines("##TerrainPreview", heights, previewCount, 0,
                                nullptr, 0.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 120));

                            // Water level indicator text
                            float waterNorm = s.oceanLevel / 256.0f;
                            ImGui::TextDisabled("Water level: %.0f (%.0f%%)", s.oceanLevel, waterNorm * 100.0f);
                        }
                    }

                    // ========================================================
                    // TAB 1: Biomes
                    // ========================================================
                    if (t == 1) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "BIOME CONFIGURATION");
                        ImGui::TextDisabled("Control biome distribution, blending, and climate influence.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Biome Scale", &s.biomeScale, 1.0f, 32.0f, 1024.0f, "%.0f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Average biome region size. Larger values create bigger biome zones.");

                        ImGui::DragFloat("Temperature Influence", &s.temperatureInfluence, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How strongly temperature affects biome selection. 0 = no effect, 1 = full effect.");

                        ImGui::DragFloat("Moisture Influence", &s.moistureInfluence, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How strongly moisture/rainfall affects biome selection.");

                        ImGui::DragFloat("Biome Blend", &s.biomeBlend, 0.1f, 0.0f, 32.0f, "%.1f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Width of the smooth transition between biomes. 0 = hard edges.");

                        // Biome color preview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Biome Types");
                        ImGui::TextDisabled("Visual reference for biome color mapping.");
                        ImGui::Separator();

                        struct BiomeInfo { const char* name; ImVec4 color; const char* desc; };
                        BiomeInfo biomes[] = {
                            {"Plains",    ImVec4(0.5f, 0.8f, 0.3f, 1.0f),  "Flat grasslands with moderate vegetation"},
                            {"Forest",    ImVec4(0.2f, 0.6f, 0.15f, 1.0f), "Dense tree coverage with varied foliage"},
                            {"Desert",    ImVec4(0.9f, 0.8f, 0.3f, 1.0f),  "Arid sand with cacti and dead bushes"},
                            {"Tundra",    ImVec4(0.7f, 0.8f, 0.9f, 1.0f),  "Frozen terrain with sparse vegetation"},
                            {"Jungle",    ImVec4(0.1f, 0.7f, 0.2f, 1.0f),  "Dense tropical vegetation with tall trees"},
                            {"Swamp",     ImVec4(0.3f, 0.5f, 0.2f, 1.0f),  "Waterlogged terrain with vines and lily pads"},
                            {"Mountains", ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "High altitude with steep cliffs and snow"},
                            {"Savanna",   ImVec4(0.8f, 0.7f, 0.3f, 1.0f),  "Dry grasslands with acacia trees"},
                            {"Taiga",     ImVec4(0.15f,0.4f, 0.2f, 1.0f),  "Cold forest biome with spruce trees"},
                            {"Mushroom",  ImVec4(0.6f, 0.2f, 0.5f, 1.0f),  "Rare biome covered in mycelium"},
                        };

                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        for (int bi = 0; bi < 10; bi++) {
                            ImVec2 pos = ImGui::GetCursorScreenPos();
                            ImU32 col = IM_COL32((int)(biomes[bi].color.x*255), (int)(biomes[bi].color.y*255),
                                                  (int)(biomes[bi].color.z*255), 255);
                            dl->AddRectFilled(pos, ImVec2(pos.x + 16, pos.y + 16), col, 3.0f);
                            dl->AddRect(pos, ImVec2(pos.x + 16, pos.y + 16), IM_COL32(200,200,200,100), 3.0f);
                            ImGui::Dummy(ImVec2(16, 16));
                            ImGui::SameLine();
                            ImGui::TextColored(biomes[bi].color, "%s", biomes[bi].name);
                            ImGui::SameLine(200);
                            ImGui::TextDisabled("%s", biomes[bi].desc);
                        }
                    }

                    // ========================================================
                    // TAB 2: Mountains & Volcanos
                    // ========================================================
                    if (t == 2) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "MOUNTAIN GENERATION");
                        ImGui::TextDisabled("Configure mountain ranges and their surface features.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Mountain Frequency", &s.mountainFrequency, 0.0005f, 0.0f, 0.05f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How often mountain ranges appear. 0 = no mountains.");

                        ImGui::DragFloat("Mountain Height", &s.mountainHeight, 1.0f, 0.0f, 256.0f, "%.0f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum mountain peak height above base terrain.");

                        ImGui::DragFloat("Mountain Sharpness", &s.mountainSharpness, 0.02f, 0.5f, 6.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Peak sharpness. Higher = sharper, more dramatic peaks.");

                        ImGui::DragFloat("Snow Line", &s.mountainSnowLine, 1.0f, 50.0f, 256.0f, "%.0f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Altitude above which snow covers mountain surfaces.");

                        // Mountain height profile preview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Mountain Profile Preview");
                        ImGui::Separator();
                        {
                            const int mCount = 300;
                            float mHeights[mCount];
                            for (int i = 0; i < mCount; i++) {
                                float x = (float)i / (float)mCount * 10.0f;
                                float val = std::max(0.0f, std::sin(x * 0.8f + std::cos(x * 0.3f)));
                                val = std::pow(val, s.mountainSharpness);
                                mHeights[i] = val * s.mountainHeight / 256.0f;
                            }
                            ImGui::PlotLines("##MountainProfile", mHeights, mCount, 0,
                                nullptr, 0.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100));

                            float snowNorm = s.mountainSnowLine / 256.0f;
                            ImGui::TextDisabled("Snow line at: %.0f blocks (%.0f%% of world height)", s.mountainSnowLine, snowNorm * 100.0f);
                        }

                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "VOLCANO GENERATION");
                        ImGui::TextDisabled("Configure volcanic features in the world.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragInt("Volcano Count", &s.volcanoCount, 0.1f, 0, 20);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Number of volcanoes in the world.");
                        s.volcanoCount = std::clamp(s.volcanoCount, 0, 20);

                        if (s.volcanoCount > 0) {
                            ImGui::DragFloat("Volcano Radius", &s.volcanoRadius, 0.5f, 5.0f, 100.0f, "%.1f blocks");
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Base radius of the volcanic cone.");

                            ImGui::DragFloat("Volcano Height", &s.volcanoHeight, 1.0f, 10.0f, 200.0f, "%.0f blocks");
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Peak height of the volcano.");

                            ImGui::DragFloat("Lava Depth", &s.lavaDepth, 0.2f, 0.0f, 50.0f, "%.1f blocks");
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Depth of the lava pool inside the crater.");

                            ImGui::DragFloat("Eruption Chance", &s.eruptionChance, 0.005f, 0.0f, 1.0f, "%.3f");
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Probability of eruption events. 0 = dormant, 1 = always erupting.");

                            // Lava level bar
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 barPos = ImGui::GetCursorScreenPos();
                            float barW = 150.0f; float barH = 18.0f;
                            float fillRatio = std::clamp(s.lavaDepth / 50.0f, 0.0f, 1.0f);
                            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(40,40,40,200), 3.0f);
                            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW * fillRatio, barPos.y + barH), IM_COL32(255,80,20,220), 3.0f);
                            dl->AddRect(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(100,100,100,200), 3.0f);
                            char lavaLabel[32];
                            snprintf(lavaLabel, sizeof(lavaLabel), "Lava: %.0f%%", fillRatio * 100.0f);
                            dl->AddText(ImVec2(barPos.x + 4, barPos.y + 2), IM_COL32(255,255,255,230), lavaLabel);
                            ImGui::Dummy(ImVec2(barW, barH + 4));
                        }
                    }

                    // ========================================================
                    // TAB 3: Water Bodies
                    // ========================================================
                    if (t == 3) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "OCEAN SETTINGS");
                        ImGui::TextDisabled("Configure the global water level and ocean parameters.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Ocean Level", &s.oceanLevel, 0.5f, -1.0f, 200.0f, "%.1f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Global sea level Y coordinate. Set to -1 to disable.");

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "LAKE SETTINGS");
                        ImGui::TextDisabled("Configure inland lake generation.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragInt("Lake Count", &s.lakeCount, 0.1f, 0, 50);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Number of lakes generated per region.");
                        s.lakeCount = std::clamp(s.lakeCount, 0, 50);

                        ImGui::DragFloat("Lake Min Size", &s.lakeMinSize, 0.2f, 1.0f, s.lakeMaxSize, "%.1f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Minimum lake radius.");

                        ImGui::DragFloat("Lake Max Size", &s.lakeMaxSize, 0.2f, s.lakeMinSize, 100.0f, "%.1f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum lake radius.");

                        ImGui::DragFloat("Lake Depth", &s.lakeDepth, 0.2f, 1.0f, 30.0f, "%.1f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum depth of generated lakes.");

                        ImGui::DragFloat("Shore Width", &s.lakeShoreWidth, 0.1f, 0.0f, 10.0f, "%.1f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Width of the sandy/gravel shore around lakes.");

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "RIVER SETTINGS");
                        ImGui::TextDisabled("Configure river generation parameters.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("River Width", &s.riverWidth, 0.1f, 1.0f, 20.0f, "%.1f blocks");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Average width of generated rivers.");

                        ImGui::DragFloat("River Frequency", &s.riverFrequency, 0.0005f, 0.0f, 0.02f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How often rivers are generated. 0 = no rivers.");

                        // Water level overview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Water Level Overview");
                        ImGui::Separator();
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 barPos = ImGui::GetCursorScreenPos();
                            float barW = std::min(ImGui::GetContentRegionAvail().x - 16, 350.0f);
                            float barH = 60.0f;

                            // Terrain block
                            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(60,80,50,255));

                            // Sky above water
                            float waterRatio = std::clamp(s.oceanLevel / 256.0f, 0.0f, 1.0f);
                            float waterY = barPos.y + barH * (1.0f - waterRatio);
                            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, waterY), IM_COL32(100,160,220,180));

                            // Water line
                            dl->AddLine(ImVec2(barPos.x, waterY), ImVec2(barPos.x + barW, waterY),
                                IM_COL32(50,100,255,255), 2.0f);

                            char lvlText[32];
                            snprintf(lvlText, sizeof(lvlText), "Ocean: Y=%.0f", s.oceanLevel);
                            dl->AddText(ImVec2(barPos.x + 4, waterY - 16), IM_COL32(255,255,255,220), lvlText);

                            // Base height indicator
                            float baseRatio = std::clamp(s.baseHeight / 256.0f, 0.0f, 1.0f);
                            float baseY = barPos.y + barH * (1.0f - baseRatio);
                            dl->AddLine(ImVec2(barPos.x, baseY), ImVec2(barPos.x + barW, baseY),
                                IM_COL32(80,200,80,200), 1.5f);
                            snprintf(lvlText, sizeof(lvlText), "Base: Y=%.0f", s.baseHeight);
                            dl->AddText(ImVec2(barPos.x + barW - 100, baseY + 2), IM_COL32(80,200,80,220), lvlText);

                            dl->AddRect(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(80,80,80,200));
                            ImGui::Dummy(ImVec2(barW, barH + 4));
                        }
                    }

                    // ========================================================
                    // TAB 4: Caves & Ores
                    // ========================================================
                    if (t == 4) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "CAVE GENERATION");
                        ImGui::TextDisabled("Configure underground cave networks and their properties.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Cave Frequency", &s.caveFrequency, 0.001f, 0.0f, 0.2f, "%.3f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How often cave tunnels are generated. Higher = more caves.");

                        ImGui::DragFloat("Cave Threshold", &s.caveThreshold, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Noise threshold for carving caves. Lower values carve more space.");

                        ImGui::DragFloat("Cave Scale", &s.caveScale, 0.01f, 0.1f, 5.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale multiplier for cave tunnel size.");

                        ImGui::DragFloat("Stalactite Density", &s.stalactiteDensity, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Density of stalactite/stalagmite decorations inside caves.");

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "ORE GENERATION");
                        ImGui::TextDisabled("Configure underground ore vein frequency.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Ore Frequency", &s.oreFrequency, 0.001f, 0.0f, 0.1f, "%.3f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Global ore vein generation frequency. Affects all ore types.");

                        // Cave density visualizer
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Underground Density Overview");
                        ImGui::Separator();
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 pos = ImGui::GetCursorScreenPos();
                            float w = std::min(ImGui::GetContentRegionAvail().x - 16, 350.0f);

                            struct DensityBar { const char* label; float value; float maxVal; ImU32 color; };
                            DensityBar bars[] = {
                                {"Cave Freq",     s.caveFrequency,     0.2f, IM_COL32(120,80,160,220)},
                                {"Cave Threshold",s.caveThreshold,     1.0f, IM_COL32(100,100,140,220)},
                                {"Ore Freq",      s.oreFrequency,      0.1f, IM_COL32(200,180,60,220)},
                                {"Stalactites",   s.stalactiteDensity, 1.0f, IM_COL32(140,120,100,220)},
                            };
                            for (auto& bar : bars) {
                                ImGui::Text("%-16s", bar.label);
                                ImGui::SameLine(140);
                                ImVec2 bPos = ImGui::GetCursorScreenPos();
                                float bH = 14.0f;
                                float bW = w - 140;
                                float ratio = std::clamp(bar.value / bar.maxVal, 0.0f, 1.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(30,30,30,200), 2.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW * ratio, bPos.y + bH), bar.color, 2.0f);
                                dl->AddRect(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(60,60,60,200), 2.0f);
                                ImGui::Dummy(ImVec2(bW, bH + 2));
                            }
                        }
                    }

                    // ========================================================
                    // TAB 5: Vegetation
                    // ========================================================
                    if (t == 5) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "TREE SETTINGS");
                        ImGui::TextDisabled("Configure tree generation density and height range.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Tree Density", &s.treeDensity, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overall tree placement density. 0 = no trees, 1 = maximum.");

                        ImGui::DragInt("Tree Height Min", &s.treeHeightMin, 0.1f, 1, s.treeHeightMax);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Minimum tree trunk height in blocks.");
                        s.treeHeightMin = std::clamp(s.treeHeightMin, 1, s.treeHeightMax);

                        ImGui::DragInt("Tree Height Max", &s.treeHeightMax, 0.1f, s.treeHeightMin, 30);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum tree trunk height in blocks.");
                        s.treeHeightMax = std::clamp(s.treeHeightMax, s.treeHeightMin, 30);

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "GROUND COVER");
                        ImGui::TextDisabled("Configure grass, flowers, and bush generation.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragFloat("Grass Density", &s.grassDensity, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Percentage of surface covered with tall grass.");

                        ImGui::DragFloat("Flower Density", &s.flowerDensity, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Density of flower patches on grassy terrain.");

                        ImGui::DragFloat("Bush Density", &s.bushDensity, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Density of bush/shrub placement.");

                        // Vegetation density overview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Vegetation Density Overview");
                        ImGui::Separator();
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            float w = std::min(ImGui::GetContentRegionAvail().x - 16, 300.0f);

                            struct VegBar { const char* name; float value; ImU32 color; };
                            VegBar bars[] = {
                                {"Trees",   s.treeDensity,   IM_COL32(60,160,40,220)},
                                {"Grass",   s.grassDensity,  IM_COL32(80,200,60,220)},
                                {"Flowers", s.flowerDensity, IM_COL32(220,120,180,220)},
                                {"Bushes",  s.bushDensity,   IM_COL32(100,180,60,220)},
                            };
                            for (auto& bar : bars) {
                                ImGui::Text("%-10s", bar.name);
                                ImGui::SameLine(100);
                                ImVec2 bPos = ImGui::GetCursorScreenPos();
                                float bH = 14.0f;
                                float bW = w - 100;
                                float ratio = std::clamp(bar.value, 0.0f, 1.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(30,30,30,200), 2.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW * ratio, bPos.y + bH), bar.color, 2.0f);
                                dl->AddRect(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(60,60,60,200), 2.0f);
                                ImGui::Dummy(ImVec2(bW, bH + 2));
                            }
                        }

                        // Tree height range
                        ImGui::Spacing();
                        ImGui::TextDisabled("Tree height range: %d - %d blocks", s.treeHeightMin, s.treeHeightMax);
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 bPos = ImGui::GetCursorScreenPos();
                            float bW = 200.0f; float bH = 14.0f;
                            dl->AddRectFilled(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(30,30,30,200), 2.0f);
                            float minR = s.treeHeightMin / 30.0f;
                            float maxR = s.treeHeightMax / 30.0f;
                            dl->AddRectFilled(ImVec2(bPos.x + bW * minR, bPos.y), ImVec2(bPos.x + bW * maxR, bPos.y + bH),
                                IM_COL32(60,160,40,220), 2.0f);
                            dl->AddRect(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(60,60,60,200), 2.0f);
                            ImGui::Dummy(ImVec2(bW, bH + 4));
                        }
                    }

                    // ========================================================
                    // TAB 6: Structures
                    // ========================================================
                    if (t == 6) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "STRUCTURE GENERATION");
                        ImGui::TextDisabled("Configure the frequency of procedurally generated structures.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Surface Structures");
                        ImGui::Separator();

                        ImGui::DragFloat("Village Frequency", &s.villageFrequency, 0.0001f, 0.0f, 0.01f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How often villages spawn in valid biomes (plains, desert, savanna).");

                        ImGui::DragFloat("Temple Frequency", &s.templeFrequency, 0.0001f, 0.0f, 0.01f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Spawn frequency of temples and pyramids.");

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Underground Structures");
                        ImGui::Separator();

                        ImGui::DragFloat("Dungeon Frequency", &s.dungeonFrequency, 0.0001f, 0.0f, 0.02f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frequency of underground monster spawner dungeons.");

                        ImGui::DragFloat("Mineshaft Frequency", &s.mineshaftFrequency, 0.0001f, 0.0f, 0.02f, "%.4f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frequency of abandoned mineshaft generation.");

                        // Frequency bar chart
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Frequency Comparison");
                        ImGui::Separator();
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            float w = std::min(ImGui::GetContentRegionAvail().x - 16, 300.0f);

                            struct FreqBar { const char* name; float value; float maxVal; ImU32 color; };
                            FreqBar bars[] = {
                                {"Villages",   s.villageFrequency,   0.01f, IM_COL32(180,140,60,220)},
                                {"Temples",    s.templeFrequency,    0.01f, IM_COL32(160,120,80,220)},
                                {"Dungeons",   s.dungeonFrequency,   0.02f, IM_COL32(120,80,120,220)},
                                {"Mineshafts", s.mineshaftFrequency, 0.02f, IM_COL32(100,80,60,220)},
                            };
                            for (auto& bar : bars) {
                                ImGui::Text("%-11s", bar.name);
                                ImGui::SameLine(110);
                                ImVec2 bPos = ImGui::GetCursorScreenPos();
                                float bH = 14.0f;
                                float bW = w - 110;
                                float ratio = std::clamp(bar.value / bar.maxVal, 0.0f, 1.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(30,30,30,200), 2.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW * ratio, bPos.y + bH), bar.color, 2.0f);
                                dl->AddRect(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(60,60,60,200), 2.0f);
                                ImGui::Dummy(ImVec2(bW, bH + 2));
                            }
                        }
                    }

                    // ========================================================
                    // TAB 7: Erosion
                    // ========================================================
                    if (t == 7) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "EROSION SIMULATION");
                        ImGui::TextDisabled("Configure terrain erosion to create more natural landscapes.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::DragInt("Erosion Iterations", &s.erosionIterations, 100.0f, 0, 200000);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Number of erosion simulation passes.\nMore iterations = smoother, more natural terrain. 0 = disable erosion.");
                        s.erosionIterations = std::clamp(s.erosionIterations, 0, 200000);

                        ImGui::DragFloat("Erosion Strength", &s.erosionStrength, 0.005f, 0.0f, 1.0f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Intensity of hydraulic erosion.\nHigher values remove more material per droplet.");

                        ImGui::DragFloat("Sediment Capacity", &s.sedimentCapacity, 0.1f, 0.5f, 20.0f, "%.1f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much sediment each water droplet can carry.\nHigher = deeper channels and more material redistribution.");

                        ImGui::DragFloat("Thermal Erosion", &s.thermalErosion, 0.001f, 0.0f, 0.1f, "%.3f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rate of slope-based thermal erosion.\nCauses steep slopes to crumble and flatten over time.");

                        // Erosion intensity preview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Erosion Intensity");
                        ImGui::Separator();
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            float w = std::min(ImGui::GetContentRegionAvail().x - 16, 300.0f);

                            struct ErosionBar { const char* name; float value; float maxVal; ImU32 color; };
                            ErosionBar bars[] = {
                                {"Iterations", (float)s.erosionIterations / 200000.0f, 1.0f, IM_COL32(100,150,200,220)},
                                {"Strength",   s.erosionStrength, 1.0f, IM_COL32(150,120,80,220)},
                                {"Sediment",   s.sedimentCapacity / 20.0f, 1.0f, IM_COL32(180,160,100,220)},
                                {"Thermal",    s.thermalErosion / 0.1f, 1.0f, IM_COL32(200,100,80,220)},
                            };
                            for (auto& bar : bars) {
                                ImGui::Text("%-12s", bar.name);
                                ImGui::SameLine(120);
                                ImVec2 bPos = ImGui::GetCursorScreenPos();
                                float bH = 14.0f;
                                float bW = w - 120;
                                float ratio = std::clamp(bar.value, 0.0f, 1.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(30,30,30,200), 2.0f);
                                dl->AddRectFilled(bPos, ImVec2(bPos.x + bW * ratio, bPos.y + bH), bar.color, 2.0f);
                                dl->AddRect(bPos, ImVec2(bPos.x + bW, bPos.y + bH), IM_COL32(60,60,60,200), 2.0f);
                                ImGui::Dummy(ImVec2(bW, bH + 2));
                            }
                        }

                        // Before/after terrain preview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Erosion Effect Preview");
                        ImGui::TextDisabled("Top: raw terrain. Bottom: after erosion simulation.");
                        ImGui::Separator();
                        {
                            const int eCount = 300;
                            float rawHeights[eCount];
                            generatePreviewHeights(s, rawHeights, eCount);

                            // Simulate basic erosion on a copy
                            float erodedHeights[eCount];
                            for (int i = 0; i < eCount; i++) erodedHeights[i] = rawHeights[i];

                            // Simple smoothing passes to approximate erosion effect
                            int smoothPasses = std::clamp((int)(s.erosionStrength * 20.0f + s.thermalErosion * 100.0f), 0, 50);
                            for (int p = 0; p < smoothPasses; p++) {
                                float temp[eCount];
                                for (int i = 0; i < eCount; i++) {
                                    float left = (i > 0) ? erodedHeights[i-1] : erodedHeights[i];
                                    float right = (i < eCount-1) ? erodedHeights[i+1] : erodedHeights[i];
                                    temp[i] = erodedHeights[i] * 0.6f + left * 0.2f + right * 0.2f;
                                }
                                for (int i = 0; i < eCount; i++) erodedHeights[i] = temp[i];
                            }

                            ImGui::PlotLines("##RawTerrain", rawHeights, eCount, 0,
                                "Raw Terrain", 0.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
                            ImGui::PlotLines("##ErodedTerrain", erodedHeights, eCount, 0,
                                "After Erosion", 0.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
                        }
                    }

                    // ========================================================
                    // TAB 8: Presets Management
                    // ========================================================
                    if (t == 8) {
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "PRESET MANAGEMENT");
                        ImGui::TextDisabled("Save, load, and manage world generation presets.");
                        ImGui::Separator();
                        ImGui::Spacing();

                        // Preset name editing
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Active Preset");
                        ImGui::Separator();
                        ImGui::InputText("Name", s.name, 64);

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Quick Load Default Presets");
                        ImGui::TextDisabled("Replace the current preset with a built-in template.");
                        ImGui::Separator();

                        struct QuickPreset { const char* name; ImVec4 color; };
                        QuickPreset quickPresets[] = {
                            {"Default",   ImVec4(0.3f, 0.7f, 0.3f, 1.0f)},
                            {"Flat World",ImVec4(0.6f, 0.6f, 0.6f, 1.0f)},
                            {"Amplified", ImVec4(0.8f, 0.5f, 0.2f, 1.0f)},
                            {"Islands",   ImVec4(0.3f, 0.6f, 0.9f, 1.0f)},
                            {"Mesa",      ImVec4(0.8f, 0.6f, 0.3f, 1.0f)},
                            {"Volcanic",  ImVec4(0.9f, 0.3f, 0.2f, 1.0f)},
                        };

                        for (int qi = 0; qi < 6; qi++) {
                            if (qi > 0 && qi % 3 != 0) ImGui::SameLine();
                            ImGui::PushID(600 + qi);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(quickPresets[qi].color.x * 0.5f,
                                quickPresets[qi].color.y * 0.5f, quickPresets[qi].color.z * 0.5f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, quickPresets[qi].color);
                            if (ImGui::Button(quickPresets[qi].name, ImVec2(150, 30))) {
                                pushUndo();
                                WorldGenSettings loaded;
                                switch (qi) {
                                    case 0: loaded = makeDefaultPreset(); break;
                                    case 1: loaded = makeFlatPreset(); break;
                                    case 2: loaded = makeAmplifiedPreset(); break;
                                    case 3: loaded = makeIslandsPreset(); break;
                                    case 4: loaded = makeMesaPreset(); break;
                                    case 5: loaded = makeVolcanicPreset(); break;
                                }
                                loaded.seed = s.seed; // Keep current seed
                                m_presets[m_activePresetIdx] = loaded;
                                m_saveNotifyTimer = 2.0f;
                            }
                            ImGui::PopStyleColor(2);
                            ImGui::PopID();
                        }

                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Import / Export");
                        ImGui::TextDisabled("Save all presets to disk or reload from file.");
                        ImGui::Separator();

                        if (ImGui::Button("Save All Presets to Disk", ImVec2(220, 28))) {
                            savePresetsToFile(kPresetsFile);
                            m_dirty = false;
                            m_saveNotifyTimer = 2.0f;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Writes all presets to '%s'", kPresetsFile);

                        ImGui::SameLine();
                        if (ImGui::Button("Reload Presets from Disk", ImVec2(220, 28))) {
                            if (loadPresetsFromFile(kPresetsFile)) {
                                m_dirty = false;
                                m_undoStack.clear();
                                m_redoStack.clear();
                                m_saveNotifyTimer = 2.0f;
                            }
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reloads presets from '%s'", kPresetsFile);

                        ImGui::SameLine();
                        if (ImGui::Button("Reset to Defaults", ImVec2(180, 28))) {
                            initDefaults();
                            m_saveNotifyTimer = 2.0f;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset all presets to built-in defaults. Unsaved data will be lost.");

                        // Preset list overview
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "All Presets (%d)", (int)m_presets.size());
                        ImGui::Separator();

                        ImGui::BeginChild("##PresetListOverview", ImVec2(0, 200), true);
                        for (int pi = 0; pi < (int)m_presets.size(); pi++) {
                            ImGui::PushID(700 + pi);
                            bool isActive = (pi == m_activePresetIdx);

                            if (isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));

                            ImGui::Text("%s%s", m_presets[pi].name, isActive ? "  [Active]" : "");
                            ImGui::SameLine(300);
                            ImGui::TextDisabled("Seed: %d | Base: %.0f | Amp: %.0f",
                                m_presets[pi].seed, m_presets[pi].baseHeight, m_presets[pi].terrainAmplitude);

                            if (isActive) ImGui::PopStyleColor();
                            ImGui::PopID();
                        }
                        ImGui::EndChild();

                        // Current preset summary
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Current Preset Summary");
                        ImGui::Separator();

                        ImGui::Columns(2, "##summaryCol", false);
                        ImGui::SetColumnWidth(0, 200);

                        ImGui::TextDisabled("Name:"); ImGui::SameLine(); ImGui::Text("%s", s.name);
                        ImGui::TextDisabled("Seed:"); ImGui::SameLine(); ImGui::Text("%d", s.seed);
                        ImGui::TextDisabled("Octaves:"); ImGui::SameLine(); ImGui::Text("%d", s.octaves);
                        ImGui::TextDisabled("Height:"); ImGui::SameLine(); ImGui::Text("%.0f +/- %.0f", s.baseHeight, s.terrainAmplitude);
                        ImGui::TextDisabled("Ocean:"); ImGui::SameLine();
                        if (s.oceanLevel < 0) ImGui::Text("Disabled");
                        else ImGui::Text("Y=%.0f", s.oceanLevel);

                        ImGui::NextColumn();

                        ImGui::TextDisabled("Mountains:"); ImGui::SameLine(); ImGui::Text("%.0f high", s.mountainHeight);
                        ImGui::TextDisabled("Volcanos:"); ImGui::SameLine(); ImGui::Text("%d", s.volcanoCount);
                        ImGui::TextDisabled("Lakes:"); ImGui::SameLine(); ImGui::Text("%d", s.lakeCount);
                        ImGui::TextDisabled("Cave Freq:"); ImGui::SameLine(); ImGui::Text("%.3f", s.caveFrequency);
                        ImGui::TextDisabled("Erosion:"); ImGui::SameLine(); ImGui::Text("%d iters", s.erosionIterations);

                        ImGui::Columns(1);
                    }

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        // ====================================================================
        // Terrain preview at bottom (always visible)
        // ====================================================================
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "TERRAIN CROSS-SECTION");
        ImGui::TextDisabled("Approximate terrain profile based on current settings.");
        {
            const int prevCount = 500;
            float prevHeights[prevCount];
            generatePreviewHeights(s, prevHeights, prevCount);

            float availW = ImGui::GetContentRegionAvail().x;
            float plotH = std::min(std::max(availW * 0.18f, 60.0f), 100.0f);
            ImGui::PlotLines("##GlobalTerrainPreview", prevHeights, prevCount, 0,
                nullptr, 0.0f, 1.0f, ImVec2(availW, plotH));
        }

        // ====================================================================
        // Detect changes for undo
        // ====================================================================
        {
            bool changed = false;
            if (std::memcmp(&s, &preEdit, sizeof(WorldGenSettings)) != 0)
                changed = true;
            if (changed) {
                m_undoStack.push_back(preEdit);
                if ((int)m_undoStack.size() > kMaxUndo)
                    m_undoStack.erase(m_undoStack.begin());
                m_redoStack.clear();
                m_dirty = true;
            }
        }
    }
    ImGui::EndChild(); // end ##WE_RightSide

    ImGui::End();
}
