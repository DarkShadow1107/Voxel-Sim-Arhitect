#include "HelperWindow.hpp"
#include "imgui.h"
#include <cstring>

// ---------------------------------------------------------------------------
//  Internal helpers
// ---------------------------------------------------------------------------

static bool matchesSearch(const char* text, const char* q) {
    if (!q || q[0] == '\0') return true;
    // Case-insensitive substring match
    const char* a = text;
    while (*a) {
        const char* ta = a;
        const char* tb = q;
        while (*ta && *tb && ((*ta | 0x20) == (*tb | 0x20))) { ++ta; ++tb; }
        if (!*tb) return true;
        ++a;
    }
    return false;
}

// Render a two-column keybinding row: key in col-0, description in col-1.
// Returns false (is filtered out) if neither key nor desc passes the search.
static bool keyRow(const char* key, const char* desc, const char* q) {
    if (!matchesSearch(key, q) && !matchesSearch(desc, q)) return false;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.45f, 1.0f), "%s", key);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextDisabled("%s", desc);
    return true;
}

// Render a two-column panel-info row.
static bool panelRow(const char* name, const char* info, const char* q) {
    if (!matchesSearch(name, q) && !matchesSearch(info, q)) return false;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), "%s", name);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextDisabled("%s", info);
    return true;
}

// ---------------------------------------------------------------------------

void HelperWindow::show() {
    if (!m_open) return;

    ImGui::SetNextWindowSize(ImVec2(640, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    if (!ImGui::Begin("Help & Reference", &m_open,
                       ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // Header
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f),
                       "Voxel-Sim Architect — Help & Reference");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
    ImGui::TextDisabled("V0.6 Beta  |  OpenGL 4.1 + Dear ImGui");
    ImGui::Separator();

    // Search bar
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##helpsearch", "Search controls, panels, tips...",
                             m_search, sizeof(m_search));
    ImGui::Spacing();

    const char* q = m_search;
    const bool searching = (q[0] != '\0');

    // Tab bar (suppressed while searching so all results show at once)
    bool showControls = true, showEditor = true, showTips = true, showAbout = true;

    if (!searching) {
        if (ImGui::BeginTabBar("##helptabs")) {
            showControls = ImGui::BeginTabItem("Controls");
            if (showControls) ImGui::EndTabItem();
            showEditor   = ImGui::BeginTabItem("Editor Panels");
            if (showEditor)   ImGui::EndTabItem();
            showTips     = ImGui::BeginTabItem("Tips");
            if (showTips)     ImGui::EndTabItem();
            showAbout    = ImGui::BeginTabItem("About");
            if (showAbout)    ImGui::EndTabItem();
            ImGui::EndTabBar();
        }
    }

    ImGui::BeginChild("##helpcontent", ImVec2(0, 0), false,
                       ImGuiWindowFlags_HorizontalScrollbar);

    // ── Controls tab ─────────────────────────────────────────────────────────
    if (showControls || searching) {
        if (searching) ImGui::SeparatorText("Controls");

        constexpr ImGuiTableFlags tFlags =
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("##ctrl", 2, tFlags)) {
            ImGui::TableSetupColumn("Key / Input",   ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Action",        ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "Movement");

            keyRow("W / A / S / D",  "Move Forward / Left / Back / Right", q);
            keyRow("Arrow Keys",     "Move Forward / Left / Back / Right (same as WASD)", q);
            keyRow("Space",          "Jump  |  Fly Up (fly mode)", q);
            keyRow("Left Shift",     "Crouch / Sprint Brake  |  Fly Down", q);
            keyRow("Left Ctrl",      "Speed Boost (2.5x)", q);
            keyRow("Double Space",   "Toggle Fly Mode", q);
            keyRow("Double W",       "Toggle Sprint (1.8x speed)", q);
            keyRow("Mouse Look",     "Aim / Look Around", q);

            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "Blocks & Tools");

            keyRow("LMB",            "Break Block (hold)", q);
            keyRow("RMB",            "Place Block (tap)", q);
            keyRow("1 – 9",          "Select Hotbar Slot", q);
            keyRow("Scroll Wheel",   "Cycle Hotbar Slots", q);
            keyRow("E",              "Open / Close Inventory", q);

            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "Editor & System");

            keyRow("F10",            "Toggle Menu Mode / World Input", q);
            keyRow("F11",            "Toggle Fullscreen", q);
            keyRow("Ctrl + P",       "Open Preferences", q);
            keyRow("T",              "Open Chat (command console)", q);
            keyRow("Escape",         "Pause / Game Menu", q);

            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "Chat Commands");

            keyRow("/clear",         "Set weather to Clear", q);
            keyRow("/rain",          "Start Rain", q);
            keyRow("/snow",          "Start Snow", q);
            keyRow("/time set <N>",  "Set world time (0 = midnight, 6000 = sunrise, 12000 = noon)", q);
            keyRow("/save",          "Quick-save the current world", q);
            keyRow("/load",          "Open the World List", q);
            keyRow("/new",           "Start a new world", q);

            ImGui::EndTable();
        }
    }

    // ── Editor Panels tab ────────────────────────────────────────────────────
    if (showEditor || searching) {
        if (searching) { ImGui::Spacing(); ImGui::SeparatorText("Editor Panels"); }

        constexpr ImGuiTableFlags tFlags =
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("##panels", 2, tFlags)) {
            ImGui::TableSetupColumn("Panel",        ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Description",  ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            panelRow("World Editor",      "Seed, render distance, time/weather, save & load worlds", q);
            panelRow("Block Designer",    "Create or edit custom block types; set textures and properties", q);
            panelRow("Mob Designer",      "Design mob appearances and part hierarchies", q);
            panelRow("Texture Designer",  "Paint the 16x16 atlas texture grid directly in the editor", q);
            panelRow("Tool Designer",     "Define custom tools: damage, speed, tier, special effects", q);
            panelRow("Weather Designer",  "Configure rain / snow / fog presets with custom particles", q);
            panelRow("Sound Designer",    "Synthesise procedural sounds with oscillators and envelopes", q);
            panelRow("Sound Editor",      "Assign audio clips to block / mob / environment events", q);
            panelRow("Interaction Editor","Build behaviour trees and AI node graphs for mobs", q);
            panelRow("Adv. World Editor", "Sculpt terrain in-editor; paint biome regions", q);
            panelRow("Profiler",          "Frame time graph, GPU/CPU metrics, draw-call counters", q);
            panelRow("Memory Inspector",  "Arena allocator usage, chunk pool utilization", q);
            panelRow("ECS Editor",        "Inspect all active entities and their component data", q);
            panelRow("Preferences",       "VSync, wireframe, backface culling, fullscreen toggle", q);

            ImGui::EndTable();
        }
    }

    // ── Tips tab ─────────────────────────────────────────────────────────────
    if (showTips || searching) {
        if (searching) { ImGui::Spacing(); ImGui::SeparatorText("Tips"); }

        static const char* kTips[] = {
            "Break a water or lava source block to stop the flow.",
            "Place water next to a lava source to create Obsidian.",
            "Place water next to flowing lava to create Cobblestone.",
            "Generation-placed water and lava are dormant — they only start flowing once a neighbour block is broken.",
            "Double-tap Space to enter / exit fly mode for fast camera movement.",
            "Double-tap W (or Up arrow) to sprint at 1.8x walk speed.",
            "All editor panels are fully dockable — drag them by their title bar to reorganise your workspace.",
            "Use the Viewport compass (top-right) to snap the camera to cardinal views.",
            "The Block Designer can add custom drops, break times, tool requirements, and fire/burn behaviours.",
            "FireAspect tools (Sword subtypes) set struck mobs on fire for 5 seconds.",
            "Sheep drop coloured wool matching their fleece — build the Mob Designer to customise colours.",
            "The Weather Designer supports custom particle colours, fog, wind, thunder rate, and ambient sounds.",
            "The Sound Designer synthesises procedural SFX with two oscillators, ADSR envelope, and effects bus.",
            "Ctrl+P opens Preferences at any time, even while the game is paused.",
            "The Interaction Editor uses a node graph to define mob attack, flee, idle, and patrol behaviours.",
        };

        int shown = 0;
        for (const char* tip : kTips) {
            if (!matchesSearch(tip, q)) continue;
            ImGui::Bullet(); ImGui::SameLine();
            ImGui::TextWrapped("%s", tip);
            ++shown;
        }
        if (shown == 0)
            ImGui::TextDisabled("No tips match the search.");
    }

    // ── About tab ────────────────────────────────────────────────────────────
    if (showAbout || searching) {
        if (searching) { ImGui::Spacing(); ImGui::SeparatorText("About"); }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f),
                           "Voxel-Sim Architect  V0.6 Beta");
        ImGui::Spacing();
        ImGui::TextDisabled("Build: " __DATE__ "  " __TIME__);
        ImGui::TextDisabled("Renderer: OpenGL 4.1 (GLAD loader)");
        ImGui::TextDisabled("UI: Dear ImGui (docking branch) + ImGuizmo");
        ImGui::TextDisabled("Noise: FastNoiseLite — OpenSimplex2 / FBm");
        ImGui::TextDisabled("Audio: AudioManager (OpenAL-compatible backend)");
        ImGui::Spacing();
        ImGui::SeparatorText("Architecture");
        ImGui::TextDisabled("ECS — Registry, component pools, O(1) entity iteration");
        ImGui::TextDisabled("FluidSimulator — event-driven dormant/active two-state fluid physics");
        ImGui::TextDisabled("BiomeRegistry — central data table for all 9 biome definitions");
        ImGui::TextDisabled("TaskScheduler — multi-threaded chunk gen + mesh building");
        ImGui::TextDisabled("ArenaAllocator + ChunkPool — zero-fragment memory management");
    }

    ImGui::EndChild();
    ImGui::End();
}
