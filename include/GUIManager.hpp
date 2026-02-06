#pragma once

#include <array>
#include <cstddef>
#include <string>

struct GLFWwindow;

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    bool init(GLFWwindow* window);
    void setAtlasTextureID(uint32_t id) { m_atlasID = id; }
    void beginFrame();
    void endFrame();
    void shutdown();

    // Production-grade UI
    void applyTheme();
    void showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS, bool& showWorldEditor, bool& showSettings, bool& showSoundEditor, bool& showBlockDesigner, bool& showMobDesigner, bool& showInteractionEditor);
    void showSettings(bool* open, bool& vsync, bool& wireframe, bool& fullscreen, bool& backfaceCulling, class Renderer& renderer);
    void showSoundEditor(bool* open);
    void showBlockDesigner(bool* open);
    void showMobDesigner(bool* open);
    void showInteractionEditor(bool* open);

    // GUI Panels
    void showProfiler(float frameTime);
    void showMemoryInspector(size_t arenaOffset, size_t arenaSize, size_t poolUsed, size_t poolTotal);
    void showECSEditor();

private:
    static constexpr int kProfilerHistorySize = 240;

    GLFWwindow* m_window = nullptr;
    uint32_t m_atlasID = 0;
    bool m_initialized = false;
    bool m_vsync = true;

    std::array<float, kProfilerHistorySize> m_frameMsHistory{};
    int m_frameMsHead = 0;
    int m_frameMsCount = 0;

    // CPU Profiling
    unsigned long long m_lastCPUUsageTime = 0;
    unsigned long long m_lastProcessTime = 0;
    float m_cpuUsagePercent = 0.0f;
};
