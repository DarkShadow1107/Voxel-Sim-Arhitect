#include "GUIManager.hpp"
#include "Renderer.hpp"
#include "Framebuffer.hpp"
#include "Shader.hpp"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// NVIDIA VRAM extension
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049

GUIManager::GUIManager() {}

GUIManager::~GUIManager() {
    shutdown();
}

bool GUIManager::init(GLFWwindow* window) {
    std::cout << "Initializing Dear ImGui..." << std::endl;
    if (m_initialized) {
        return true;
    }
    if (!window) {
        std::cerr << "GUIManager::init called with null window" << std::endl;
        return false;
    }

    m_window = window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDragClickToInputText = true; // Allow single-click on DragFloat/DragInt to type values

    applyTheme();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "ImGui_ImplGlfw_InitForOpenGL failed" << std::endl;
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "ImGui_ImplOpenGL3_Init failed" << std::endl;
        return false;
    }

    m_blockPreviewBuf = new Framebuffer();
    m_blockPreviewBuf->init(512, 512);
    m_mobPreviewBuf = new Framebuffer();
    m_mobPreviewBuf->init(512, 512);
    m_toolPreviewBuf = new Framebuffer();
    m_toolPreviewBuf->init(512, 512);
    m_partPreviewBuf = new Framebuffer();
    m_partPreviewBuf->init(256, 256);
    m_previewShader = new Shader();

    // Simple dedicated preview shaders – no fog, vignette, AO, or night cycle.
    // Applies vertex color exactly once for correct, bright preview rendering.
    static const char* kPreviewVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
layout(location=3) in vec2 aTexCoord;
uniform mat4 uModel, uView, uProjection;
out vec3 vNormal, vColor;
out vec2 vTexCoord;
void main(){
    vec4 wp = uModel * vec4(aPos, 1.0);
    vNormal  = mat3(uModel) * aNormal;
    vColor   = aColor;
    vTexCoord= aTexCoord;
    gl_Position = uProjection * uView * wp;
}
)";
    static const char* kPreviewFrag = R"(
#version 330 core
out vec4 FragColor;
in vec3 vNormal, vColor;
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uColorTint;
void main(){
    vec3 n = normalize(vNormal);
    vec3 l = normalize(-uLightDir);
    vec4 tex = texture(uTexture, vTexCoord);
    if(tex.a < 0.05) discard;
    float ndotl = max(dot(n, l), 0.0);
    float lighting = 0.55 + 0.50 * ndotl;
    vec3 color = lighting * tex.rgb * vColor * uColorTint;
    FragColor = vec4(color, tex.a);
}
)";
    if (!m_previewShader->loadFromSource(kPreviewVert, kPreviewFrag)) {
        std::cerr << "Failed to compile preview shader from source!" << std::endl;
    }

    glfwSwapInterval(m_vsync ? 1 : 0);
    m_initialized = true;
    return true;
}

void GUIManager::beginFrame() {
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUIManager::applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // --- Geometry & spacing ---
    style.WindowPadding     = ImVec2(10, 10);
    style.FramePadding      = ImVec2(6, 4);
    style.CellPadding       = ImVec2(6, 3);
    style.ItemSpacing       = ImVec2(8, 5);
    style.ItemInnerSpacing  = ImVec2(6, 4);
    style.IndentSpacing     = 18.0f;
    style.ScrollbarSize     = 13.0f;
    style.GrabMinSize       = 10.0f;

    // --- Rounding ---
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;

    // --- Borders ---
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;

    // Accent: muted teal-blue (#3D8EC9 / 0.24, 0.56, 0.79)
    const ImVec4 accent     = ImVec4(0.24f, 0.56f, 0.79f, 1.00f);
    const ImVec4 accentDim  = ImVec4(0.18f, 0.42f, 0.62f, 1.00f);
    const ImVec4 accentHi   = ImVec4(0.32f, 0.65f, 0.90f, 1.00f);

    // --- Text ---
    colors[ImGuiCol_Text]                   = ImVec4(0.93f, 0.94f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.45f, 0.48f, 0.52f, 1.00f);

    // --- Backgrounds ---
    colors[ImGuiCol_WindowBg]               = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.10f, 0.11f, 0.13f, 0.96f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);

    // --- Borders ---
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.22f, 0.25f, 0.60f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // --- Frames (input fields, combos, drag floats) ---
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.21f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);

    // --- Title bars ---
    colors[ImGuiCol_TitleBg]                = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.09f, 0.10f, 0.12f, 0.75f);

    // --- Scrollbars ---
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.11f, 0.13f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.25f, 0.27f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.32f, 0.34f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.38f, 0.40f, 0.45f, 1.00f);

    // --- Interactive controls ---
    colors[ImGuiCol_CheckMark]              = accentHi;
    colors[ImGuiCol_SliderGrab]             = accent;
    colors[ImGuiCol_SliderGrabActive]       = accentHi;

    // --- Buttons ---
    colors[ImGuiCol_Button]                 = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = accent;
    colors[ImGuiCol_ButtonActive]           = accentDim;

    // --- Headers (collapsing headers, selectables) ---
    colors[ImGuiCol_Header]                 = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(accent.x, accent.y, accent.z, 0.65f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(accent.x, accent.y, accent.z, 0.85f);

    // --- Separators ---
    colors[ImGuiCol_Separator]              = ImVec4(0.22f, 0.24f, 0.27f, 0.80f);
    colors[ImGuiCol_SeparatorHovered]       = accent;
    colors[ImGuiCol_SeparatorActive]        = accentHi;

    // --- Resize grips ---
    colors[ImGuiCol_ResizeGrip]             = ImVec4(accent.x, accent.y, accent.z, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(accent.x, accent.y, accent.z, 0.55f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(accent.x, accent.y, accent.z, 0.85f);

    // --- Tabs ---
    colors[ImGuiCol_Tab]                    = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(accent.x, accent.y, accent.z, 0.75f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);

    // --- Docking ---
    colors[ImGuiCol_DockingPreview]         = ImVec4(accent.x, accent.y, accent.z, 0.60f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.08f, 0.09f, 0.10f, 1.00f);

    // --- Plots ---
    colors[ImGuiCol_PlotLines]              = ImVec4(0.50f, 0.65f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = accentHi;
    colors[ImGuiCol_PlotHistogram]          = accent;
    colors[ImGuiCol_PlotHistogramHovered]   = accentHi;

    // --- Table ---
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.22f, 0.24f, 0.27f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.18f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    // --- Misc ---
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(accent.x, accent.y, accent.z, 0.30f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(accent.x, accent.y, accent.z, 0.90f);
    colors[ImGuiCol_NavHighlight]           = accent;
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);

    // Multi-viewport style
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void GUIManager::showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS, bool& showWorldEditor, bool& showSettings, bool& showSoundEditor, bool& showBlockDesigner, bool& showMobDesigner, bool& showInteractionEditor, bool& showToolDesigner, bool& showWeatherDesigner, bool& showSoundDesigner, bool& showAdvWorldEditor, bool& showTextureDesigner) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Esc")) {
                glfwSetWindowShouldClose(m_window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Designers")) {
            ImGui::MenuItem("Block Designer", nullptr, &showBlockDesigner);
            ImGui::MenuItem("Mob Designer", nullptr, &showMobDesigner);
            ImGui::MenuItem("Tool Designer", nullptr, &showToolDesigner);
            ImGui::MenuItem("Texture Designer", nullptr, &showTextureDesigner);
            ImGui::MenuItem("Weather Designer", nullptr, &showWeatherDesigner);
            ImGui::MenuItem("Sound Designer", nullptr, &showSoundDesigner);
            ImGui::Separator();
            if (ImGui::BeginMenu("Editors")) {
                ImGui::MenuItem("Sound Editor", nullptr, &showSoundEditor);
                ImGui::MenuItem("Game Engine Editor", nullptr, &showInteractionEditor);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Profiler", nullptr, &showProfiler);
            ImGui::MenuItem("Memory Inspector", nullptr, &showMemory);
            ImGui::MenuItem("ECS Editor", nullptr, &showECS);
            ImGui::MenuItem("World Editor", nullptr, &showWorldEditor);
            ImGui::MenuItem("Advanced World Editor", nullptr, &showAdvWorldEditor);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Settings", nullptr, &showSettings);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void GUIManager::showInteractionEditor(bool* open) {
    if (!*open) return;
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Game Engine Editor", open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("##GameEngineTabs")) {

        // ===== TAB 1: BLOCK MECHANICS =====
        if (ImGui::BeginTabItem("Block Mechanics")) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "BLOCK MECHANICS");
            ImGui::TextDisabled("Configure break time, tool requirements, and loot drops for each block type.");
            ImGui::Separator();
            ImGui::Spacing();

            auto& blocks = GameRegistry::getInstance().getAllBlocks();

            static uint8_t selBlockId = 1;
            ImGui::Text("Select Block:");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##BlockSel", blocks.count(selBlockId) ? blocks[selBlockId].name.c_str() : "?")) {
                for (auto& [id, def] : blocks) {
                    if (id == 0) continue;
                    bool sel = (id == selBlockId);
                    if (ImGui::Selectable(def.name.c_str(), sel)) selBlockId = id;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (blocks.count(selBlockId)) {
                auto& b = blocks[selBlockId];
                ImGui::Spacing();
                ImGui::Separator();

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Breaking");

                ImGui::DragFloat("Break Time (s)", &b.breakTime, 0.05f, 0.0f, 60.0f, "%.2f");
                ImGui::DragFloat("Blast Resistance", &b.blastResistance, 0.2f, 0.0f, 100.0f, "%.1f");

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Properties");

                ImGui::DragInt("Light Emission", &b.lightEmission, 0.15f, 0, 15);
                ImGui::Checkbox("Has Gravity", &b.hasGravity);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Tool Requirements");

                const char* toolTypes[] = {"None (Hand)", "pickaxe", "axe", "shovel", "hoe", "sword"};
                int toolIdx = 0;
                if (b.requiredToolType == "pickaxe") toolIdx = 1;
                else if (b.requiredToolType == "axe") toolIdx = 2;
                else if (b.requiredToolType == "shovel") toolIdx = 3;
                else if (b.requiredToolType == "hoe") toolIdx = 4;
                else if (b.requiredToolType == "sword") toolIdx = 5;
                if (ImGui::Combo("Required Tool", &toolIdx, toolTypes, 6)) {
                    b.requiredToolType = (toolIdx == 0) ? "" : toolTypes[toolIdx];
                }

                const char* tierNames[] = {"Hand (0)", "Wood (1)", "Stone (2)", "Iron (3)", "Diamond (4)"};
                ImGui::Combo("Min Tool Tier", &b.requiredToolTier, tierNames, 5);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Loot Drops");

                ImGui::Checkbox("Drops Itself", &b.dropsItself);
                if (!b.dropsItself) {
                    ImGui::Indent(10.0f);
                    for (int di = 0; di < (int)b.drops.size(); di++) {
                        ImGui::PushID(di);
                        auto& drop = b.drops[di];
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "Drop #%d", di + 1);
                        int dropBlockId = (int)drop.blockId;
                        ImGui::InputInt("Block ID", &dropBlockId);
                        drop.blockId = (uint8_t)std::clamp(dropBlockId, 0, 255);
                        ImGui::InputInt("Min", &drop.minCount);
                        ImGui::SameLine(); ImGui::InputInt("Max", &drop.maxCount);
                        ImGui::DragFloat("Chance", &drop.chance, 0.005f, 0.0f, 1.0f, "%.2f");
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
                        if (ImGui::SmallButton("Remove")) {
                            b.drops.erase(b.drops.begin() + di);
                            di--;
                        }
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                        ImGui::PopID();
                    }
                    ImGui::Unindent(10.0f);
                    ImGui::Spacing();
                    if (ImGui::Button("+ Add Drop", ImVec2(120, 24))) b.drops.push_back({0, 1, 1, 1.0f});
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Physics Properties");

                ImGui::DragFloat("Friction", &b.friction, 0.005f, 0.0f, 1.0f, "%.2f");
                ImGui::DragFloat("Slipperiness", &b.slipperiness, 0.005f, 0.0f, 1.0f, "%.2f");
                ImGui::DragInt("Opacity", &b.opacity, 0.15f, 0, 15);
                ImGui::Checkbox("Flammable", &b.flammable);
                if (b.flammable) {
                    ImGui::Indent(10.0f);
                    ImGui::DragInt("Burn Time (ticks)", &b.burnTime, 1.0f, 1, 600);
                    ImGui::Unindent(10.0f);
                }
                ImGui::Checkbox("Replaceable", &b.replaceable);
                ImGui::DragInt("Redstone Power", &b.redstonePower, 0.15f, 0, 15);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Block Interactions");
                ImGui::TextDisabled("Define what happens when specific neighbors are present.");

                for (int ii = 0; ii < (int)b.interactions.size(); ii++) {
                    ImGui::PushID(100 + ii);
                    auto& inter = b.interactions[ii];
                    ImGui::Spacing();

                    // Neighbor block combo
                    ImGui::Text("When:");
                    ImGui::SameLine();
                    if (ImGui::BeginCombo("##neighbor", blocks.count(inter.neighborBlockId) ? blocks[inter.neighborBlockId].name.c_str() : "?")) {
                        for (auto& [nid, ndef] : blocks) {
                            if (nid == 0) continue;
                            if (ImGui::Selectable(ndef.name.c_str(), inter.neighborBlockId == nid))
                                inter.neighborBlockId = nid;
                        }
                        ImGui::EndCombo();
                    }

                    // Condition combo
                    const char* conditions[] = {"adjacent", "above", "below"};
                    int condIdx = 0;
                    if (inter.condition == "above") condIdx = 1;
                    else if (inter.condition == "below") condIdx = 2;
                    ImGui::Text("Is:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    if (ImGui::Combo("##cond", &condIdx, conditions, 3)) inter.condition = conditions[condIdx];

                    // Result block combo
                    ImGui::Text("Becomes:");
                    ImGui::SameLine();
                    if (ImGui::BeginCombo("##result", blocks.count(inter.resultBlockId) ? blocks[inter.resultBlockId].name.c_str() : "?")) {
                        for (auto& [rid, rdef] : blocks) {
                            if (ImGui::Selectable(rdef.name.c_str(), inter.resultBlockId == rid))
                                inter.resultBlockId = rid;
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
                    if (ImGui::SmallButton("Remove##inter")) {
                        b.interactions.erase(b.interactions.begin() + ii);
                        ii--;
                    }
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                    ImGui::PopID();
                }
                if (ImGui::Button("+ Add Interaction", ImVec2(150, 24))) {
                    b.interactions.push_back({});
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Block States");

                ImGui::Checkbox("Has States", &b.hasStates);
                if (b.hasStates) {
                    ImGui::DragInt("State Count", &b.stateCount, 0.1f, 1, 4);
                    for (int si = 0; si < b.stateCount; si++) {
                        ImGui::PushID(200 + si);
                        char stateLabel[32];
                        snprintf(stateLabel, sizeof(stateLabel), "State %d", si);
                        char stateBuf[64];
                        strncpy(stateBuf, b.stateNames[si].c_str(), 63);
                        stateBuf[63] = '\0';
                        if (ImGui::InputText(stateLabel, stateBuf, 64)) b.stateNames[si] = stateBuf;
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndTabItem();
        }

        // ===== TAB 2: TOOL SYSTEM =====
        if (ImGui::BeginTabItem("Tool System")) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "TOOL SYSTEM");
            ImGui::TextDisabled("Quick editor for tools. For full editing with undo/redo, use the standalone Tool Designer.");
            ImGui::Separator();
            ImGui::Spacing();

            auto& tools = GameRegistry::getInstance().getAllTools();

            static int selToolId = 1;

            // Tool list
            ImGui::BeginChild("##ToolList", ImVec2(200, 0), true);
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "TOOLS (%zu)", tools.size());
            ImGui::Separator();
            for (auto& [id, tool] : tools) {
                bool sel = (id == selToolId);
                char label[128];
                snprintf(label, sizeof(label), "%s (T%d)", tool.name.c_str(), tool.tier);
                if (ImGui::Selectable(label, sel)) selToolId = id;
            }
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("+ New Tool", ImVec2(-1, 24))) {
                int newId = 100;
                while (tools.count(newId)) newId++;
                ToolDefinition t;
                t.id = newId;
                t.name = "New Tool";
                t.toolType = "pickaxe";
                t.tier = 1;
                GameRegistry::getInstance().registerTool(t);
                selToolId = newId;
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // Tool properties
            ImGui::BeginChild("##ToolProps", ImVec2(0, 0), true);
            if (tools.count(selToolId)) {
                auto& t = tools[selToolId];

                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "PROPERTIES");
                ImGui::Separator();
                ImGui::Spacing();

                char nameBuf[64];
                strncpy(nameBuf, t.name.c_str(), 63); nameBuf[63] = '\0';
                if (ImGui::InputText("Name", nameBuf, 64)) t.name = nameBuf;

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Classification");

                const char* tTypes[] = {"pickaxe", "axe", "shovel", "hoe", "sword"};
                int tIdx = 0;
                for (int i = 0; i < 5; i++) if (t.toolType == tTypes[i]) { tIdx = i; break; }
                if (ImGui::Combo("Tool Type", &tIdx, tTypes, 5)) t.toolType = tTypes[tIdx];

                const char* tierNames[] = {"Wood (1)", "Stone (2)", "Iron (3)", "Diamond (4)"};
                int tierIdx = std::clamp(t.tier - 1, 0, 3);
                if (ImGui::Combo("Tier", &tierIdx, tierNames, 4)) t.tier = tierIdx + 1;

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Stats");

                ImGui::DragFloat("Speed Multiplier", &t.speedMultiplier, 0.1f, 0.5f, 20.0f, "%.1f");
                ImGui::DragFloat("Damage", &t.damage, 0.1f, 0.0f, 20.0f, "%.1f");

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Appearance");
                ImGui::ColorEdit3("Color", &t.color.x, ImGuiColorEditFlags_Float);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
                if (ImGui::Button("Delete Tool", ImVec2(120, 24))) {
                    tools.erase(selToolId);
                    selToolId = tools.empty() ? -1 : tools.begin()->first;
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::TextDisabled("Select a tool from the list.");
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // ===== TAB 3: MOB AI =====
        if (ImGui::BeginTabItem("Mob AI")) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MOB AI BEHAVIOR");
            ImGui::TextDisabled("Visual node graph editor for mob AI state machines. Right-click to add nodes, drag pins to connect.");
            ImGui::Separator();
            ImGui::Spacing();

            auto& mobs = GameRegistry::getInstance().getAllMobs();

            static MobType selMobType = MOB_COW;
            ImGui::Text("Select Mob:");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##MobAISel", mobs.count(selMobType) ? mobs[selMobType].name.c_str() : "?")) {
                for (auto& [type, def] : mobs) {
                    bool sel = (type == selMobType);
                    if (ImGui::Selectable(def.name.c_str(), sel)) selMobType = type;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (mobs.count(selMobType)) {
                auto& mob = mobs[selMobType];
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Stats:");
                ImGui::SameLine();
                ImGui::Text("HP: %.0f | Speed: %.1f | %s | %s",
                    mob.maxHp, mob.speed,
                    mob.isAquatic ? "Aquatic" : "Land",
                    mob.isHostile ? "Hostile" : "Passive");
                ImGui::Spacing();

                ImGui::BeginChild("##MobAICanvas", ImVec2(0, 0), true);
                m_interactionAIEditor.show(mob.aiGraph);
                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }

        // ===== TAB 4: CONTROLS =====
        if (ImGui::BeginTabItem("Controls")) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CONTROLS & KEY BINDINGS");
            ImGui::TextDisabled("Reference for in-game controls and tool equipping.");
            ImGui::Separator();
            ImGui::Spacing();

            static int interactionRange = 5;
            ImGui::DragInt("Interaction Range", &interactionRange, 0.1f, 1, 10);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Key Bindings");
            ImGui::Spacing();

            ImGui::Columns(2, "##KeyBindCols", true);
            ImGui::SetColumnWidth(0, 200);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Action"); ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Key"); ImGui::NextColumn();
            ImGui::Separator();

            ImGui::Text("Break Block"); ImGui::NextColumn(); ImGui::Text("Left Click"); ImGui::NextColumn();
            ImGui::Text("Place Block"); ImGui::NextColumn(); ImGui::Text("Right Click"); ImGui::NextColumn();
            ImGui::Text("Interact with Mob"); ImGui::NextColumn(); ImGui::Text("E"); ImGui::NextColumn();
            ImGui::Text("Movement"); ImGui::NextColumn(); ImGui::Text("W / A / S / D"); ImGui::NextColumn();
            ImGui::Text("Jump / Fly Up"); ImGui::NextColumn(); ImGui::Text("Space"); ImGui::NextColumn();
            ImGui::Text("Sprint"); ImGui::NextColumn(); ImGui::Text("Shift"); ImGui::NextColumn();
            ImGui::Text("Toggle Fly"); ImGui::NextColumn(); ImGui::Text("Double Space"); ImGui::NextColumn();
            ImGui::Text("Toggle Inventory"); ImGui::NextColumn(); ImGui::Text("Tab"); ImGui::NextColumn();
            ImGui::Text("Hotbar Selection"); ImGui::NextColumn(); ImGui::Text("1 - 9"); ImGui::NextColumn();
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Tool Selector");
            ImGui::TextDisabled("Equip a tool to affect break speed and harvesting.");
            ImGui::Spacing();

            auto& tools = GameRegistry::getInstance().getAllTools();
            static int equippedTool = 0;
            if (ImGui::RadioButton("Hand (no tool)", equippedTool == 0)) equippedTool = 0;
            for (auto& [id, tool] : tools) {
                char label[128];
                snprintf(label, 128, "%s (T%d, x%.0f speed)", tool.name.c_str(), tool.tier, tool.speedMultiplier);
                if (ImGui::RadioButton(label, equippedTool == id)) equippedTool = id;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void GUIManager::showSettings(bool* open, bool& vsync, bool& wireframe, bool& fullscreen, bool& backfaceCulling, Renderer& renderer) {
    ImGui::Begin("Settings", open);

    if (ImGui::Checkbox("VSync", &vsync)) {
        renderer.setVSync(vsync);
    }

    if (ImGui::Checkbox("Wireframe", &wireframe)) {
        renderer.setWireframe(wireframe);
    }

    if (ImGui::Checkbox("Backface Culling", &backfaceCulling)) {
        renderer.setBackfaceCulling(backfaceCulling);
    }

    if (ImGui::Checkbox("Fullscreen", &fullscreen)) {
        renderer.setFullscreen(fullscreen);
    }

    ImGui::End();
}

void GUIManager::endFrame() {
    if (!m_initialized) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void GUIManager::coordinateMobBlockEdit(bool& showBlockDesigner) {
    // --- MobDesigner -> BlockDesigner flow ---
    if (m_mobDesigner.wantsBlockDesigner()) {
        m_mobDesigner.clearWantsBlockDesigner();
        int partIdx = m_mobDesigner.getEditingPartIndex();
        BlockDefinition partBlock = m_mobDesigner.getPartAsBlock(partIdx);
        // Pass the actual part shape to BlockDesigner
        auto& parts = m_mobDesigner.getMobParts();
        Vec3 shape = {1, 1, 1};
        if (partIdx >= 0 && partIdx < (int)parts.size())
            shape = parts[partIdx].size;
        m_blockDesigner.loadExternalBlock(partBlock, shape);
        showBlockDesigner = true;
    }

    // When user clicks "Done & Return", apply result without closing window
    if (m_mobDesigner.isEditingPartExternally() && m_blockDesigner.isExternalDone()) {
        BlockDefinition result = m_blockDesigner.getExternalResult();
        int partIdx = m_mobDesigner.getEditingPartIndex();
        m_mobDesigner.applyBlockToPart(partIdx, result);
        m_mobDesigner.finishExternalEdit();
        m_blockDesigner.exitExternalMode();
    }

    // --- ToolDesigner -> BlockDesigner flow ---
    if (m_toolDesigner.wantsBlockDesigner()) {
        m_toolDesigner.clearWantsBlockDesigner();
        int pieceIdx = m_toolDesigner.getEditingPieceIndex();
        BlockDefinition pieceBlock = m_toolDesigner.getPieceAsBlock(pieceIdx);
        // Pass the actual piece shape
        Vec3 shape = m_toolDesigner.getPieceShape(pieceIdx);
        m_blockDesigner.loadExternalBlock(pieceBlock, shape);
        showBlockDesigner = true;
    }

    // When user clicks "Done & Return", apply result without closing window
    if (m_toolDesigner.isEditingPieceExternally() && m_blockDesigner.isExternalDone()) {
        BlockDefinition result = m_blockDesigner.getExternalResult();
        int pieceIdx = m_toolDesigner.getEditingPieceIndex();
        m_toolDesigner.applyBlockToPiece(pieceIdx, result);
        m_toolDesigner.finishExternalEdit();
        m_blockDesigner.exitExternalMode();
    }
}

void GUIManager::shutdown() {
    if (!m_initialized) return;

    if (m_previewShader) { delete m_previewShader; m_previewShader = nullptr; }
    if (m_blockPreviewBuf) { delete m_blockPreviewBuf; m_blockPreviewBuf = nullptr; }
    if (m_mobPreviewBuf) { delete m_mobPreviewBuf; m_mobPreviewBuf = nullptr; }
    if (m_toolPreviewBuf) { delete m_toolPreviewBuf; m_toolPreviewBuf = nullptr; }
    if (m_partPreviewBuf) { delete m_partPreviewBuf; m_partPreviewBuf = nullptr; }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_window = nullptr;
    m_initialized = false;
}

void GUIManager::showMemoryInspector(size_t arenaOffset, size_t arenaSize, size_t poolUsed, size_t poolTotal) {
    if (!m_initialized) return;

    ImGui::Begin("Memory Inspector");

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        float usedMB = (float)pmc.PrivateUsage / (1024.0f * 1024.0f);
        ImGui::Text("Process RAM: %.1f MB", usedMB);
    }
#endif

    GLint totalVRAM = 0;
    GLint currentVRAM = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalVRAM);
    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &currentVRAM);
    if (totalVRAM > 0) {
        float totalMB = (float)totalVRAM / 1024.0f;
        float availableMB = (float)currentVRAM / 1024.0f;
        float usedMB = totalMB - availableMB;
        ImGui::Text("VRAM (NVIDIA): %.1f / %.1f MB", usedMB, totalMB);
        ImGui::ProgressBar(usedMB / totalMB, ImVec2(-1.0f, 0.0f));
    }

    ImGui::Separator();
    ImGui::Text("Custom Allocators:");

    const float arenaRatio = (arenaSize > 0) ? (float)((double)arenaOffset / (double)arenaSize) : 0.0f;
    ImGui::Text("Arena: %zu / %zu bytes", arenaOffset, arenaSize);
    ImGui::ProgressBar(std::clamp(arenaRatio, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f));

    const float poolRatio = (poolTotal > 0) ? (float)((double)poolUsed / (double)poolTotal) : 0.0f;
    ImGui::Text("Pool: %zu / %zu objects", poolUsed, poolTotal);
    ImGui::ProgressBar(std::clamp(poolRatio, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f));

    ImGui::End();
}

void GUIManager::showECSEditor() {
    if (!m_initialized) return;

    ImGui::Begin("ECS Editor");
    ImGui::TextUnformatted("(Scaffold) Entity/component editing hooks go here.");
    ImGui::Separator();
    ImGui::Text("Entities: %d", 1);

    if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float pos[3] = {0.0f, 0.0f, 0.0f};
        ImGui::InputFloat3("Position", pos);
    }

    ImGui::End();
}
