#include "GUIManager.hpp"
#include "Renderer.hpp"
#include "Framebuffer.hpp"
#include "Shader.hpp"
#include "AudioManager.hpp"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

    // VSync is managed exclusively by Renderer::setVSync() — do not call
    // glfwSwapInterval here or it will silently override the main-loop setting
    // and cap the frame rate (was the cause of the 30 FPS cap).
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

    // ── Geometry & spacing ─────────────────────────────────────────────────
    style.WindowPadding       = ImVec2(16, 14);
    style.FramePadding        = ImVec2(9,  5);
    style.CellPadding         = ImVec2(7,  4);
    style.ItemSpacing         = ImVec2(9,  6);
    style.ItemInnerSpacing    = ImVec2(6,  4);
    style.IndentSpacing       = 20.0f;
    style.ScrollbarSize       = 10.0f;
    style.GrabMinSize         = 10.0f;
    style.SeparatorTextPadding = ImVec2(20, 5);

    // ── Rounding — pronounced curves for a clean product feel ─────────────
    style.WindowRounding      = 8.0f;
    style.ChildRounding       = 5.0f;
    style.FrameRounding       = 5.0f;
    style.PopupRounding       = 6.0f;
    style.ScrollbarRounding   = 6.0f;
    style.GrabRounding        = 4.0f;
    style.TabRounding         = 5.0f;

    // ── Borders — minimal, surface-only ────────────────────────────────────
    style.WindowBorderSize    = 1.0f;
    style.ChildBorderSize     = 1.0f;
    style.PopupBorderSize     = 1.0f;
    style.FrameBorderSize     = 0.0f;
    style.TabBorderSize       = 0.0f;

    // Centered window titles
    style.WindowTitleAlign    = ImVec2(0.5f, 0.5f);

    // ── Palette ────────────────────────────────────────────────────────────
    //   Background tiers  (dark → darkest for depth illusion)
    //     bg0  #0E0E10   main window bg
    //     bg1  #16161A   child/panel bg
    //     bg2  #1E1E24   frames, input fields
    //     bg3  #242430   elevated surfaces (title bar active)
    //   Accent:  Cornflower #5B9BD5  (softer but vivid blue)
    //   Accent+: #7AB8F0  (hover / highlight)
    //   Accent-: #3A6A9E  (pressed / dim)
    const ImVec4 accent    = ImVec4(0.357f, 0.608f, 0.835f, 1.00f);  // #5B9BD5
    const ImVec4 accentDim = ImVec4(0.227f, 0.416f, 0.620f, 1.00f);  // #3A6A9E
    const ImVec4 accentHi  = ImVec4(0.478f, 0.722f, 0.941f, 1.00f);  // #7AB8F0

    // ── Text ───────────────────────────────────────────────────────────────
    colors[ImGuiCol_Text]              = ImVec4(0.918f, 0.925f, 0.941f, 1.00f); // #EAECF0
    colors[ImGuiCol_TextDisabled]      = ImVec4(0.376f, 0.400f, 0.451f, 1.00f); // #606673

    // ── Backgrounds ────────────────────────────────────────────────────────
    colors[ImGuiCol_WindowBg]          = ImVec4(0.055f, 0.055f, 0.063f, 1.00f); // #0E0E10
    colors[ImGuiCol_ChildBg]           = ImVec4(0.086f, 0.086f, 0.102f, 1.00f); // #16161A
    colors[ImGuiCol_PopupBg]           = ImVec4(0.075f, 0.075f, 0.090f, 0.98f); // #131316
    colors[ImGuiCol_MenuBarBg]         = ImVec4(0.051f, 0.051f, 0.067f, 1.00f); // #0D0D11

    // ── Borders ────────────────────────────────────────────────────────────
    colors[ImGuiCol_Border]            = ImVec4(0.176f, 0.188f, 0.224f, 0.70f); // #2D3039
    colors[ImGuiCol_BorderShadow]      = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);

    // ── Frames ─────────────────────────────────────────────────────────────
    colors[ImGuiCol_FrameBg]           = ImVec4(0.118f, 0.118f, 0.141f, 1.00f); // #1E1E24
    colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.153f, 0.157f, 0.188f, 1.00f); // #272730
    colors[ImGuiCol_FrameBgActive]     = ImVec4(0.094f, 0.098f, 0.118f, 1.00f); // #18181E

    // ── Title bars ─────────────────────────────────────────────────────────
    colors[ImGuiCol_TitleBg]           = ImVec4(0.043f, 0.043f, 0.055f, 1.00f); // #0B0B0E
    colors[ImGuiCol_TitleBgActive]     = ImVec4(0.090f, 0.157f, 0.259f, 1.00f); // #172842
    colors[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.039f, 0.039f, 0.051f, 0.80f);

    // ── Scrollbars ─────────────────────────────────────────────────────────
    colors[ImGuiCol_ScrollbarBg]       = ImVec4(0.055f, 0.059f, 0.075f, 0.90f);
    colors[ImGuiCol_ScrollbarGrab]     = ImVec4(0.220f, 0.235f, 0.275f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.290f, 0.310f, 0.360f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = accent;

    // ── Checkmarks / sliders ──────────────────────────────────────────────
    colors[ImGuiCol_CheckMark]         = accentHi;
    colors[ImGuiCol_SliderGrab]        = accent;
    colors[ImGuiCol_SliderGrabActive]  = accentHi;

    // ── Buttons ────────────────────────────────────────────────────────────
    colors[ImGuiCol_Button]            = ImVec4(0.133f, 0.153f, 0.196f, 1.00f); // #222732
    colors[ImGuiCol_ButtonHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.88f);
    colors[ImGuiCol_ButtonActive]      = accentDim;

    // ── Headers ────────────────────────────────────────────────────────────
    colors[ImGuiCol_Header]            = ImVec4(0.110f, 0.133f, 0.173f, 1.00f);
    colors[ImGuiCol_HeaderHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.50f);
    colors[ImGuiCol_HeaderActive]      = ImVec4(accent.x, accent.y, accent.z, 0.78f);

    // ── Separators ────────────────────────────────────────────────────────
    colors[ImGuiCol_Separator]         = ImVec4(0.165f, 0.180f, 0.216f, 0.85f);
    colors[ImGuiCol_SeparatorHovered]  = ImVec4(accent.x, accent.y, accent.z, 0.65f);
    colors[ImGuiCol_SeparatorActive]   = accentHi;

    // ── Resize grips ──────────────────────────────────────────────────────
    colors[ImGuiCol_ResizeGrip]        = ImVec4(accent.x, accent.y, accent.z, 0.14f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.52f);
    colors[ImGuiCol_ResizeGripActive]  = ImVec4(accent.x, accent.y, accent.z, 0.88f);

    // ── Tabs ──────────────────────────────────────────────────────────────
    colors[ImGuiCol_Tab]               = ImVec4(0.071f, 0.075f, 0.094f, 1.00f);
    colors[ImGuiCol_TabHovered]        = ImVec4(accent.x, accent.y, accent.z, 0.60f);
    colors[ImGuiCol_TabActive]         = ImVec4(0.110f, 0.176f, 0.282f, 1.00f); // accent-tinted
    colors[ImGuiCol_TabUnfocused]      = ImVec4(0.059f, 0.063f, 0.082f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]= ImVec4(0.094f, 0.114f, 0.149f, 1.00f);

    // ── Docking ───────────────────────────────────────────────────────────
    colors[ImGuiCol_DockingPreview]    = ImVec4(accent.x, accent.y, accent.z, 0.50f);
    colors[ImGuiCol_DockingEmptyBg]    = ImVec4(0.039f, 0.043f, 0.055f, 1.00f);

    // ── Plots ─────────────────────────────────────────────────────────────
    colors[ImGuiCol_PlotLines]         = ImVec4(0.478f, 0.655f, 0.835f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]  = accentHi;
    colors[ImGuiCol_PlotHistogram]     = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accentHi;

    // ── Tables ────────────────────────────────────────────────────────────
    colors[ImGuiCol_TableHeaderBg]     = ImVec4(0.094f, 0.110f, 0.145f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.176f, 0.196f, 0.235f, 1.00f);
    colors[ImGuiCol_TableBorderLight]  = ImVec4(0.130f, 0.149f, 0.188f, 1.00f);
    colors[ImGuiCol_TableRowBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]     = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    // ── Misc ──────────────────────────────────────────────────────────────
    colors[ImGuiCol_TextSelectedBg]    = ImVec4(accent.x, accent.y, accent.z, 0.32f);
    colors[ImGuiCol_DragDropTarget]    = ImVec4(accent.x, accent.y, accent.z, 0.88f);
    colors[ImGuiCol_NavHighlight]      = accentHi;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.45f);
    colors[ImGuiCol_ModalWindowDimBg]  = ImVec4(0.00f, 0.00f, 0.00f, 0.65f);

    // Multi-viewport: no rounding on OS-level windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void GUIManager::showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS, bool& showWorldEditor, bool& showSettings, bool& showSoundEditor, bool& showBlockDesigner, bool& showMobDesigner, bool& showInteractionEditor, bool& showToolDesigner, bool& showWeatherDesigner, bool& showSoundDesigner, bool& showAdvWorldEditor, bool& showTextureDesigner) {

    // ── Taller, more refined bar ───────────────────────────────────────────────
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(8.0f, 11.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(10.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg,        ImVec4(0.051f, 0.051f, 0.067f, 1.00f)); // match applyTheme
    ImGui::PushStyleColor(ImGuiCol_PopupBg,          ImVec4(0.075f, 0.075f, 0.090f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,    ImVec4(0.357f, 0.608f, 0.835f, 0.35f)); // accent hover
    ImGui::PushStyleColor(ImGuiCol_Header,           ImVec4(0.110f, 0.176f, 0.282f, 1.00f)); // accent-tinted

    const bool barOpen = ImGui::BeginMainMenuBar();

    // Pop style immediately after Begin so inner menus inherit default style
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    if (!barOpen)
        return;

    // ── Mode-tinted accent strip (bottom of menu bar) ─────────────────────
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();
        float w  = ImGui::GetWindowWidth();
        float h  = ImGui::GetWindowHeight();
        // Colour palette: blue = Creative, amber = Survival
        const bool  crtv = m_isCreativeMode;
        const ImU32 cEdge = crtv ? IM_COL32(30,  65, 120, 70)  : IM_COL32(110, 70, 10, 70);
        const ImU32 cPeak = crtv ? IM_COL32(91, 155, 213, 175) : IM_COL32(230,150, 30, 175);
        // 3-px bar: fade in from left → peak at centre → fade out to right
        dl->AddRectFilledMultiColor(
            ImVec2(p.x,         p.y + h - 3.0f),
            ImVec2(p.x + w*0.5f, p.y + h),
            cEdge, cPeak, cPeak, cEdge);
        dl->AddRectFilledMultiColor(
            ImVec2(p.x + w*0.5f, p.y + h - 3.0f),
            ImVec2(p.x + w,      p.y + h),
            cPeak, cEdge, cEdge, cPeak);
        // 1-px specular highlight at very top of bar for depth
        dl->AddLine(ImVec2(p.x, p.y + 1.0f), ImVec2(p.x + w, p.y + 1.0f),
                    IM_COL32(255, 255, 255, 9), 1.0f);
    }

    // ── Left brand badge — filled pill ───────────────────────────────────────
    {
        const char* brandText = "VSA";
        ImVec2 textSz  = ImGui::CalcTextSize(brandText);
        const float pH = 6.0f, pW = 13.0f;  // pill inner padding
        // Grab cursor in screen space before we draw
        ImVec2 origin  = ImGui::GetCursorScreenPos();
        float  pillY   = origin.y + (ImGui::GetFrameHeight() - textSz.y - pH * 2.0f) * 0.5f;
        ImVec2 pillMin(origin.x, pillY);
        ImVec2 pillMax(pillMin.x + textSz.x + pW * 2.0f, pillMin.y + textSz.y + pH * 2.0f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // Filled pill — gradient (top half lighter)
        ImVec2 pillMid(pillMin.x, (pillMin.y + pillMax.y) * 0.5f);
        dl->AddRectFilledMultiColor(pillMin, ImVec2(pillMax.x, pillMid.y),
            IM_COL32(52, 98, 158, 220), IM_COL32(52, 98, 158, 220),
            IM_COL32(38, 78, 128, 210), IM_COL32(38, 78, 128, 210));
        dl->AddRectFilled(ImVec2(pillMin.x, pillMid.y), pillMax, IM_COL32(38, 78, 128, 210), 0.0f);
        // Round corners on top half too
        dl->AddRectFilled(pillMin, pillMax, IM_COL32(0,0,0,0), 7.0f);
        dl->AddRectFilled(pillMin, pillMax, IM_COL32(38, 78, 128, 210), 7.0f);
        // Accent border ring
        dl->AddRect(pillMin, pillMax, IM_COL32(91, 155, 213, 160), 7.0f, 0, 1.2f);
        // Text on top
        dl->AddText(ImVec2(pillMin.x + pW, pillMin.y + pH),
                    IM_COL32(200, 228, 255, 255), brandText);
        // Claim the layout space so SameLine knows where we ended
        ImGui::Dummy(ImVec2(textSz.x + pW * 2.0f, ImGui::GetFrameHeight()));
    }
    ImGui::SameLine(0.0f, 14.0f);

    // ── Thin vertical separator ───────────────────────────────────────────────
    {
        float sepH = ImGui::GetFrameHeight() * 0.65f;
        float cy   = ImGui::GetCursorScreenPos().y + (ImGui::GetFrameHeight() - sepH) * 0.5f;
        float cx   = ImGui::GetCursorScreenPos().x;
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(cx, cy), ImVec2(cx, cy + sepH),
            IM_COL32(80, 100, 140, 140), 1.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 12.0f);
    }

    // ── File ─────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save World", "Ctrl+S")) { /* hooked in main loop */ }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Save the current world to disk");
        ImGui::Separator();
        if (ImGui::MenuItem("Preferences", "Ctrl+P")) { showSettings = true; }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Display, graphics and control settings");
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4"))
            glfwSetWindowShouldClose(m_window, true);
        ImGui::EndMenu();
    }

    // ── World ──────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("World")) {
        ImGui::SeparatorText("Terrain");
        ImGui::MenuItem("World Editor", "Ctrl+W", &showWorldEditor);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("New world, save/load, seed and terrain settings");
        ImGui::MenuItem("Advanced Editor", nullptr, &showAdvWorldEditor);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Fine-tune biomes, erosion, ore distribution and structures");
        ImGui::EndMenu();
    }

    // ── Create ────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Create")) {
        ImGui::SeparatorText("Blocks & Mobs");
        ImGui::MenuItem("Block Designer",   "B", &showBlockDesigner);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Create and edit custom block types, textures and properties");
        ImGui::MenuItem("Mob Designer",     "M", &showMobDesigner);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Build mob meshes, animations and behaviour trees");
        ImGui::MenuItem("Tool Designer",    nullptr, &showToolDesigner);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Define tools, their stats and breaking properties");
        ImGui::MenuItem("Texture Designer", nullptr, &showTextureDesigner);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Paint and manage block / mob texture atlases");
        ImGui::SeparatorText("Environment");
        ImGui::MenuItem("Weather Designer", nullptr, &showWeatherDesigner);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Control precipitation, wind and ambient particle effects");
        ImGui::MenuItem("Sound Designer",   nullptr, &showSoundDesigner);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Compose and layer ambient and block sounds");
        ImGui::EndMenu();
    }

    // ── Engine ────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Engine")) {
        ImGui::SeparatorText("Gameplay");
        ImGui::MenuItem("Game Engine Editor", "G", &showInteractionEditor);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Block mechanics, tool system, mob AI and control bindings");
        ImGui::MenuItem("Sound Editor",       nullptr, &showSoundEditor);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Mix and preview in-game sound effects");
        ImGui::EndMenu();
    }

    // ── Debug ─────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Debug")) {
        ImGui::SeparatorText("Diagnostics");
        ImGui::MenuItem("Profiler",         "F2", &showProfiler);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Frame timing, GPU and CPU usage graph");
        ImGui::MenuItem("Memory Inspector", "F3", &showMemory);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Arena allocator and chunk pool utilisation");
        ImGui::MenuItem("ECS Inspector",    "F4", &showECS);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Entity / Component / System live viewer");
        ImGui::EndMenu();
    }

    // ── Help ──────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Open Help & Reference..."))
            m_helperWindow.open();
        ImGui::Separator();
        ImGui::SeparatorText("Quick Controls");
        ImGui::TextDisabled("W A S D / Arrows"); ImGui::SameLine(190); ImGui::TextDisabled("Move");
        ImGui::TextDisabled("Space");            ImGui::SameLine(190); ImGui::TextDisabled("Jump / Fly Up");
        ImGui::TextDisabled("Dbl Space");        ImGui::SameLine(190); ImGui::TextDisabled("Toggle Fly");
        ImGui::TextDisabled("LMB / RMB");        ImGui::SameLine(190); ImGui::TextDisabled("Break / Place Block");
        ImGui::TextDisabled("E");                ImGui::SameLine(190); ImGui::TextDisabled("Inventory");
        ImGui::TextDisabled("F10");              ImGui::SameLine(190); ImGui::TextDisabled("Toggle Menu / World");
        ImGui::TextDisabled("Esc");              ImGui::SameLine(190); ImGui::TextDisabled("Game Menu");
        ImGui::Separator();
        ImGui::TextDisabled("Voxel-Sim Architect  V0.6 Beta");
        ImGui::TextDisabled("Build: " __DATE__ "  " __TIME__);
        ImGui::EndMenu();
    }

    // ══════════════════════════════════════════════════════════════════════════
    // Redesigned Quick-Launch Toolbar — 5 colour-coded groups, fully labelled.
    // Groups: WORLD | DESIGN | ENVIRON | ENGINE | DEBUG+SETTINGS
    // Each group has a small dim category label followed by pill buttons.
    // Groups are divided by thicker tinted separators for clear visual zones.
    // ══════════════════════════════════════════════════════════════════════════
    {
        ImGui::SameLine(0.0f, 16.0f);

        const float kBtnH      = ImGui::GetFrameHeight() * 0.80f;
        const float kSepMargin = 10.0f;

        // Thicker, colour-tinted separator between groups
        auto drawGroupSep = [&](ImU32 col) {
            float sepH = ImGui::GetFrameHeight() * 0.70f;
            float cy   = ImGui::GetCursorScreenPos().y + (ImGui::GetFrameHeight() - sepH) * 0.5f;
            float cx   = ImGui::GetCursorScreenPos().x + kSepMargin * 0.5f;
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cx, cy), ImVec2(cx, cy + sepH), col, 2.0f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kSepMargin);
        };

        // Small group label rendered inline before the buttons of each group
        auto drawGroupLabel = [&](const char* txt, ImU32 col) {
            ImVec2 cp   = ImGui::GetCursorScreenPos();
            float  barH = ImGui::GetFrameHeight();
            float  ty   = cp.y + barH - ImGui::GetTextLineHeightWithSpacing() * 1.30f;
            ImGui::GetWindowDrawList()->AddText(ImVec2(cp.x, ty), col, txt);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                 ImGui::CalcTextSize(txt).x + 4.0f);
        };

        // Toggle pill button: filled when active, ghost when inactive
        auto qlBtn = [&](const char* label,
                         float r, float g, float b,
                         bool active) -> bool {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, active ? 1.0f : 0.0f);
            float textW = ImGui::CalcTextSize(label).x;
            float btnW  = textW + ImGui::GetStyle().FramePadding.x * 2.0f + 14.0f;
            if (active) {
                float tr = std::min(r * 1.6f, 1.f), tg = std::min(g * 1.6f, 1.f), tb = std::min(b * 1.6f, 1.f);
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(r*0.45f, g*0.45f, b*0.45f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r*0.65f, g*0.65f, b*0.65f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(r*0.32f, g*0.32f, b*0.32f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(std::min(r*1.2f,1.f), std::min(g*1.2f,1.f), std::min(b*1.2f,1.f), 0.75f));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(tr, tg, tb, 1.00f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(r*0.07f, g*0.07f, b*0.07f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r*0.22f, g*0.22f, b*0.22f, 0.95f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(r*0.38f, g*0.38f, b*0.38f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.46f, 0.50f, 0.60f, 1.00f));
            }
            bool clicked = ImGui::Button(label, ImVec2(btnW, kBtnH));
            ImGui::PopStyleColor(5);
            ImGui::PopStyleVar(2);
            return clicked;
        };

        auto intra = [&]() { ImGui::SameLine(0.0f, 3.0f); };  // tight gap between same-group buttons

        // ───────────────────── WORLD ──────────────────────────────────────────
        drawGroupLabel("WORLD", IM_COL32(100, 158, 215, 175));

        if (qlBtn("World##ql",     0.36f, 0.61f, 0.84f, showWorldEditor))
            showWorldEditor = !showWorldEditor;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("World Editor  [Ctrl+W]\nTerrain generation, seed & save/load");

        intra();
        if (qlBtn("Adv.World##ql", 0.30f, 0.52f, 0.78f, showAdvWorldEditor))
            showAdvWorldEditor = !showAdvWorldEditor;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Advanced World Editor\nBiomes, erosion, ore distribution & structures");

        ImGui::SameLine(0.0f, kSepMargin);
        drawGroupSep(IM_COL32(80, 130, 200, 200));

        // ───────────────────── DESIGN ─────────────────────────────────────────
        drawGroupLabel("DESIGN", IM_COL32(172, 115, 242, 175));

        if (qlBtn("Blocks##ql",   0.62f, 0.40f, 0.88f, showBlockDesigner))
            showBlockDesigner = !showBlockDesigner;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Block Designer  [B]\nCreate and edit custom block types");

        intra();
        if (qlBtn("Mobs##ql",     0.62f, 0.40f, 0.88f, showMobDesigner))
            showMobDesigner = !showMobDesigner;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Mob Designer  [M]\nBuild mob meshes, animations and AI");

        intra();
        if (qlBtn("Tools##ql",    0.52f, 0.34f, 0.80f, showToolDesigner))
            showToolDesigner = !showToolDesigner;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Tool Designer\nDefine tools, stats and breaking properties");

        intra();
        if (qlBtn("Textures##ql", 0.52f, 0.34f, 0.80f, showTextureDesigner))
            showTextureDesigner = !showTextureDesigner;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Texture Designer\nPaint and manage block / mob texture atlases");

        ImGui::SameLine(0.0f, kSepMargin);
        drawGroupSep(IM_COL32(150, 90, 222, 200));

        // ───────────────────── ENVIRON ────────────────────────────────────────
        drawGroupLabel("ENVIRON", IM_COL32(70, 205, 182, 175));

        if (qlBtn("Weather##ql",  0.22f, 0.72f, 0.64f, showWeatherDesigner))
            showWeatherDesigner = !showWeatherDesigner;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Weather Designer\nPrecipitation, wind and ambient particles");

        intra();
        if (qlBtn("Sounds##ql",   0.22f, 0.72f, 0.64f, showSoundDesigner))
            showSoundDesigner = !showSoundDesigner;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Sound Designer\nCompose and layer ambient and block sounds");

        ImGui::SameLine(0.0f, kSepMargin);
        drawGroupSep(IM_COL32(50, 185, 162, 200));

        // ───────────────────── ENGINE ─────────────────────────────────────────
        drawGroupLabel("ENGINE", IM_COL32(233, 172, 45, 175));

        if (qlBtn("Engine##ql",  0.90f, 0.62f, 0.20f, showInteractionEditor))
            showInteractionEditor = !showInteractionEditor;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Game Engine Editor  [G]\nBlock mechanics, mob AI and control bindings");

        intra();
        if (qlBtn("Sfx Ed.##ql",  0.82f, 0.55f, 0.15f, showSoundEditor))
            showSoundEditor = !showSoundEditor;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Sound Editor\nMix and preview in-game sound effects");

        ImGui::SameLine(0.0f, kSepMargin);
        drawGroupSep(IM_COL32(215, 155, 28, 200));

        // ───────────────────── DEBUG + SETTINGS ──────────────────────────────
        drawGroupLabel("DEBUG", IM_COL32(75, 215, 128, 175));

        if (qlBtn("Profiler##ql", 0.28f, 0.82f, 0.50f, showProfiler))
            showProfiler = !showProfiler;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Performance Profiler  [F2]\nFrame timing, GPU and CPU usage graph");

        intra();
        if (qlBtn("Memory##ql",   0.28f, 0.82f, 0.50f, showMemory))
            showMemory = !showMemory;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Memory Inspector  [F3]\nArena allocator and chunk pool utilisation");

        intra();
        if (qlBtn("ECS##ql",      0.28f, 0.82f, 0.50f, showECS))
            showECS = !showECS;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("ECS Inspector  [F4]\nEntity / Component / System live viewer");

        intra();
        drawGroupSep(IM_COL32(50, 185, 100, 200));
        if (qlBtn("Settings##ql", 0.55f, 0.58f, 0.68f, showSettings))
            showSettings = !showSettings;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Preferences  [Ctrl+P]\nDisplay, graphics and control settings");
    }

    // ── Right-aligned status / info bar ──────────────────────────────────────
    {
        const float fps     = ImGui::GetIO().Framerate;
        const float frameMs = (fps > 0.0f) ? 1000.0f / fps : 0.0f;

        // ── Compute text buffers ─────────────────────────────────────────────
        // Player position — compact signed-integer format
        char posBuf[48];
        snprintf(posBuf, sizeof(posBuf), "%d / %d / %d",
                 (int)m_playerX, (int)m_playerY, (int)m_playerZ);

        // Game mode badge: "CRTV" (Creative) or "SURV" (Survival)
        const char* modeBuf  = m_isCreativeMode ? "CRTV" : "SURV";
        const ImVec4 modeCol = m_isCreativeMode
            ? ImVec4(0.30f, 0.70f, 0.98f, 1.00f)  // cyan-blue for Creative
            : ImVec4(0.98f, 0.70f, 0.20f, 1.00f);  // amber for Survival

        // Player level
        char levelBuf[16];
        snprintf(levelBuf, sizeof(levelBuf), "Lv.%u", m_playerLevel);

        // HP — colour shifts red when low
        char hpBuf[16];
        snprintf(hpBuf, sizeof(hpBuf), "HP:%.0f", m_playerHp);
        const ImVec4 hpCol = (m_playerHp > 10.0f) ? ImVec4(0.90f, 0.35f, 0.35f, 1.00f)
                           : (m_playerHp > 5.0f)  ? ImVec4(1.00f, 0.60f, 0.10f, 1.00f)
                                                   : ImVec4(1.00f, 0.15f, 0.15f, 1.00f);

        // Oxygen — only show when drowning (< 20)
        char oxyBuf[16] = "";
        bool showOxy = (m_playerOxygen < 19.5f);
        if (showOxy)
            snprintf(oxyBuf, sizeof(oxyBuf), "O2:%.0f", m_playerOxygen);

        // Biome name (empty string → hidden)
        const bool showBiome  = !m_biomeName.empty();
        const bool showChunks = (m_loadedChunks > 0);

        char chunkBuf[24];
        if (showChunks)
            snprintf(chunkBuf, sizeof(chunkBuf), "%d ch", m_loadedChunks);

        // Count open panels
        int openCount = (showProfiler ? 1 : 0) + (showMemory ? 1 : 0) + (showECS ? 1 : 0)
                      + (showWorldEditor ? 1 : 0) + (showAdvWorldEditor ? 1 : 0)
                      + (showBlockDesigner ? 1 : 0) + (showMobDesigner ? 1 : 0)
                      + (showToolDesigner ? 1 : 0) + (showTextureDesigner ? 1 : 0)
                      + (showWeatherDesigner ? 1 : 0) + (showSoundDesigner ? 1 : 0)
                      + (showSoundEditor ? 1 : 0) + (showInteractionEditor ? 1 : 0)
                      + (showSettings ? 1 : 0);

        // FPS colour: emerald ≥60 / lime 60>x≥45 / amber ≥30 / red <30
        const ImVec4 fpsCol = (fps >= 60.f) ? ImVec4(0.30f, 0.90f, 0.48f, 1.00f)
                            : (fps >= 45.f) ? ImVec4(0.54f, 0.88f, 0.30f, 1.00f)
                            : (fps >= 30.f) ? ImVec4(0.96f, 0.76f, 0.15f, 1.00f)
                                            : ImVec4(0.96f, 0.28f, 0.28f, 1.00f);

        char fpsBuf[32], msBuf[24], panelBuf[32];
        snprintf(fpsBuf,   sizeof(fpsBuf),   "%.0f FPS", fps);
        snprintf(msBuf,    sizeof(msBuf),    "%.1f ms",  frameMs);
        if (openCount > 0)
            snprintf(panelBuf, sizeof(panelBuf), "%d open", openCount);
        else
            panelBuf[0] = '\0';

        constexpr const char* kVersion = "V0.6 Beta";
        const float spx = ImGui::GetStyle().ItemSpacing.x;

        // ── Measure total width for right-align ──────────────────────────────
        float totalW = 0.0f;

        // Mode badge
        totalW += ImGui::CalcTextSize(modeBuf).x + spx + 22.0f;  // +chip padding
        // Level badge
        totalW += ImGui::CalcTextSize(levelBuf).x + spx;
        // HP
        totalW += ImGui::CalcTextSize(hpBuf).x + spx + 22.0f;    // +chip padding
        // Oxygen (conditional)
        if (showOxy) totalW += ImGui::CalcTextSize(oxyBuf).x + spx;
        // World name badge (shown in status between HP and XYZ)
        const bool showWorldN = !m_worldName.empty();
        if (showWorldN) totalW += 10.0f + ImGui::CalcTextSize(m_worldName.c_str()).x + 10.0f + spx + 16.0f;
        // Position block
        float posLabelW = ImGui::CalcTextSize("XYZ").x;
        float posValW   = ImGui::CalcTextSize(posBuf).x;
        totalW += posLabelW + 4.0f + posValW + spx + 16.0f; // +sep

        // Biome badge
        if (showBiome)
            totalW += ImGui::CalcTextSize(m_biomeName.c_str()).x + spx + 16.0f;

        // Chunk count
        if (showChunks)
            totalW += ImGui::CalcTextSize(chunkBuf).x + spx + 16.0f;

        // Panel count
        if (panelBuf[0])
            totalW += ImGui::CalcTextSize(panelBuf).x + spx + 16.0f;

        // FPS + ms
        totalW += ImGui::CalcTextSize(fpsBuf).x + 4.0f + ImGui::CalcTextSize(msBuf).x + spx + 16.0f;

        // Version
        totalW += ImGui::CalcTextSize(kVersion).x + spx + 18.0f; // right margin

        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - totalW);

        // Helper: inline thin bar separator
        auto inlineSep = [&]() {
            ImGui::SameLine(0.0f, 8.0f);
            float sepH = ImGui::GetFrameHeight() * 0.45f;
            float cy   = ImGui::GetCursorScreenPos().y + (ImGui::GetFrameHeight() - sepH) * 0.5f;
            float cx   = ImGui::GetCursorScreenPos().x;
            ImGui::GetWindowDrawList()->AddLine(ImVec2(cx, cy), ImVec2(cx, cy + sepH),
                IM_COL32(55, 70, 100, 130), 1.0f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
        };

        // ── Game Mode badge — coloured chip ────────────────────────────────────
        {
            ImVec2 cp   = ImGui::GetCursorScreenPos();
            float  tw   = ImGui::CalcTextSize(modeBuf).x;
            const float chPadX = 6.0f, chPadY = 2.0f;
            float  barH = ImGui::GetFrameHeight();
            float  chipY = cp.y + (barH - ImGui::GetTextLineHeight() - chPadY*2.0f)*0.5f;
            ImU32  chipBg = m_isCreativeMode ? IM_COL32(28,55,105,150) : IM_COL32(105,60,15,150);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(cp.x - chPadX, chipY),
                ImVec2(cp.x + tw + chPadX, chipY + ImGui::GetTextLineHeight() + chPadY*2.0f),
                chipBg, 3.5f);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, modeCol);
        ImGui::TextUnformatted(modeBuf);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip(m_isCreativeMode ? "Creative Mode" : "Survival Mode");

        // ── Player Level ──────────────────────────────────────────────────────
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.88f, 0.25f, 1.00f));
        ImGui::TextUnformatted(levelBuf);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Player level — gain XP by mining blocks");

        // ── HP — coloured chip ────────────────────────────────────────────────
        ImGui::SameLine(0.0f, 6.0f);
        {
            ImVec2 cp   = ImGui::GetCursorScreenPos();
            float  tw   = ImGui::CalcTextSize(hpBuf).x;
            const float chPadX = 6.0f, chPadY = 2.0f;
            float  barH = ImGui::GetFrameHeight();
            float  chipY = cp.y + (barH - ImGui::GetTextLineHeight() - chPadY*2.0f)*0.5f;
            ImU32  chipBg = (m_playerHp > 10.0f) ? IM_COL32(90,20,20,130)
                          : (m_playerHp >  5.0f) ? IM_COL32(110,55,10,150)
                                                  : IM_COL32(130,15,15,170);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(cp.x - chPadX, chipY),
                ImVec2(cp.x + tw + chPadX, chipY + ImGui::GetTextLineHeight() + chPadY*2.0f),
                chipBg, 3.5f);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, hpCol);
        ImGui::TextUnformatted(hpBuf);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Player health (%.1f / 20)", (double)m_playerHp);

        // ── Oxygen (drowning indicator) ────────────────────────────────────────
        if (showOxy) {
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.75f, 1.00f, 1.00f));
            ImGui::TextUnformatted(oxyBuf);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Oxygen (%.0f / 20) — move upward or surface to breathe", (double)m_playerOxygen);
        }

        // ── World name badge ──────────────────────────────────────────────────
        if (showWorldN) {
            inlineSep();
            // Subtle tinted pill background
            {
                ImVec2 cp    = ImGui::GetCursorScreenPos();
                float  tw    = ImGui::CalcTextSize(m_worldName.c_str()).x;
                float  padX  = 5.0f, padY = 2.0f;
                float  barH  = ImGui::GetFrameHeight();
                float  chipY = cp.y + (barH - ImGui::GetTextLineHeight() - padY*2.0f)*0.5f;
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(cp.x - padX, chipY),
                    ImVec2(cp.x + tw + padX, chipY + ImGui::GetTextLineHeight() + padY*2.0f),
                    IM_COL32(35, 55, 88, 130), 4.0f);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.91f, 1.00f, 1.00f));
            ImGui::TextUnformatted(m_worldName.c_str());
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("World: %s", m_worldName.c_str());
        }

        inlineSep();

        // ── Player position ───────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.38f, 0.50f, 0.65f, 1.00f));
        ImGui::TextUnformatted("XYZ");
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.84f, 0.94f, 1.00f));
        ImGui::TextUnformatted(posBuf);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Player position  X / Y / Z");

        // ── Biome badge ───────────────────────────────────────────────────────
        if (showBiome) {
            inlineSep();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.78f, 0.52f, 1.00f));
            ImGui::TextUnformatted(m_biomeName.c_str());
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Current biome");
        }

        // ── Chunk count ──────────────────────────────────────────────────────
        if (showChunks) {
            inlineSep();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.56f, 0.56f, 0.70f, 1.00f));
            ImGui::TextUnformatted(chunkBuf);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Loaded chunks: %d", m_loadedChunks);
        }

        // ── Open panel count badge ────────────────────────────────────────────
        if (panelBuf[0]) {
            inlineSep();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.72f, 0.88f, 1.00f));
            ImGui::TextUnformatted(panelBuf);
            ImGui::PopStyleColor();
        }

        inlineSep();

        // ── FPS (colour-coded chip) ───────────────────────────────────────────
        {
            ImVec2 cp   = ImGui::GetCursorScreenPos();
            float  tw   = ImGui::CalcTextSize(fpsBuf).x;
            const float chPadX = 5.0f, chPadY = 2.0f;
            float  barH = ImGui::GetFrameHeight();
            float  chipY = cp.y + (barH - ImGui::GetTextLineHeight() - chPadY*2.0f)*0.5f;
            ImU32  chipBg = (fps >= 60.f) ? IM_COL32(15,80,30,120)
                          : (fps >= 45.f) ? IM_COL32(50,80,15,120)
                          : (fps >= 30.f) ? IM_COL32(90,70,10,130)
                                          : IM_COL32(100,15,15,140);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(cp.x - chPadX, chipY),
                ImVec2(cp.x + tw + chPadX, chipY + ImGui::GetTextLineHeight() + chPadY*2.0f),
                chipBg, 3.5f);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, fpsCol);
        ImGui::TextUnformatted(fpsBuf);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 4.0f);

        // ── ms (muted, same line) ────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.38f, 0.44f, 0.55f, 1.00f));
        ImGui::TextUnformatted(msBuf);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("%.3f ms per frame", frameMs);

        inlineSep();

        // ── Version badge ────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.36f, 0.42f, 0.52f, 1.00f));
        ImGui::TextUnformatted(kVersion);
        ImGui::PopStyleColor();
    }

    ImGui::EndMainMenuBar();
}

void GUIManager::showInteractionEditor(bool* open) {
    if (!*open) return;
    ImVec2 mvCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(mvCenter, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
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
    // Center on first appearance only — do NOT call SetNextWindowFocus() every
    // frame or it will steal keyboard/mouse focus from every other panel.
    ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // Bring to front once on open; after that the user can freely reorder.
    static bool s_justOpened = false;
    if (!s_justOpened) {
        ImGui::SetNextWindowFocus();
        s_justOpened = true;
    }
    // Reset trigger for next open.
    static bool s_wasOpen = false;
    if (!*open) { s_wasOpen = false; s_justOpened = false; }
    else if (!s_wasOpen) { s_wasOpen = true; s_justOpened = false; }

    const ImGuiWindowFlags prefFlags = ImGuiWindowFlags_NoDocking
                                     | ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("Preferences", open, prefFlags)) { ImGui::End(); return; }

    if (ImGui::BeginTabBar("##PrefTabs")) {

        // ── Display ───────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("Display")) {
            ImGui::Spacing();
            ImGui::SeparatorText("Window");
            ImGui::Spacing();

            if (ImGui::Checkbox("Fullscreen", &fullscreen))
                renderer.setFullscreen(fullscreen);
            ImGui::SameLine(0.0f, 16.0f);
            if (ImGui::Checkbox("VSync", &vsync))
                renderer.setVSync(vsync);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Locks framerate to monitor refresh rate.\nDisable for uncapped FPS.");

            ImGui::Spacing();
            ImGui::SeparatorText("Rendering");
            ImGui::Spacing();

            if (ImGui::Checkbox("Wireframe Mode", &wireframe))
                renderer.setWireframe(wireframe);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Render only block edges — useful for debugging geometry.");

            if (ImGui::Checkbox("Backface Culling", &backfaceCulling))
                renderer.setBackfaceCulling(backfaceCulling);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Skip rendering faces that face away from the camera.\nImproves performance, disable if you see holes.");

            ImGui::Spacing();
            ImGui::SeparatorText("Info");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.38f, 0.44f, 0.55f, 1.00f));
            ImGui::TextUnformatted("Build: " __DATE__ "  " __TIME__);
            ImGui::TextUnformatted("Version: V0.6 Beta");
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }

        // ── Graphics ──────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("Graphics")) {
            ImGui::Spacing();
            ImGui::SeparatorText("Performance");
            ImGui::Spacing();

            static int renderDist = 8;
            ImGui::PushItemWidth(220.0f);
            ImGui::SliderInt("Render Distance", &renderDist, 2, 24);
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Chunks loaded around the player.\nLower values improve performance.");

            static float fov = 75.0f;
            ImGui::PushItemWidth(220.0f);
            ImGui::SliderFloat("Field of View", &fov, 50.0f, 110.0f, "%.0f deg");
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::SeparatorText("Quality");
            ImGui::Spacing();

            static bool ambientOcclusion = true;
            static bool faceShading      = true;
            static bool fogEnabled       = true;
            ImGui::Checkbox("Ambient Occlusion", &ambientOcclusion);
            ImGui::SameLine(200.0f);
            ImGui::Checkbox("Face Shading", &faceShading);
            ImGui::Checkbox("Distance Fog", &fogEnabled);

            ImGui::Spacing();
            ImGui::TextDisabled("Note: Some settings take effect on next world load.");
            ImGui::EndTabItem();
        }

        // ── Audio ─────────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("Audio")) {
            ImGui::Spacing();
            ImGui::SeparatorText("Volume");
            ImGui::Spacing();

            ImGui::PushItemWidth(260.0f);
            float master = AudioManager::getInstance().getMasterVolume();
            if (ImGui::SliderFloat("Master Volume", &master, 0.0f, 1.0f, "%.2f"))
                AudioManager::getInstance().setMasterVolume(master);

            float music = AudioManager::getInstance().getMusicVolume();
            if (ImGui::SliderFloat("Music Volume", &music, 0.0f, 1.0f, "%.2f"))
                AudioManager::getInstance().setMusicVolume(music);
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::SeparatorText("Toggles");
            ImGui::Spacing();

            bool mobs = AudioManager::getInstance().isMobSoundsEnabled();
            if (ImGui::Checkbox("Mob Sounds", &mobs))
                AudioManager::getInstance().setMobSoundsEnabled(mobs);
            ImGui::SameLine(200.0f);
            bool blks = AudioManager::getInstance().isBlockSoundsEnabled();
            if (ImGui::Checkbox("Block Sounds", &blks))
                AudioManager::getInstance().setBlockSoundsEnabled(blks);

            bool musicOn = AudioManager::getInstance().isMusicEnabled();
            if (ImGui::Checkbox("Background Music", &musicOn))
                AudioManager::getInstance().setMusicEnabled(musicOn);

            ImGui::Spacing();
            ImGui::TextDisabled("Tip: Press ESC in-game to quickly access the Game Menu.");
            ImGui::EndTabItem();
        }

        // ── Controls ──────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("Controls")) {
            ImGui::Spacing();
            ImGui::SeparatorText("Mouse");
            ImGui::Spacing();

            static float mouseSens = 0.08f;
            ImGui::PushItemWidth(220.0f);
            ImGui::SliderFloat("Sensitivity", &mouseSens, 0.01f, 0.50f, "%.2f");
            ImGui::PopItemWidth();

            static bool invertY = false;
            ImGui::Checkbox("Invert Y Axis", &invertY);

            ImGui::Spacing();
            ImGui::SeparatorText("Key Reference");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.70f, 1.00f));
            auto row = [](const char* key, const char* action) {
                ImGui::TextUnformatted(key);
                ImGui::SameLine(150.0f);
                ImGui::TextUnformatted(action);
            };
            row("W A S D",      "Move");
            row("Space",        "Jump / Fly Up");
            row("Shift",        "Sprint / Fly Down");
            row("Space x2",     "Toggle Fly Mode");
            row("LMB",          "Break Block");
            row("RMB",          "Place Block");
            row("1 – 9",        "Hotbar Slot");
            row("Tab",          "Inventory");
            row("E",            "Interact / Pick Block");
            row("F2",           "Profiler Toggle");
            row("Ctrl+W",       "World Editor");
            row("Ctrl+P",       "Preferences");
            row("ESC",          "Game Menu / Audio");
            row("F11",          "Toggle Fullscreen");
            row("F5",           "Regenerate World");
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
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

// ---------------------------------------------------------------------------
// showWindowTabBar — redesigned secondary row pinned below the main menu bar.
//
// Features:
//   • 50 px height for better click targets and visual weight
//   • Background unified with the menu bar for seamless chrome
//   • Left-side coloured accent bar on each chip for category identity
//   • Group labels: WORLD / DESIGN / ENGINE / VIEW
//   • Hover tooltip shows the full panel title
//   • Animated "no panels" hint when all closed
// ---------------------------------------------------------------------------
void GUIManager::showWindowTabBar(
    bool& showProfiler, bool& showMemory, bool& showECS,
    bool& showWorldEditor, bool& showSettings,
    bool& showSoundEditor, bool& showBlockDesigner,
    bool& showMobDesigner, bool& showInteractionEditor,
    bool& showToolDesigner, bool& showWeatherDesigner,
    bool& showSoundDesigner, bool& showAdvWorldEditor,
    bool& showTextureDesigner)
{
    // Build list of ALL panels — only render chips for open ones.
    PanelDesc panels[] = {
        { "World Editor",     "World Editor",          &showWorldEditor,       0.36f, 0.61f, 0.84f },
        { "Adv. World",       "Advanced World Editor", &showAdvWorldEditor,    0.36f, 0.61f, 0.84f },
        { "Blocks",           "Block Designer",        &showBlockDesigner,     0.62f, 0.40f, 0.88f },
        { "Mobs",             "Mob Designer",          &showMobDesigner,       0.62f, 0.40f, 0.88f },
        { "Textures",         "Texture Designer",      &showTextureDesigner,   0.62f, 0.40f, 0.88f },
        { "Tools",            "Tool Designer",         &showToolDesigner,      0.62f, 0.40f, 0.88f },
        { "Weather",          "Weather Designer",      &showWeatherDesigner,   0.62f, 0.40f, 0.88f },
        { "Sound Designer",   "Sound Designer",        &showSoundDesigner,     0.62f, 0.40f, 0.88f },
        { "Engine",           "Game Engine Editor",    &showInteractionEditor, 0.90f, 0.62f, 0.20f },
        { "Sound Editor",     "Sound Editor",          &showSoundEditor,       0.90f, 0.62f, 0.20f },
        { "Profiler",         "Engine Profiler",       &showProfiler,          0.28f, 0.82f, 0.50f },
        { "Memory",           "Memory Inspector",      &showMemory,            0.28f, 0.82f, 0.50f },
        { "ECS",              "ECS Editor",            &showECS,               0.28f, 0.82f, 0.50f },
        { "Preferences",      "Preferences",           &showSettings,          0.28f, 0.82f, 0.50f },
    };
    constexpr int kCount = (int)(sizeof(panels) / sizeof(panels[0]));

    int openCount = 0;
    for (int i = 0; i < kCount; ++i)
        if (*panels[i].flag) ++openCount;

    const float menuBarH = ImGui::GetFrameHeightWithSpacing();
    ImGuiViewport* vp = ImGui::GetMainViewport();
    constexpr float kBarH = 38.0f;  // Taller toolbar

    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + menuBarH));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, kBarH));
    ImGui::SetNextWindowViewport(vp->ID);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    // Background unified with the menu bar for seamless top chrome.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(12.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(5.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,  ImVec4(0.042f, 0.044f, 0.060f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Border,    ImVec4(0.10f,  0.13f,  0.20f,  0.80f));

    ImGui::Begin("##PanelTabBar", nullptr, flags);

    // Top border: very thin line under menu bar
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();
        float  w = ImGui::GetWindowWidth();
        float  h = ImGui::GetWindowHeight();
        dl->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y),
                    IM_COL32(50, 70, 110, 160), 1.0f);
        // Bottom gradient line: accent colour fade — marks the edge of the toolbar zone
        dl->AddRectFilledMultiColor(
            ImVec2(p.x, p.y + h - 2.0f), ImVec2(p.x + w, p.y + h),
            IM_COL32(30, 50, 90, 90), IM_COL32(58, 106, 158, 120),
            IM_COL32(58, 106, 158, 120), IM_COL32(30, 50, 90, 90));
    }

    if (openCount == 0) {
        // Empty-state hint
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.25f, 0.28f, 0.36f, 1.00f));
        ImGui::SetCursorPosY((kBarH - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::TextUnformatted("  No panels open  —  use the menus or toolbar shortcuts to open one");
        ImGui::PopStyleColor();
    }

    // Helper: draw a small coloured category label
    auto drawGroupLabel = [&](const char* lbl, float r, float g, float b) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(r * 0.7f, g * 0.7f, b * 0.7f, 0.80f));
        ImGui::SetCursorPosY((kBarH - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::TextUnformatted(lbl);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 4.0f);
        // tiny vertical separator
        float cy = ImGui::GetCursorScreenPos().y + (kBarH - ImGui::GetTextLineHeight()) * 0.5f - 2.0f;
        float cx = ImGui::GetCursorScreenPos().x;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(cx, cy), ImVec2(cx + 1.0f, cy + ImGui::GetTextLineHeight() + 4.0f),
            IM_COL32(r * 200, g * 200, b * 200, 80));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
    };

    // Group tracking for labels
    struct GroupInfo { float r, g, b; const char* label; };
    const GroupInfo kGroups[] = {
        { 0.36f, 0.61f, 0.84f, "WORLD" },
        { 0.62f, 0.40f, 0.88f, "DESIGN" },
        { 0.90f, 0.62f, 0.20f, "ENGINE" },
        { 0.28f, 0.82f, 0.50f, "VIEW" },
    };
    auto getGroupIdx = [&](int panelIdx) -> int {
        if (panelIdx <= 1) return 0;
        if (panelIdx <= 7) return 1;
        if (panelIdx <= 9) return 2;
        return 3;
    };

    int lastGroup = -1;
    bool firstChip = true;

    for (int i = 0; i < kCount; ++i) {
        if (!(*panels[i].flag)) continue;

        const float r = panels[i].r, g = panels[i].g, b = panels[i].b;
        const float chipH = kBarH - 10.0f;  // vertical padding = 5px each side

        // Group label on first chip of a new group
        int grp = getGroupIdx(i);
        if (grp != lastGroup) {
            if (!firstChip) {
                // Group separator
                ImGui::SameLine(0.0f, 14.0f);
                float cy = ImGui::GetCursorScreenPos().y + (kBarH - chipH) * 0.5f;
                float cx = ImGui::GetCursorScreenPos().x;
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(cx, cy), ImVec2(cx + 1.0f, cy + chipH),
                    IM_COL32(60, 70, 95, 140));
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
            }
            drawGroupLabel(kGroups[grp].label, kGroups[grp].r, kGroups[grp].g, kGroups[grp].b);
            lastGroup = grp;
        } else if (!firstChip) {
            ImGui::SameLine(0.0f, 3.0f);
        }

        firstChip = false;

        // ── Chip: background + coloured bottom accent line ──────────────────
        ImVec2 chipPos = ImGui::GetCursorScreenPos();
        chipPos.y += (kBarH - chipH) * 0.5f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(12.0f, 4.0f));
        // Brighter chip so it reads clearly against the unified menu bar background.
        ImGui::PushStyleColor(ImGuiCol_Button,
            ImVec4(r*0.22f, g*0.22f, b*0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImVec4(r*0.38f, g*0.38f, b*0.38f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            ImVec4(r*0.55f, g*0.55f, b*0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,
            ImVec4(std::min(r*1.7f, 1.0f), std::min(g*1.7f, 1.0f), std::min(b*1.7f, 1.0f), 1.0f));

        ImGui::SetCursorPosY((kBarH - chipH) * 0.5f);
        char chipID[64];
        snprintf(chipID, sizeof(chipID), "%s##chip%d", panels[i].label, i);
        if (ImGui::Button(chipID, ImVec2(0.0f, chipH))) {
            *panels[i].flag = true;
            ImGui::SetWindowFocus(panels[i].winName);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Focus  %s", panels[i].winName);

        // Draw 2 px coloured BOTTOM accent line under the chip (modern IDE-style tab)
        {
            ImVec2 rMin = ImGui::GetItemRectMin();
            ImVec2 rMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(rMin.x + 2.0f, rMax.y - 2.0f),
                ImVec2(rMax.x - 2.0f, rMax.y),
                IM_COL32((uint8_t)(r * 220), (uint8_t)(g * 220), (uint8_t)(b * 220), 200),
                1.0f);
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(3);

        // ── Close × ─────────────────────────────────────────────────────────
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(3.0f, 3.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.15f, 0.15f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.65f, 0.08f, 0.08f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.40f, 0.44f, 0.52f, 1.00f));

        ImGui::SetCursorPosY((kBarH - chipH) * 0.5f);
        char closeID[64];
        snprintf(closeID, sizeof(closeID), "x##close%d", i);
        if (ImGui::Button(closeID, ImVec2(20.0f, chipH)))
            *panels[i].flag = false;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Close  %s", panels[i].label);

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(3);
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
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

    ImGui::SeparatorText("Custom Allocators");

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
    ImGui::SeparatorText("Entities");
    ImGui::Text("Entities: %d", 1);

    if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float pos[3] = {0.0f, 0.0f, 0.0f};
        ImGui::InputFloat3("Position", pos);
    }

    ImGui::End();
}
