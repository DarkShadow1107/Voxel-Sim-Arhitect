#pragma once

#include <array>
#include <string>

struct GLFWwindow;

class EngineProfiler {
public:
    static constexpr int kHistorySize = 360;
    static constexpr int kMaxCores = 64;

    void show(float frameTime, bool& vsync, GLFWwindow* window);

private:
    // Frame time ring buffer
    std::array<float, kHistorySize> m_frameMsHistory{};
    std::array<float, kHistorySize> m_fpsHistory{};
    int m_head = 0;
    int m_count = 0;

    // CPU
    unsigned long long m_lastCPUTime = 0;
    unsigned long long m_lastProcTime = 0;
    float m_cpuPercent = 0.0f;
    int m_coreCount = 0;
    std::array<float, kMaxCores> m_perCoreFreq{};
    float m_cpuFreqAvg = 0.0f;
    float m_cpuTempC = -1.0f;

    // GPU (NVIDIA via dynamic NVML)
    float m_gpuPercent = 0.0f;
    float m_gpuTempC = -1.0f;
    float m_gpuClockMHz = 0.0f;
    float m_gpuMemClockMHz = 0.0f;
    bool m_nvmlLoaded = false;
    bool m_nvmlInitAttempted = false;
    void* m_nvmlLib = nullptr;
    void* m_nvmlDevice = nullptr;

    // History arrays
    std::array<float, kHistorySize> m_cpuHistory{};
    std::array<float, kHistorySize> m_gpuHistory{};
    std::array<float, kHistorySize> m_cpuFreqHistory{};
    std::array<float, kHistorySize> m_gpuFreqHistory{};
    std::array<float, kHistorySize> m_cpuTempHistory{};
    std::array<float, kHistorySize> m_gpuTempHistory{};

    // Per-core frequency history for individual core graphs
    std::array<std::array<float, kHistorySize>, kMaxCores> m_perCoreFreqHistory{};

    // FPS stats
    float m_fps1Low = 0.0f;
    float m_fpsAvg = 0.0f;
    float m_fpsHigh = 0.0f;

    // Per-metric refresh intervals (user-configurable)
    float m_fpsInterval = 0.5f;
    float m_usageInterval = 1.0f;
    float m_freqInterval = 1.0f;
    float m_tempInterval = 2.0f;

    // Timers
    float m_fpsTimer = 0.0f;
    float m_usageTimer = 0.0f;
    float m_freqTimer = 0.0f;
    float m_tempTimer = 0.0f;

    // Cached display values
    float m_dFps1Low = 0.0f, m_dFpsAvg = 0.0f, m_dFpsHigh = 0.0f;
    float m_dCpuPct = 0.0f, m_dGpuPct = 0.0f;
    float m_dCpuFreq = 0.0f, m_dGpuClock = 0.0f;
    std::array<float, kMaxCores> m_dPerCoreFreq{};
    float m_dCpuTemp = -1.0f, m_dGpuTemp = -1.0f;

    // NVML helpers
    void tryLoadNVML();
    void queryNVML();

    // PDH for real-time CPU frequency (includes turbo boost)
    void* m_pdhQuery = nullptr;
    std::array<void*, kMaxCores> m_pdhCoreCounters{};
    bool m_pdhInitialized = false;
    float m_cpuMaxMhz = 0.0f;
    void* m_pdhThermalCounter = nullptr;
    void initPDH();

    // CPU real-time frequency via PDH % Processor Performance
    void queryCpuFrequencies();
};
