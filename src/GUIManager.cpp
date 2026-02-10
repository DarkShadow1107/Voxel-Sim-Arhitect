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
    float lighting = 0.40 + 0.60 * ndotl;
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

    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;

    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void GUIManager::showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS, bool& showWorldEditor, bool& showSettings, bool& showSoundEditor, bool& showBlockDesigner, bool& showMobDesigner, bool& showInteractionEditor) {
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
            ImGui::MenuItem("Sound Manager", nullptr, &showSoundEditor);
            ImGui::MenuItem("Interactions", nullptr, &showInteractionEditor);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Profiler", nullptr, &showProfiler);
            ImGui::MenuItem("Memory Inspector", nullptr, &showMemory);
            ImGui::MenuItem("ECS Editor", nullptr, &showECS);
            ImGui::MenuItem("World Editor", nullptr, &showWorldEditor);
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
    if (!ImGui::Begin("Interaction Editor", open)) {
        ImGui::End();
        return;
    }
    ImGui::Text("Interaction Types:");
    ImGui::BulletText("Left Click: Break Block");
    ImGui::BulletText("Right Click: Place Block");
    ImGui::BulletText("E: Interact with Mob");

    static int interactionRange = 5;
    ImGui::SliderInt("Interaction Range", &interactionRange, 1, 10);

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

void GUIManager::shutdown() {
    if (!m_initialized) return;

    if (m_previewShader) { delete m_previewShader; m_previewShader = nullptr; }
    if (m_blockPreviewBuf) { delete m_blockPreviewBuf; m_blockPreviewBuf = nullptr; }
    if (m_mobPreviewBuf) { delete m_mobPreviewBuf; m_mobPreviewBuf = nullptr; }

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
