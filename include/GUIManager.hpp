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
    void beginFrame();
    void endFrame();
    void shutdown();

    // Production-grade UI
    void applyTheme();
    void showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS, bool& showWorldEditor, bool& showSettings, bool& showSoundEditor);
    void showSettings(bool* open, bool& vsync, bool& wireframe, bool& fullscreen, bool& backfaceCulling, class Renderer& renderer);
    void showSoundEditor(bool* open);

    // GUI Panels
    void showProfiler(float frameTime);
    void showMemoryInspector(size_t arenaOffset, size_t arenaSize, size_t poolUsed, size_t poolTotal);
    void showECSEditor();

private:
    static constexpr int kProfilerHistorySize = 240;

    GLFWwindow* m_window = nullptr;
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
