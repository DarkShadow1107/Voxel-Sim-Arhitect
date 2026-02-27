#include "WeatherDesigner.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
namespace {
    std::string browseSound() {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = 260;
        ofn.lpstrFilter = "Audio Files (*.wav;*.mp3)\0*.wav;*.mp3\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameA(&ofn) == TRUE) return std::string(szFile);
        return "";
    }
}
#else
namespace { std::string browseSound() { return ""; } }
#endif

void WeatherDesigner::initDefaults() {
    m_presets.clear();

    // Clear
    {
        WeatherPreset p;
        p.name = "Clear";
        p.temperature = 22.0f;
        p.visibility = 1000.0f;
        p.cloudCoverage = 0.1f;
        m_presets.push_back(p);
    }
    // Drizzle
    {
        WeatherPreset p;
        p.name = "Drizzle";
        p.particleCount = 80;
        p.particleSpeed = 600.0f;
        p.particleSize = 1.0f;
        p.particleColor = {0.3f, 0.5f, 1.0f};
        p.particleAlpha = 0.5f;
        p.windStrength = 1.0f;
        p.fogDensity = 0.1f;
        p.skyBrightness = 0.8f;
        p.cloudCoverage = 0.5f;
        p.cloudDarkness = 0.2f;
        p.temperature = 15.0f;
        p.visibility = 500.0f;
        p.wetness = 0.3f;
        p.ambientSound = "assets/sounds/rain_ambient.wav";
        p.ambientVolume = 0.25f;
        m_presets.push_back(p);
    }
    // Rain
    {
        WeatherPreset p;
        p.name = "Rain";
        p.particleCount = 250;
        p.particleSpeed = 800.0f;
        p.particleSize = 1.5f;
        p.particleColor = {0.3f, 0.47f, 1.0f};
        p.particleAlpha = 0.7f;
        p.windStrength = 3.0f;
        p.windDirection = 45.0f;
        p.fogDensity = 0.2f;
        p.fogColor = {0.4f, 0.4f, 0.5f};
        p.skyBrightness = 0.65f;
        p.cloudCoverage = 0.8f;
        p.cloudDarkness = 0.4f;
        p.temperature = 12.0f;
        p.visibility = 200.0f;
        p.wetness = 0.7f;
        p.ambientSound = "assets/sounds/rain_ambient.wav";
        p.ambientVolume = 0.4f;
        m_presets.push_back(p);
    }
    // Storm
    {
        WeatherPreset p;
        p.name = "Storm";
        p.particleCount = 400;
        p.particleSpeed = 1200.0f;
        p.particleSize = 2.0f;
        p.particleColor = {0.25f, 0.4f, 0.9f};
        p.particleAlpha = 0.85f;
        p.windStrength = 8.0f;
        p.windDirection = 30.0f;
        p.windGusts = 4.0f;
        p.gustFrequency = 0.3f;
        p.fogDensity = 0.5f;
        p.fogColor = {0.3f, 0.3f, 0.35f};
        p.skyBrightness = 0.35f;
        p.cloudCoverage = 1.0f;
        p.cloudDarkness = 0.7f;
        p.hasThunder = true;
        p.thunderFrequency = 0.002f;
        p.thunderVolume = 0.7f;
        p.lightningFrequency = 0.003f;
        p.temperature = 10.0f;
        p.visibility = 80.0f;
        p.wetness = 1.0f;
        p.ambientSound = "assets/sounds/rain_ambient.wav";
        p.ambientVolume = 0.6f;
        m_presets.push_back(p);
    }
    // Snow
    {
        WeatherPreset p;
        p.name = "Snow";
        p.particleCount = 150;
        p.particleSpeed = 150.0f;
        p.particleSize = 2.5f;
        p.particleColor = {1.0f, 1.0f, 1.0f};
        p.particleAlpha = 0.85f;
        p.snowStyle = true;
        p.precipitationType = 1;
        p.windStrength = 2.0f;
        p.windDirection = 90.0f;
        p.fogDensity = 0.15f;
        p.fogColor = {0.7f, 0.7f, 0.75f};
        p.skyBrightness = 0.75f;
        p.cloudCoverage = 0.7f;
        p.cloudColor = {0.9f, 0.9f, 0.95f};
        p.temperature = -2.0f;
        p.visibility = 300.0f;
        p.season = "winter";
        m_presets.push_back(p);
    }
    // Blizzard
    {
        WeatherPreset p;
        p.name = "Blizzard";
        p.particleCount = 350;
        p.particleSpeed = 300.0f;
        p.particleSize = 3.0f;
        p.particleColor = {0.95f, 0.95f, 1.0f};
        p.particleAlpha = 0.9f;
        p.snowStyle = true;
        p.precipitationType = 1;
        p.windStrength = 9.0f;
        p.windDirection = 60.0f;
        p.windGusts = 5.0f;
        p.gustFrequency = 0.4f;
        p.fogDensity = 0.6f;
        p.fogColor = {0.8f, 0.8f, 0.85f};
        p.skyBrightness = 0.3f;
        p.cloudCoverage = 1.0f;
        p.cloudDarkness = 0.3f;
        p.temperature = -15.0f;
        p.visibility = 30.0f;
        p.season = "winter";
        m_presets.push_back(p);
    }
    // Hail
    {
        WeatherPreset p;
        p.name = "Hail";
        p.particleCount = 120;
        p.particleSpeed = 1500.0f;
        p.particleSize = 3.0f;
        p.particleColor = {0.85f, 0.9f, 1.0f};
        p.particleAlpha = 0.9f;
        p.precipitationType = 2;
        p.windStrength = 4.0f;
        p.windGusts = 3.0f;
        p.fogDensity = 0.15f;
        p.skyBrightness = 0.5f;
        p.cloudCoverage = 0.9f;
        p.cloudDarkness = 0.6f;
        p.hasThunder = true;
        p.thunderFrequency = 0.001f;
        p.thunderVolume = 0.5f;
        p.temperature = 5.0f;
        p.visibility = 150.0f;
        m_presets.push_back(p);
    }
    // Sandstorm
    {
        WeatherPreset p;
        p.name = "Sandstorm";
        p.particleCount = 300;
        p.particleSpeed = 200.0f;
        p.particleSize = 2.0f;
        p.particleColor = {0.8f, 0.65f, 0.3f};
        p.particleAlpha = 0.6f;
        p.precipitationType = 5;
        p.particleSpread = 2.0f;
        p.windStrength = 7.0f;
        p.windDirection = 90.0f;
        p.windGusts = 3.0f;
        p.fogDensity = 0.7f;
        p.fogColor = {0.7f, 0.6f, 0.35f};
        p.skyBrightness = 0.6f;
        p.skyColorDay = {0.7f, 0.6f, 0.4f};
        p.cloudCoverage = 0.0f;
        p.temperature = 40.0f;
        p.visibility = 40.0f;
        p.season = "summer";
        m_presets.push_back(p);
    }
}

float WeatherDesigner::applyEasing(float t, int easingType) const {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (easingType) {
        case 1: return t * t;                              // ease-in
        case 2: return 1.0f - (1.0f - t) * (1.0f - t);    // ease-out
        case 3: return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f; // ease-in-out
        default: return t;                                  // linear
    }
}

WeatherPreset WeatherDesigner::lerpPresets(const WeatherPreset& a, const WeatherPreset& b, float t) const {
    WeatherPreset r;
    r.name = "Transition";
    auto mix = [](float a, float b, float t) { return a + (b - a) * t; };
    auto mixV = [&](Vec3 a, Vec3 b) { return Vec3{mix(a.x,b.x,t), mix(a.y,b.y,t), mix(a.z,b.z,t)}; };

    r.particleCount = (int)mix((float)a.particleCount, (float)b.particleCount, t);
    r.particleSpeed = mix(a.particleSpeed, b.particleSpeed, t);
    r.particleSize = mix(a.particleSize, b.particleSize, t);
    r.particleColor = mixV(a.particleColor, b.particleColor);
    r.particleAlpha = mix(a.particleAlpha, b.particleAlpha, t);
    r.snowStyle = (t < 0.5f) ? a.snowStyle : b.snowStyle;
    r.precipitationType = (t < 0.5f) ? a.precipitationType : b.precipitationType;
    r.particleSpread = mix(a.particleSpread, b.particleSpread, t);
    r.windStrength = mix(a.windStrength, b.windStrength, t);
    r.windDirection = mix(a.windDirection, b.windDirection, t);
    r.windGusts = mix(a.windGusts, b.windGusts, t);
    r.gustFrequency = mix(a.gustFrequency, b.gustFrequency, t);
    r.fogDensity = mix(a.fogDensity, b.fogDensity, t);
    r.fogColor = mixV(a.fogColor, b.fogColor);
    r.fogStartDistance = mix(a.fogStartDistance, b.fogStartDistance, t);
    r.fogEndDistance = mix(a.fogEndDistance, b.fogEndDistance, t);
    r.fogHeight = mix(a.fogHeight, b.fogHeight, t);
    r.skyColorDay = mixV(a.skyColorDay, b.skyColorDay);
    r.skyColorNight = mixV(a.skyColorNight, b.skyColorNight);
    r.skyBrightness = mix(a.skyBrightness, b.skyBrightness, t);
    r.cloudCoverage = mix(a.cloudCoverage, b.cloudCoverage, t);
    r.cloudHeight = mix(a.cloudHeight, b.cloudHeight, t);
    r.cloudSpeed = mix(a.cloudSpeed, b.cloudSpeed, t);
    r.cloudColor = mixV(a.cloudColor, b.cloudColor);
    r.cloudDarkness = mix(a.cloudDarkness, b.cloudDarkness, t);
    r.hasThunder = (t < 0.5f) ? a.hasThunder : b.hasThunder;
    r.thunderFrequency = mix(a.thunderFrequency, b.thunderFrequency, t);
    r.thunderVolume = mix(a.thunderVolume, b.thunderVolume, t);
    r.lightningFrequency = mix(a.lightningFrequency, b.lightningFrequency, t);
    r.temperature = mix(a.temperature, b.temperature, t);
    r.visibility = mix(a.visibility, b.visibility, t);
    r.wetness = mix(a.wetness, b.wetness, t);
    r.ambientSound = (t < 0.5f) ? a.ambientSound : b.ambientSound;
    r.ambientVolume = mix(a.ambientVolume, b.ambientVolume, t);
    r.extraSound = (t < 0.5f) ? a.extraSound : b.extraSound;
    r.extraSoundVolume = mix(a.extraSoundVolume, b.extraSoundVolume, t);
    r.season = (t < 0.5f) ? a.season : b.season;
    return r;
}

const WeatherPreset& WeatherDesigner::getActivePreset() const {
    if (m_activeTransitionIdx >= 0) return m_blendedPreset;
    if (m_activePresetIdx >= 0 && m_activePresetIdx < (int)m_presets.size())
        return m_presets[m_activePresetIdx];
    static WeatherPreset fallback;
    return fallback;
}

void WeatherDesigner::update(float dt, float worldTime) {
    if (!m_inited) return;
    m_previewTime += dt;

    // Check scheduled transitions
    if (m_activeTransitionIdx < 0) {
        for (int i = 0; i < (int)m_transitions.size(); ++i) {
            auto& tr = m_transitions[i];
            if (tr.triggerWorldTime >= 0.0f) {
                float diff = std::abs(worldTime - tr.triggerWorldTime);
                if (diff < 20.0f) {
                    m_activeTransitionIdx = i;
                    m_transitionProgress = 0.0f;
                    m_activePresetIdx = tr.fromIdx;
                    break;
                }
            }
        }
    }

    // Advance active transition
    if (m_activeTransitionIdx >= 0 && m_activeTransitionIdx < (int)m_transitions.size()) {
        auto& tr = m_transitions[m_activeTransitionIdx];
        float dur = std::max(tr.duration, 0.1f);
        m_transitionProgress += dt / dur;
        if (m_transitionProgress >= 1.0f) {
            m_transitionProgress = 1.0f;
            m_activePresetIdx = tr.toIdx;
            m_activeTransitionIdx = -1;
        } else {
            int fi = std::clamp(tr.fromIdx, 0, (int)m_presets.size() - 1);
            int ti = std::clamp(tr.toIdx, 0, (int)m_presets.size() - 1);
            float eased = applyEasing(m_transitionProgress, tr.easingType);
            m_blendedPreset = lerpPresets(m_presets[fi], m_presets[ti], eased);
        }
    }
}

void WeatherDesigner::show(bool* open) {
    if (!*open) return;
    if (!m_inited) { initDefaults(); m_inited = true; }

    ImVec2 mvCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(mvCenter, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(1200, 750), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Weather Designer", open)) {
        ImGui::End();
        return;
    }

    // Ctrl+Z / Ctrl+Y
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();

    // ===== LEFT PANEL: Preset List =====
    ImGui::BeginChild("##WP_List", ImVec2(220, 0), true);
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "PRESETS");
    ImGui::Separator();

    for (int i = 0; i < (int)m_presets.size(); ++i) {
        ImGui::PushID(i);
        bool sel = (m_selectedPresetIdx == i);
        bool active = (m_activePresetIdx == i);

        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.8f, 0.5f));
        char label[128];
        const char* seasonTag = "";
        if (!m_presets[i].season.empty()) {
            if (m_presets[i].season == "spring") seasonTag = " [Spr]";
            else if (m_presets[i].season == "summer") seasonTag = " [Sum]";
            else if (m_presets[i].season == "autumn") seasonTag = " [Aut]";
            else if (m_presets[i].season == "winter") seasonTag = " [Win]";
        }
        snprintf(label, sizeof(label), "%s%s%s", m_presets[i].name.c_str(), seasonTag, active ? "  [Active]" : "");
        if (ImGui::Selectable(label, sel)) {
            m_selectedPresetIdx = i;
        }
        if (sel) ImGui::PopStyleColor();
        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("+ Add Preset", ImVec2(-1, 28))) {
        WeatherPreset p;
        p.name = "New Preset";
        m_presets.push_back(p);
        m_selectedPresetIdx = (int)m_presets.size() - 1;
    }

    if (m_selectedPresetIdx >= 0 && m_selectedPresetIdx < (int)m_presets.size()) {
        if (ImGui::Button("Set Active", ImVec2(-1, 28))) {
            m_activePresetIdx = m_selectedPresetIdx;
            m_activeTransitionIdx = -1;
        }

        if (ImGui::Button("Duplicate", ImVec2(-1, 24))) {
            WeatherPreset copy = m_presets[m_selectedPresetIdx];
            copy.name += " Copy";
            m_presets.push_back(copy);
            m_selectedPresetIdx = (int)m_presets.size() - 1;
        }

        if (ImGui::Button("Copy", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, 24))) {
            m_clipboard = m_presets[m_selectedPresetIdx];
            m_hasClipboard = true;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, m_hasClipboard ? ImVec4(0.3f, 0.6f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Paste", ImVec2(-1, 24)) && m_hasClipboard) {
            std::string keepName = m_presets[m_selectedPresetIdx].name;
            m_presets[m_selectedPresetIdx] = m_clipboard;
            m_presets[m_selectedPresetIdx].name = keepName;
            m_dirty = true;
        }
        ImGui::PopStyleColor();
    }

    // Move up/down
    if (m_selectedPresetIdx >= 0 && m_selectedPresetIdx < (int)m_presets.size()) {
        float halfW = ImGui::GetContentRegionAvail().x * 0.5f - 2;
        if (m_selectedPresetIdx > 0) {
            if (ImGui::Button("Move Up", ImVec2(halfW, 22))) {
                std::swap(m_presets[m_selectedPresetIdx], m_presets[m_selectedPresetIdx - 1]);
                m_selectedPresetIdx--;
            }
        } else {
            ImGui::InvisibleButton("##noUp", ImVec2(halfW, 22));
        }
        ImGui::SameLine();
        if (m_selectedPresetIdx < (int)m_presets.size() - 1) {
            if (ImGui::Button("Move Down", ImVec2(-1, 22))) {
                std::swap(m_presets[m_selectedPresetIdx], m_presets[m_selectedPresetIdx + 1]);
                m_selectedPresetIdx++;
            }
        }
    }

    if (m_selectedPresetIdx >= 0 && m_selectedPresetIdx < (int)m_presets.size() && (int)m_presets.size() > 1) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("- Delete Preset", ImVec2(-1, 24))) {
            m_presets.erase(m_presets.begin() + m_selectedPresetIdx);
            if (m_activePresetIdx >= (int)m_presets.size()) m_activePresetIdx = (int)m_presets.size() - 1;
            if (m_selectedPresetIdx >= (int)m_presets.size()) m_selectedPresetIdx = (int)m_presets.size() - 1;
        }
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::SameLine();

    if (m_selectedPresetIdx < 0 || m_selectedPresetIdx >= (int)m_presets.size()) {
        ImGui::TextDisabled("Select a preset.");
        ImGui::End();
        return;
    }

    // ===== CENTER PANEL: Preset Editor =====
    ImGui::BeginChild("##WP_Editor", ImVec2(500, 0), true);
    {
        auto& p = m_presets[m_selectedPresetIdx];
        WeatherPreset preEdit = p;

        // Toolbar
        {
            ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            if (ImGui::Button("Save", ImVec2(70, 28)) && m_dirty) m_dirty = false;
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_undoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
            if (ImGui::Button("Undo", ImVec2(55, 28)) && !m_undoStack.empty()) undo();
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_redoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
            if (ImGui::Button("Redo", ImVec2(55, 28)) && !m_redoStack.empty()) redo();
            ImGui::PopStyleColor();

            if (m_dirty) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " * Modified");
            }
        }
        ImGui::Separator();

        // Name
        char nameBuf[64];
        strncpy(nameBuf, p.name.c_str(), 63); nameBuf[63] = '\0';
        if (ImGui::InputText("Name", nameBuf, 64)) p.name = nameBuf;

        // Season
        const char* seasonOptions[] = {"None", "Spring", "Summer", "Autumn", "Winter"};
        const char* seasonValues[] = {"", "spring", "summer", "autumn", "winter"};
        int seasonIdx = 0;
        for (int i = 1; i < 5; i++) if (p.season == seasonValues[i]) { seasonIdx = i; break; }
        if (ImGui::Combo("Season", &seasonIdx, seasonOptions, 5)) p.season = seasonValues[seasonIdx];

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "PRECIPITATION");
        ImGui::Separator();

        const char* precipTypes[] = {"Rain", "Snow", "Hail", "Sleet", "Ash", "Sandstorm"};
        ImGui::Combo("Type", &p.precipitationType, precipTypes, 6);
        ImGui::DragInt("Count", &p.particleCount, 1.0f, 0, 500);
        ImGui::DragFloat("Fall Speed", &p.particleSpeed, 5.0f, 50.0f, 2000.0f, "%.0f px/s");
        ImGui::DragFloat("Size", &p.particleSize, 0.02f, 0.5f, 6.0f, "%.1f");
        ImGui::DragFloat("Spread", &p.particleSpread, 0.01f, 0.5f, 3.0f, "%.2f");
        ImGui::ColorEdit3("Color##part", &p.particleColor.x, ImGuiColorEditFlags_Float);
        ImGui::DragFloat("Alpha", &p.particleAlpha, 0.005f, 0.0f, 1.0f, "%.2f");
        ImGui::Checkbox("Snow Style (circles)", &p.snowStyle);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "WIND");
        ImGui::Separator();
        ImGui::DragFloat("Strength", &p.windStrength, 0.05f, 0.0f, 15.0f, "%.1f");
        ImGui::DragFloat("Direction", &p.windDirection, 1.0f, 0.0f, 360.0f, "%.0f deg");
        ImGui::DragFloat("Gust Intensity", &p.windGusts, 0.05f, 0.0f, 10.0f, "%.1f");
        if (p.windGusts > 0.01f) {
            ImGui::DragFloat("Gust Frequency", &p.gustFrequency, 0.01f, 0.1f, 2.0f, "%.2f /s");
        }

        // Wind compass
        {
            ImVec2 cpos = ImGui::GetCursorScreenPos();
            float r = 30.0f;
            ImVec2 center(cpos.x + r + 10, cpos.y + r + 5);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddCircle(center, r, IM_COL32(80, 80, 80, 200), 32);
            float rad = p.windDirection * 3.14159f / 180.0f;
            float len = r * std::clamp(p.windStrength / 15.0f, 0.1f, 1.0f);
            ImVec2 tip(center.x + std::cos(rad) * len, center.y - std::sin(rad) * len);
            dl->AddLine(center, tip, IM_COL32(100, 200, 255, 255), 2.0f);
            // Gust indicator
            if (p.windGusts > 0.01f) {
                float gustLen = len * (1.0f + p.windGusts / p.windStrength * 0.3f);
                gustLen = std::min(gustLen, r);
                ImVec2 gustTip(center.x + std::cos(rad) * gustLen, center.y - std::sin(rad) * gustLen);
                dl->AddLine(center, gustTip, IM_COL32(255, 150, 50, 120), 4.0f);
            }
            dl->AddText(ImVec2(center.x - 3, center.y - r - 14), IM_COL32(200, 200, 200, 200), "N");
            dl->AddText(ImVec2(center.x + r + 3, center.y - 5), IM_COL32(200, 200, 200, 150), "E");
            dl->AddText(ImVec2(center.x - r - 10, center.y - 5), IM_COL32(200, 200, 200, 150), "W");
            dl->AddText(ImVec2(center.x - 3, center.y + r + 2), IM_COL32(200, 200, 200, 150), "S");
            ImGui::Dummy(ImVec2(r * 2 + 20, r * 2 + 15));
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "FOG");
        ImGui::Separator();
        ImGui::DragFloat("Density", &p.fogDensity, 0.005f, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit3("Fog Color", &p.fogColor.x, ImGuiColorEditFlags_Float);
        ImGui::DragFloat("Start Distance", &p.fogStartDistance, 0.5f, 0.0f, 100.0f, "%.0f m");
        ImGui::DragFloat("End Distance", &p.fogEndDistance, 1.0f, 10.0f, 500.0f, "%.0f m");
        ImGui::DragFloat("Fog Height", &p.fogHeight, 0.5f, 5.0f, 200.0f, "%.0f");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "SKY");
        ImGui::Separator();
        ImGui::ColorEdit3("Day Sky", &p.skyColorDay.x, ImGuiColorEditFlags_Float);
        ImGui::ColorEdit3("Night Sky", &p.skyColorNight.x, ImGuiColorEditFlags_Float);
        ImGui::DragFloat("Brightness", &p.skyBrightness, 0.01f, 0.0f, 2.0f, "%.2f");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "CLOUDS");
        ImGui::Separator();
        ImGui::DragFloat("Coverage", &p.cloudCoverage, 0.005f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Height##cloud", &p.cloudHeight, 1.0f, 32.0f, 256.0f, "%.0f");
        ImGui::DragFloat("Speed##cloud", &p.cloudSpeed, 0.02f, 0.0f, 5.0f, "%.1f");
        ImGui::ColorEdit3("Cloud Color", &p.cloudColor.x, ImGuiColorEditFlags_Float);
        ImGui::DragFloat("Underbelly Darkness", &p.cloudDarkness, 0.005f, 0.0f, 1.0f, "%.2f");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "THUNDER & LIGHTNING");
        ImGui::Separator();
        ImGui::Checkbox("Enabled##thunder", &p.hasThunder);
        if (p.hasThunder) {
            ImGui::DragFloat("Thunder Freq##th", &p.thunderFrequency, 0.0001f, 0.0001f, 0.01f, "%.4f");
            ImGui::DragFloat("Thunder Vol##th", &p.thunderVolume, 0.005f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Lightning Freq", &p.lightningFrequency, 0.0001f, 0.0001f, 0.02f, "%.4f");
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "ENVIRONMENT");
        ImGui::Separator();
        ImGui::DragFloat("Temperature", &p.temperature, 0.2f, -30.0f, 50.0f, "%.1f C");
        // Color-coded temperature indicator
        {
            float normT = std::clamp((p.temperature + 30.0f) / 80.0f, 0.0f, 1.0f);
            ImVec4 tempCol;
            if (normT < 0.25f) tempCol = ImVec4(0.2f, 0.3f, 1.0f, 1.0f);       // cold blue
            else if (normT < 0.5f) tempCol = ImVec4(0.3f, 0.8f, 0.4f, 1.0f);    // cool green
            else if (normT < 0.75f) tempCol = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);   // warm yellow
            else tempCol = ImVec4(1.0f, 0.3f, 0.2f, 1.0f);                       // hot red
            ImGui::SameLine();
            ImGui::TextColored(tempCol, "%s",
                p.temperature < -10 ? "Freezing" : p.temperature < 0 ? "Cold" :
                p.temperature < 15 ? "Cool" : p.temperature < 25 ? "Warm" :
                p.temperature < 35 ? "Hot" : "Scorching");
        }
        ImGui::DragFloat("Visibility", &p.visibility, 5.0f, 5.0f, 2000.0f, "%.0f m");
        ImGui::DragFloat("Wetness", &p.wetness, 0.005f, 0.0f, 1.0f, "%.2f");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "SOUND");
        ImGui::Separator();
        // Primary ambient sound
        char soundBuf[128];
        strncpy(soundBuf, p.ambientSound.c_str(), 127); soundBuf[127] = '\0';
        if (ImGui::InputText("Ambient Sound", soundBuf, 128)) p.ambientSound = soundBuf;
        ImGui::SameLine();
        if (ImGui::SmallButton("Browse##wsnd")) {
            std::string path = browseSound();
            if (!path.empty()) p.ambientSound = path;
        }
        ImGui::DragFloat("Volume##snd", &p.ambientVolume, 0.005f, 0.0f, 1.0f, "%.2f");

        // Secondary sound layer
        ImGui::Spacing();
        ImGui::TextDisabled("Secondary Sound Layer");
        char extraBuf[128];
        strncpy(extraBuf, p.extraSound.c_str(), 127); extraBuf[127] = '\0';
        if (ImGui::InputText("Extra Sound", extraBuf, 128)) p.extraSound = extraBuf;
        ImGui::SameLine();
        if (ImGui::SmallButton("Browse##xsnd")) {
            std::string path = browseSound();
            if (!path.empty()) p.extraSound = path;
        }
        ImGui::DragFloat("Extra Volume", &p.extraSoundVolume, 0.005f, 0.0f, 1.0f, "%.2f");

        // Detect changes for undo
        {
            bool changed = false;
            if (p.name != preEdit.name) changed = true;
            if (p.season != preEdit.season) changed = true;
            if (p.particleCount != preEdit.particleCount) changed = true;
            if (p.particleSpeed != preEdit.particleSpeed) changed = true;
            if (p.particleSize != preEdit.particleSize) changed = true;
            if (p.particleAlpha != preEdit.particleAlpha) changed = true;
            if (p.snowStyle != preEdit.snowStyle) changed = true;
            if (p.precipitationType != preEdit.precipitationType) changed = true;
            if (p.particleSpread != preEdit.particleSpread) changed = true;
            if (p.windStrength != preEdit.windStrength) changed = true;
            if (p.windDirection != preEdit.windDirection) changed = true;
            if (p.windGusts != preEdit.windGusts) changed = true;
            if (p.gustFrequency != preEdit.gustFrequency) changed = true;
            if (p.fogDensity != preEdit.fogDensity) changed = true;
            if (p.fogStartDistance != preEdit.fogStartDistance) changed = true;
            if (p.fogEndDistance != preEdit.fogEndDistance) changed = true;
            if (p.fogHeight != preEdit.fogHeight) changed = true;
            if (p.skyBrightness != preEdit.skyBrightness) changed = true;
            if (p.cloudCoverage != preEdit.cloudCoverage) changed = true;
            if (p.cloudHeight != preEdit.cloudHeight) changed = true;
            if (p.cloudSpeed != preEdit.cloudSpeed) changed = true;
            if (p.cloudDarkness != preEdit.cloudDarkness) changed = true;
            if (p.hasThunder != preEdit.hasThunder) changed = true;
            if (p.thunderFrequency != preEdit.thunderFrequency) changed = true;
            if (p.thunderVolume != preEdit.thunderVolume) changed = true;
            if (p.lightningFrequency != preEdit.lightningFrequency) changed = true;
            if (p.temperature != preEdit.temperature) changed = true;
            if (p.visibility != preEdit.visibility) changed = true;
            if (p.wetness != preEdit.wetness) changed = true;
            if (p.ambientSound != preEdit.ambientSound) changed = true;
            if (p.ambientVolume != preEdit.ambientVolume) changed = true;
            if (p.extraSound != preEdit.extraSound) changed = true;
            if (p.extraSoundVolume != preEdit.extraSoundVolume) changed = true;
            if (p.particleColor.x != preEdit.particleColor.x || p.particleColor.y != preEdit.particleColor.y || p.particleColor.z != preEdit.particleColor.z) changed = true;
            if (p.fogColor.x != preEdit.fogColor.x || p.fogColor.y != preEdit.fogColor.y || p.fogColor.z != preEdit.fogColor.z) changed = true;
            if (p.skyColorDay.x != preEdit.skyColorDay.x || p.skyColorDay.y != preEdit.skyColorDay.y || p.skyColorDay.z != preEdit.skyColorDay.z) changed = true;
            if (p.skyColorNight.x != preEdit.skyColorNight.x || p.skyColorNight.y != preEdit.skyColorNight.y || p.skyColorNight.z != preEdit.skyColorNight.z) changed = true;
            if (p.cloudColor.x != preEdit.cloudColor.x || p.cloudColor.y != preEdit.cloudColor.y || p.cloudColor.z != preEdit.cloudColor.z) changed = true;
            if (changed) {
                m_undoStack.push_back(preEdit);
                if ((int)m_undoStack.size() > kMaxUndo) m_undoStack.erase(m_undoStack.begin());
                m_redoStack.clear();
                m_dirty = true;
            }
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // ===== RIGHT PANEL: Live Preview + Transitions =====
    ImGui::BeginChild("##WP_Preview", ImVec2(0, 0), true);
    {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "LIVE PREVIEW");
        ImGui::Separator();

        auto& p = (m_activeTransitionIdx >= 0) ? m_blendedPreset : m_presets[std::clamp(m_selectedPresetIdx, 0, (int)m_presets.size() - 1)];

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float pvH = std::min(avail.y * 0.50f, 350.0f);
        float pvW = avail.x - 16;
        if (pvW < 100) pvW = 100;

        ImVec2 pvPos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Sky gradient background
        Vec3 skyTop = p.skyColorDay;
        Vec3 skyBot = {skyTop.x * 0.6f, skyTop.y * 0.6f, skyTop.z * 0.7f};
        skyTop.x *= p.skyBrightness; skyTop.y *= p.skyBrightness; skyTop.z *= p.skyBrightness;
        skyBot.x *= p.skyBrightness; skyBot.y *= p.skyBrightness; skyBot.z *= p.skyBrightness;
        ImU32 colTop = IM_COL32((int)(std::clamp(skyTop.x, 0.0f, 1.0f) * 255), (int)(std::clamp(skyTop.y, 0.0f, 1.0f) * 255), (int)(std::clamp(skyTop.z, 0.0f, 1.0f) * 255), 255);
        ImU32 colBot = IM_COL32((int)(std::clamp(skyBot.x, 0.0f, 1.0f) * 255), (int)(std::clamp(skyBot.y, 0.0f, 1.0f) * 255), (int)(std::clamp(skyBot.z, 0.0f, 1.0f) * 255), 255);
        dl->AddRectFilledMultiColor(pvPos, ImVec2(pvPos.x + pvW, pvPos.y + pvH), colTop, colTop, colBot, colBot);

        // Cloud layer
        if (p.cloudCoverage > 0.01f) {
            float cloudY = pvPos.y + pvH * 0.15f;
            float cloudH = pvH * 0.12f;
            int cloudAlpha = (int)(p.cloudCoverage * 200.0f);

            // Animated cloud shapes
            float drift = m_previewTime * p.cloudSpeed * 20.0f;
            int numClouds = (int)(p.cloudCoverage * 8.0f) + 1;
            for (int ci = 0; ci < numClouds; ci++) {
                float seed = (float)(ci * 337 + 17);
                float cx = std::fmod(seed * 123.4f + drift, pvW + 100.0f) - 50.0f;
                float cw = 40.0f + std::fmod(seed * 45.6f, 60.0f);
                float ch = cloudH * (0.5f + std::fmod(seed * 7.8f, 0.5f));
                float cy = cloudY + std::fmod(seed * 12.3f, cloudH * 0.5f);

                ImU32 topCol = IM_COL32((int)(p.cloudColor.x * 255), (int)(p.cloudColor.y * 255), (int)(p.cloudColor.z * 255), cloudAlpha);
                float darkFactor = 1.0f - p.cloudDarkness;
                ImU32 botCol = IM_COL32((int)(p.cloudColor.x * darkFactor * 255), (int)(p.cloudColor.y * darkFactor * 255), (int)(p.cloudColor.z * darkFactor * 255), cloudAlpha);
                dl->AddRectFilledMultiColor(ImVec2(pvPos.x + cx, cy), ImVec2(pvPos.x + cx + cw, cy + ch), topCol, topCol, botCol, botCol);
            }
        }

        // Ground
        float groundY = pvPos.y + pvH * 0.75f;
        ImU32 groundCol = IM_COL32(60, 80, 50, 255);
        // Wet ground darker
        if (p.wetness > 0.01f) {
            int darken = (int)(p.wetness * 30);
            groundCol = IM_COL32(std::max(0, 60 - darken), std::max(0, 80 - darken), std::max(0, 50 - darken), 255);
        }
        dl->AddRectFilled(ImVec2(pvPos.x, groundY), ImVec2(pvPos.x + pvW, pvPos.y + pvH), groundCol);

        // Visibility indicator (darkening at distance)
        if (p.visibility < 500.0f) {
            float visAlpha = std::clamp((500.0f - p.visibility) / 500.0f * 0.3f, 0.0f, 0.3f);
            dl->AddRectFilled(pvPos, ImVec2(pvPos.x + pvW, pvPos.y + pvH),
                IM_COL32((int)(p.fogColor.x * 255), (int)(p.fogColor.y * 255), (int)(p.fogColor.z * 255), (int)(visAlpha * 255)));
        }

        // Fog overlay
        if (p.fogDensity > 0.01f) {
            int fogAlpha = (int)(p.fogDensity * 180.0f);
            ImU32 fc = IM_COL32((int)(p.fogColor.x * 255), (int)(p.fogColor.y * 255), (int)(p.fogColor.z * 255), fogAlpha);
            // Height-based fog: denser at bottom
            float fogTop = pvPos.y + pvH * 0.3f;
            dl->AddRectFilledMultiColor(
                ImVec2(pvPos.x, fogTop), ImVec2(pvPos.x + pvW, pvPos.y + pvH),
                IM_COL32((int)(p.fogColor.x * 255), (int)(p.fogColor.y * 255), (int)(p.fogColor.z * 255), fogAlpha / 4),
                IM_COL32((int)(p.fogColor.x * 255), (int)(p.fogColor.y * 255), (int)(p.fogColor.z * 255), fogAlpha / 4),
                fc, fc);
        }

        // Particles with wind gusts
        float t = m_previewTime;
        int pc = p.particleCount;
        float effectiveWind = p.windStrength;
        if (p.windGusts > 0.01f) {
            // Simulate intermittent gusts
            float gustPhase = std::sin(t * p.gustFrequency * 6.28f);
            if (gustPhase > 0.3f) effectiveWind += p.windGusts * gustPhase;
        }
        float windDrift = effectiveWind * 2.0f;
        ImU32 partCol = IM_COL32(
            (int)(p.particleColor.x * 255),
            (int)(p.particleColor.y * 255),
            (int)(p.particleColor.z * 255),
            (int)(p.particleAlpha * 255));

        for (int i = 0; i < pc; ++i) {
            float seed = (float)(i * 7919 + 31) * 0.001f;
            float rx = std::fmod(seed * 1234.5f, pvW) * p.particleSpread;
            float baseY = std::fmod(seed * 567.8f + t * p.particleSpeed * 0.3f, pvH);
            float drift = std::sin(t * 0.5f + seed) * windDrift;

            float sx = pvPos.x + std::fmod(rx + drift, pvW);
            float sy = pvPos.y + baseY;

            if (sx < pvPos.x) sx += pvW;
            if (sx > pvPos.x + pvW) sx -= pvW;

            if (p.snowStyle || p.precipitationType == 1) {
                dl->AddCircleFilled(ImVec2(sx, sy), p.particleSize, partCol);
            } else if (p.precipitationType == 2) {
                // Hail - small filled circles
                dl->AddCircleFilled(ImVec2(sx, sy), p.particleSize * 0.8f, partCol);
                dl->AddCircle(ImVec2(sx, sy), p.particleSize * 0.8f, IM_COL32(255, 255, 255, 80));
            } else if (p.precipitationType == 4) {
                // Ash - small fading circles
                dl->AddCircleFilled(ImVec2(sx, sy), p.particleSize * 0.6f, partCol);
            } else if (p.precipitationType == 5) {
                // Sandstorm - horizontal streaks
                float streakLen = std::clamp(effectiveWind * 2.0f, 3.0f, 20.0f);
                dl->AddLine(ImVec2(sx, sy), ImVec2(sx + streakLen, sy + 1.0f), partCol, p.particleSize * 0.4f);
            } else {
                // Rain / sleet - lines
                float lineLen = std::clamp(p.particleSpeed * 0.015f, 5.0f, 25.0f);
                dl->AddLine(ImVec2(sx, sy), ImVec2(sx + drift * 0.1f, sy + lineLen), partCol, std::clamp(p.particleSize * 0.5f, 0.5f, 3.0f));
            }
        }

        // Thunder/lightning flash
        if (p.hasThunder) {
            int flashChance = (int)(p.lightningFrequency * 500.0f);
            if (flashChance > 0 && (std::rand() % std::max(1, 1000 / flashChance)) == 0) {
                dl->AddRectFilled(pvPos, ImVec2(pvPos.x + pvW, pvPos.y + pvH), IM_COL32(255, 255, 255, 100));
                // Lightning bolt
                float lx = pvPos.x + std::fmod(std::rand() * 0.01f, pvW * 0.6f) + pvW * 0.2f;
                float ly = pvPos.y + pvH * 0.1f;
                for (int seg = 0; seg < 4; seg++) {
                    float nx = lx + (std::rand() % 30 - 15);
                    float ny = ly + pvH * 0.15f;
                    dl->AddLine(ImVec2(lx, ly), ImVec2(nx, ny), IM_COL32(200, 200, 255, 255), 2.0f);
                    lx = nx; ly = ny;
                }
            }
        }

        // Wind direction arrow
        {
            ImVec2 arrowCenter(pvPos.x + pvW - 30, pvPos.y + 25);
            float rad = p.windDirection * 3.14159f / 180.0f;
            float len = 15.0f * std::clamp(effectiveWind / 15.0f, 0.1f, 1.0f);
            ImVec2 tip(arrowCenter.x + std::cos(rad) * len, arrowCenter.y - std::sin(rad) * len);
            dl->AddCircle(arrowCenter, 18, IM_COL32(200, 200, 200, 100), 16);
            dl->AddLine(arrowCenter, tip, IM_COL32(100, 200, 255, 200), 2.0f);
        }

        // Temperature display
        {
            char tempStr[32];
            snprintf(tempStr, sizeof(tempStr), "%.0f C", p.temperature);
            ImVec2 tempPos(pvPos.x + 8, pvPos.y + 8);
            float normT = std::clamp((p.temperature + 30.0f) / 80.0f, 0.0f, 1.0f);
            ImU32 tempCol;
            if (normT < 0.25f) tempCol = IM_COL32(50, 80, 255, 220);
            else if (normT < 0.5f) tempCol = IM_COL32(80, 200, 100, 220);
            else if (normT < 0.75f) tempCol = IM_COL32(255, 200, 50, 220);
            else tempCol = IM_COL32(255, 80, 50, 220);
            dl->AddText(tempPos, tempCol, tempStr);
        }

        // Border
        dl->AddRect(pvPos, ImVec2(pvPos.x + pvW, pvPos.y + pvH), IM_COL32(80, 80, 80, 200));
        ImGui::Dummy(ImVec2(pvW, pvH));

        // Status bar
        ImGui::Text("Active: %s", m_presets[std::clamp(m_activePresetIdx, 0, (int)m_presets.size() - 1)].name.c_str());
        if (m_activeTransitionIdx >= 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " Transitioning... %.0f%%", m_transitionProgress * 100.0f);
        }
        ImGui::SameLine();
        ImGui::TextDisabled(" | Visibility: %.0fm | Wetness: %.0f%%", p.visibility, p.wetness * 100.0f);

        ImGui::Spacing();
        if (ImGui::SmallButton("Randomize Current Preset")) {
            auto& rp = m_presets[m_selectedPresetIdx];
            pushUndo();
            rp.particleCount = std::rand() % 400;
            rp.particleSpeed = 100.0f + (std::rand() % 1500);
            rp.particleSize = 0.5f + (std::rand() % 50) * 0.1f;
            rp.windStrength = (std::rand() % 150) * 0.1f;
            rp.windDirection = (float)(std::rand() % 360);
            rp.fogDensity = (std::rand() % 100) * 0.01f;
            rp.skyBrightness = 0.2f + (std::rand() % 80) * 0.01f;
            rp.cloudCoverage = (std::rand() % 100) * 0.01f;
            rp.temperature = -20.0f + (std::rand() % 60);
            rp.visibility = 20.0f + (std::rand() % 980);
            rp.wetness = (std::rand() % 100) * 0.01f;
            m_dirty = true;
        }

        ImGui::Spacing();
        ImGui::Separator();

        // ===== TRANSITIONS =====
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "TRANSITIONS");
        ImGui::Separator();

        if (ImGui::Button("+ Add Transition", ImVec2(160, 24))) {
            WeatherTransition tr;
            tr.fromIdx = 0;
            tr.toIdx = std::min(1, (int)m_presets.size() - 1);
            tr.duration = 5.0f;
            tr.triggerWorldTime = -1.0f;
            tr.easingType = 0;
            m_transitions.push_back(tr);
        }

        ImGui::BeginChild("##TransList", ImVec2(0, 0), false);
        for (int i = 0; i < (int)m_transitions.size(); ++i) {
            ImGui::PushID(800 + i);
            auto& tr = m_transitions[i];

            // From
            const char* fromName = (tr.fromIdx >= 0 && tr.fromIdx < (int)m_presets.size()) ? m_presets[tr.fromIdx].name.c_str() : "?";
            ImGui::SetNextItemWidth(100);
            if (ImGui::BeginCombo("From", fromName)) {
                for (int j = 0; j < (int)m_presets.size(); ++j) {
                    if (ImGui::Selectable(m_presets[j].name.c_str(), tr.fromIdx == j)) tr.fromIdx = j;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            // To
            const char* toName = (tr.toIdx >= 0 && tr.toIdx < (int)m_presets.size()) ? m_presets[tr.toIdx].name.c_str() : "?";
            ImGui::SetNextItemWidth(100);
            if (ImGui::BeginCombo("To", toName)) {
                for (int j = 0; j < (int)m_presets.size(); ++j) {
                    if (ImGui::Selectable(m_presets[j].name.c_str(), tr.toIdx == j)) tr.toIdx = j;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            ImGui::SetNextItemWidth(70);
            ImGui::DragFloat("##dur", &tr.duration, 0.1f, 0.5f, 60.0f, "%.1fs");
            ImGui::SameLine();

            // Easing type
            const char* easingNames[] = {"Linear", "Ease In", "Ease Out", "Ease I/O"};
            ImGui::SetNextItemWidth(80);
            ImGui::Combo("##easing", &tr.easingType, easingNames, 4);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(80);
            ImGui::InputFloat("##time", &tr.triggerWorldTime, 0, 0, "%.0f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("World time trigger (-1 = manual only)");
            ImGui::SameLine();

            if (ImGui::SmallButton("Trigger")) {
                m_activeTransitionIdx = i;
                m_transitionProgress = 0.0f;
                m_activePresetIdx = tr.fromIdx;
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            if (ImGui::SmallButton("X##del")) {
                m_transitions.erase(m_transitions.begin() + i);
                --i;
            }
            ImGui::PopStyleColor();

            ImGui::PopID();
        }

        // Timeline bar
        if (!m_transitions.empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("Timeline (24000 units)");
            ImVec2 tlPos = ImGui::GetCursorScreenPos();
            float tlW = ImGui::GetContentRegionAvail().x - 16;
            float tlH = 24.0f;
            ImDrawList* tdl = ImGui::GetWindowDrawList();
            tdl->AddRectFilled(tlPos, ImVec2(tlPos.x + tlW, tlPos.y + tlH), IM_COL32(30, 30, 30, 200));

            // Day/night indicator
            float dawnX = (6000.0f / 24000.0f) * tlW;
            float duskX = (18000.0f / 24000.0f) * tlW;
            tdl->AddRectFilled(ImVec2(tlPos.x + dawnX, tlPos.y), ImVec2(tlPos.x + duskX, tlPos.y + tlH), IM_COL32(60, 60, 40, 100));

            for (int i = 0; i < (int)m_transitions.size(); ++i) {
                auto& tr = m_transitions[i];
                if (tr.triggerWorldTime < 0) continue;
                float x = (tr.triggerWorldTime / 24000.0f) * tlW;
                float w = (tr.duration / 24000.0f * 13.33f) * tlW;
                if (w < 4) w = 4;
                ImU32 tc = (i == m_activeTransitionIdx) ? IM_COL32(255, 200, 50, 200) : IM_COL32(100, 180, 255, 180);
                tdl->AddRectFilled(ImVec2(tlPos.x + x, tlPos.y + 2), ImVec2(tlPos.x + x + w, tlPos.y + tlH - 2), tc, 2.0f);
            }
            tdl->AddRect(tlPos, ImVec2(tlPos.x + tlW, tlPos.y + tlH), IM_COL32(80, 80, 80, 200));
            ImGui::Dummy(ImVec2(tlW, tlH));
        }

        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::End();
}

void WeatherDesigner::pushUndo() {
    if (m_selectedPresetIdx >= 0 && m_selectedPresetIdx < (int)m_presets.size()) {
        m_undoStack.push_back(m_presets[m_selectedPresetIdx]);
        if ((int)m_undoStack.size() > kMaxUndo) m_undoStack.erase(m_undoStack.begin());
        m_redoStack.clear();
        m_dirty = true;
    }
}

void WeatherDesigner::undo() {
    if (m_undoStack.empty()) return;
    if (m_selectedPresetIdx >= 0 && m_selectedPresetIdx < (int)m_presets.size()) {
        m_redoStack.push_back(m_presets[m_selectedPresetIdx]);
        m_presets[m_selectedPresetIdx] = m_undoStack.back();
        m_undoStack.pop_back();
        m_dirty = !m_undoStack.empty();
    }
}

void WeatherDesigner::redo() {
    if (m_redoStack.empty()) return;
    if (m_selectedPresetIdx >= 0 && m_selectedPresetIdx < (int)m_presets.size()) {
        m_undoStack.push_back(m_presets[m_selectedPresetIdx]);
        m_presets[m_selectedPresetIdx] = m_redoStack.back();
        m_redoStack.pop_back();
        m_dirty = true;
    }
}
