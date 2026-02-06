#include "GUIManager.hpp"
#include "Renderer.hpp"
#include "AudioManager.hpp"
#include "Registry.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <thread>
#include <map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <commdlg.h>
#endif

namespace {
    std::string openFileDialog() {
#ifdef _WIN32
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = 260;
        ofn.lpstrFilter = "Audio Files (*.wav;*.mp3)\0*.wav;*.mp3\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            return std::string(szFile);
        }
#endif
        return "";
    }
}

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    applyTheme();

    // Platform/renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "ImGui_ImplGlfw_InitForOpenGL failed" << std::endl;
        return false;
    }

    // This is a safe default for GL 3.0+ contexts. If your GPU is older, you can lower it.
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "ImGui_ImplOpenGL3_Init failed" << std::endl;
        return false;
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

    // When using multi-viewport, match ImGui defaults to avoid rounding artifacts
    // between the main window and platform windows.
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

void GUIManager::showBlockDesigner(bool* open) {
    if (!*open) return;
    if (!ImGui::Begin("Block Designer", open)) {
        ImGui::End();
        return;
    }

    auto& blocks = GameRegistry::getInstance().getAllBlocks();
    static uint8_t selectedId = 1;

    ImGui::BeginChild("BlockList", ImVec2(200, 0), true);
    for (auto& [id, def] : blocks) {
        if (id == 0) continue;
        if (ImGui::Selectable(def.name.c_str(), selectedId == id)) {
            selectedId = id;
        }
    }
    if (ImGui::Button("Add New Block")) {
        uint8_t nextId = 1;
        while (blocks.count(nextId)) nextId++;
        GameRegistry::getInstance().registerBlock({nextId, "New Block", {1,1,1}, 0, 0});
        selectedId = nextId;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginGroup();
    auto& def = blocks[selectedId];
    char nameBuf[64];
    strncpy(nameBuf, def.name.c_str(), 63);
    nameBuf[63] = '\0';
    if (ImGui::InputText("Name", nameBuf, 64)) def.name = nameBuf;
    
    ImGui::ColorEdit3("Color Tint", &def.color.x);
    ImGui::InputInt("Texture X", &def.texX);
    ImGui::InputInt("Texture Y", &def.texY);

    ImGui::Text("Preview:");
    if (m_atlasID) {
        float size = 64.0f;
        float uv_step = 1.0f / 16.0f;
        ImVec2 uv0 = ImVec2(def.texX * uv_step, def.texY * uv_step);
        ImVec2 uv1 = ImVec2((def.texX + 1) * uv_step, (def.texY + 1) * uv_step);
        // Using ImageWithBg for the newer ImGui version (1.91.9+) which moved tint_col there
        ImGui::ImageWithBg((ImTextureID)(uintptr_t)m_atlasID, ImVec2(size, size), uv0, uv1, ImVec4(0,0,0,0), ImVec4(def.color.x, def.color.y, def.color.z, 1.0f));
    }

    ImGui::Checkbox("Transparent", &def.isTransparent);
    ImGui::Checkbox("Liquid (Water-like)", &def.isLiquid);
    
    ImGui::Separator();
    ImGui::Text("Sounds");
    ImGui::Text("Break: %s", def.breakSound.empty() ? "None" : def.breakSound.c_str());
    ImGui::Text("Step: %s", def.stepSound.empty() ? "None" : def.stepSound.c_str());

    ImGui::EndGroup();

    ImGui::End();
}

void GUIManager::showMobDesigner(bool* open) {
    if (!*open) return;
    if (!ImGui::Begin("Mob Designer", open)) {
        ImGui::End();
        return;
    }

    auto& mobs = GameRegistry::getInstance().getAllMobs();
    static MobType selectedType = MOB_COW;

    ImGui::BeginChild("MobList", ImVec2(200, 0), true);
    for (auto& [type, def] : mobs) {
        if (ImGui::Selectable(def.name.c_str(), selectedType == type)) {
            selectedType = type;
        }
    }
    if (ImGui::Button("Add New Mob")) {
        int nextId = (int)MOB_COUNT;
        while (mobs.count((MobType)nextId)) nextId++;
        GameRegistry::getInstance().registerMob({(MobType)nextId, "New Mob", 10.0f, 2.0f});
        selectedType = (MobType)nextId;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginGroup();
    auto& def = mobs[selectedType];
    char nameBuf[64];
    strncpy(nameBuf, def.name.c_str(), 63);
    nameBuf[63] = '\0';
    if (ImGui::InputText("Name", nameBuf, 64)) def.name = nameBuf;
    
    ImGui::SliderFloat("Health", &def.maxHp, 1.0f, 100.0f);
    ImGui::SliderFloat("Speed", &def.speed, 0.5f, 10.0f);
    ImGui::Checkbox("Aquatic", &def.isAquatic);
    ImGui::Checkbox("Hostile", &def.isHostile);
    
    ImGui::Separator();
    ImGui::Text("Model Blueprint Preview:");
    ImGui::BeginChild("MobPreview", ImVec2(0, 100), true);
    ImGui::Text("Scale: [1.0, 1.0, 1.0]");
    ImGui::Text("Parts: Body, Head, 4x Legs");
    if (def.isAquatic) ImGui::Text("Material: Aquatic/Submerged");
    if (def.isHostile) ImGui::Text("Aura: Hostile/Red");
    ImGui::EndChild();
    
    ImGui::Separator();
    ImGui::Text("Model: Standard Voxel");

    ImGui::EndGroup();

    ImGui::End();
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


void GUIManager::showSoundEditor(bool* open) {
    if (!*open) return;
    
    auto& am = AudioManager::getInstance();

    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Sound Editor", open)) {
        ImGui::Text("Volume Mixers");
        ImGui::Separator();

        float master = am.getMasterVolume();
        if (ImGui::SliderFloat("Master", &master, 0.0f, 1.0f)) am.setMasterVolume(master);

        float music = am.getMusicVolume();
        if (ImGui::SliderFloat("Music", &music, 0.0f, 1.0f)) am.setMusicVolume(music);

        float mobs = am.getMobVolume();
        if (ImGui::SliderFloat("Mobs", &mobs, 0.0f, 1.0f)) am.setMobVolume(mobs);

        float blocks = am.getBlockVolume();
        if (ImGui::SliderFloat("Blocks", &blocks, 0.0f, 1.0f)) am.setBlockVolume(blocks);

        ImGui::Spacing();
        ImGui::Text("Environmental Sounds");
        ImGui::Separator();

        bool musicOn = am.isMusicEnabled();
        if (ImGui::Checkbox("Background Music", &musicOn)) am.setMusicEnabled(musicOn);

        if (ImGui::Button("Next Track")) {
            // Need to expose startNextMusic or just stop current
            am.setMusicEnabled(false);
            am.setMusicEnabled(true);
        }

        bool mobsOn = am.isMobSoundsEnabled();
        if (ImGui::Checkbox("Mob Sounds", &mobsOn)) am.setMobSoundsEnabled(mobsOn);

        bool blocksOn = am.isBlockSoundsEnabled();
        if (ImGui::Checkbox("Block Breaking", &blocksOn)) am.setBlockSoundsEnabled(blocksOn);

        ImGui::Separator();
        ImGui::Text("Sound Studio");
        static char soundPath[256] = "";
        static float pitch = 1.0f;
        static float vol = 1.0f;
        ImGui::InputText("File Path", soundPath, 256);
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            std::string path = openFileDialog();
            if (!path.empty()) {
                // Try to make it relative to project root if possible
                strncpy(soundPath, path.c_str(), 255);
            }
        }
        ImGui::SliderFloat("Pitch Alteration", &pitch, 0.5f, 2.0f);
        ImGui::SliderFloat("Volume Alteration", &vol, 0.0f, 1.0f);
        
        if (ImGui::Button("Preview Adjusted sound")) {
            if (strlen(soundPath) > 0) {
                am.playSoundWithPitch(soundPath, vol, pitch);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Set as Block Break")) {
            // Mapping logic
        }

        ImGui::Separator();
        if (ImGui::Button("Close")) *open = false;
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

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void GUIManager::shutdown() {
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_window = nullptr;
    m_initialized = false;
}

void GUIManager::showProfiler(float frameTime) {
    if (!m_initialized) {
        return;
    }

    // Store history as a ring buffer (no per-frame allocations).
    m_frameMsHistory[(size_t)m_frameMsHead] = frameTime;
    m_frameMsHead = (m_frameMsHead + 1) % kProfilerHistorySize;
    m_frameMsCount = std::min(m_frameMsCount + 1, kProfilerHistorySize);

    // CPU Usage Calculation (Windows)
#ifdef _WIN32
    FILETIME ftSysIdle, ftSysKernel, ftSysUser;
    FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
    if (GetSystemTimeAsFileTime(&ftSysIdle), GetProcessTimes(GetCurrentProcess(), &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser)) {
        ULARGE_INTEGER now, proc;
        GetSystemTimeAsFileTime(&ftSysIdle); // Reuse ftSysIdle for current time
        now.LowPart = ftSysIdle.dwLowDateTime;
        now.HighPart = ftSysIdle.dwHighDateTime;
        
        proc.LowPart = ftProcKernel.dwLowDateTime + ftProcUser.dwLowDateTime;
        proc.HighPart = ftProcKernel.dwHighDateTime + ftProcUser.dwHighDateTime;

        if (m_lastCPUUsageTime > 0) {
            unsigned long long diffTime = now.QuadPart - m_lastCPUUsageTime;
            unsigned long long diffProc = proc.QuadPart - m_lastProcessTime;
            if (diffTime > 0) {
                m_cpuUsagePercent = (float)((double)diffProc / (double)diffTime) * 100.0f / (float)std::thread::hardware_concurrency();
            }
        }
        m_lastCPUUsageTime = now.QuadPart;
        m_lastProcessTime = proc.QuadPart;
    }
#endif

    ImGui::Begin("Engine Profiler");

    const float safeMs = (frameTime > 0.0001f) ? frameTime : 0.0001f;
    ImGui::Text("Frame: %.3f ms (%.1f FPS)", frameTime, 1000.0f / safeMs);
    ImGui::Text("CPU Usage: %.1f%% (%u Cores)", m_cpuUsagePercent, std::thread::hardware_concurrency());
    
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    ImGui::Text("GPU: %s", renderer ? renderer : "Unknown");
    ImGui::Text("Vendor: %s", vendor ? vendor : "Unknown");

    bool vsync = m_vsync;
    if (ImGui::Checkbox("VSync", &vsync)) {
        m_vsync = vsync;
        if (m_window) {
            glfwSwapInterval(m_vsync ? 1 : 0);
        }
    }

    const auto getter = [](void* data, int idx) -> float {
        GUIManager* self = static_cast<GUIManager*>(data);
        if (self->m_frameMsCount <= 0) {
            return 0.0f;
        }

        // Oldest sample first.
        const int start = (self->m_frameMsHead - self->m_frameMsCount + kProfilerHistorySize) % kProfilerHistorySize;
        const int i = (start + idx) % kProfilerHistorySize;
        return self->m_frameMsHistory[(size_t)i];
    };

    ImGui::PlotLines(
        "Frame time (ms)",
        getter,
        this,
        m_frameMsCount,
        0,
        nullptr,
        0.0f,
        40.0f,
        ImVec2(0.0f, 80.0f));

    ImGui::End();
}

void GUIManager::showMemoryInspector(size_t arenaOffset, size_t arenaSize, size_t poolUsed, size_t poolTotal) {
    if (!m_initialized) {
        return;
    }

    ImGui::Begin("Memory Inspector");

    // RAM Usage (Windows)
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        float usedMB = (float)pmc.PrivateUsage / (1024.0f * 1024.0f);
        ImGui::Text("Process RAM: %.1f MB", usedMB);
    }
#endif

    // VRAM Usage (NVIDIA)
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
    if (!m_initialized) {
        return;
    }

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
