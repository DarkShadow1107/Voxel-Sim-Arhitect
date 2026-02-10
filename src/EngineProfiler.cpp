#include "EngineProfiler.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include <algorithm>
#include <vector>
#include <thread>
#include <cstring>

// NVIDIA VRAM extension
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX  0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <powerbase.h>   // CallNtPowerInformation
#include <pdh.h>         // Performance Data Helper for real-time CPU frequency
typedef LONG NTSTATUS;   // Avoid pulling in ntstatus.h (causes hundreds of macro redefinitions)

// NVML types for dynamic loading
typedef enum {
    NVML_SUCCESS = 0
} nvmlReturn_t;

typedef enum {
    NVML_TEMPERATURE_GPU = 0
} nvmlTemperatureSensors_t;

typedef enum {
    NVML_CLOCK_GRAPHICS = 0,
    NVML_CLOCK_MEM = 2
} nvmlClockType_t;

typedef void* nvmlDevice_t;

typedef struct {
    unsigned int gpu;
    unsigned int memory;
} nvmlUtilization_t;

// Function pointer types
typedef nvmlReturn_t (*pfn_nvmlInit)(void);
typedef nvmlReturn_t (*pfn_nvmlShutdown)(void);
typedef nvmlReturn_t (*pfn_nvmlDeviceGetHandleByIndex)(unsigned int, nvmlDevice_t*);
typedef nvmlReturn_t (*pfn_nvmlDeviceGetUtilizationRates)(nvmlDevice_t, nvmlUtilization_t*);
typedef nvmlReturn_t (*pfn_nvmlDeviceGetTemperature)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int*);
typedef nvmlReturn_t (*pfn_nvmlDeviceGetClockInfo)(nvmlDevice_t, nvmlClockType_t, unsigned int*);

// Global NVML function pointers
static pfn_nvmlInit                       s_nvmlInit = nullptr;
static pfn_nvmlShutdown                   s_nvmlShutdown = nullptr;
static pfn_nvmlDeviceGetHandleByIndex     s_nvmlDeviceGetHandleByIndex = nullptr;
static pfn_nvmlDeviceGetUtilizationRates  s_nvmlDeviceGetUtilizationRates = nullptr;
static pfn_nvmlDeviceGetTemperature       s_nvmlDeviceGetTemperature = nullptr;
static pfn_nvmlDeviceGetClockInfo         s_nvmlDeviceGetClockInfo = nullptr;

// PROCESSOR_POWER_INFORMATION for CallNtPowerInformation
typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

#endif // _WIN32

void EngineProfiler::tryLoadNVML() {
#ifdef _WIN32
    if (m_nvmlInitAttempted) return;
    m_nvmlInitAttempted = true;

    // NVML ships with NVIDIA drivers — try system path first, then Program Files
    HMODULE hMod = LoadLibraryA("nvml.dll");
    if (!hMod) {
        // Try the standard NVIDIA driver path
        hMod = LoadLibraryA("C:\\Windows\\System32\\nvml.dll");
    }
    if (!hMod) {
        // Try NVSMI path (nvidia-smi ships nvml.dll here)
        hMod = LoadLibraryA("C:\\Program Files\\NVIDIA Corporation\\NVSMI\\nvml.dll");
    }
    if (!hMod) return;

    s_nvmlInit = (pfn_nvmlInit)GetProcAddress(hMod, "nvmlInit_v2");
    if (!s_nvmlInit) s_nvmlInit = (pfn_nvmlInit)GetProcAddress(hMod, "nvmlInit");
    s_nvmlShutdown = (pfn_nvmlShutdown)GetProcAddress(hMod, "nvmlShutdown");
    s_nvmlDeviceGetHandleByIndex = (pfn_nvmlDeviceGetHandleByIndex)GetProcAddress(hMod, "nvmlDeviceGetHandleByIndex_v2");
    if (!s_nvmlDeviceGetHandleByIndex)
        s_nvmlDeviceGetHandleByIndex = (pfn_nvmlDeviceGetHandleByIndex)GetProcAddress(hMod, "nvmlDeviceGetHandleByIndex");
    s_nvmlDeviceGetUtilizationRates = (pfn_nvmlDeviceGetUtilizationRates)GetProcAddress(hMod, "nvmlDeviceGetUtilizationRates");
    s_nvmlDeviceGetTemperature = (pfn_nvmlDeviceGetTemperature)GetProcAddress(hMod, "nvmlDeviceGetTemperature");
    s_nvmlDeviceGetClockInfo = (pfn_nvmlDeviceGetClockInfo)GetProcAddress(hMod, "nvmlDeviceGetClockInfo");

    if (!s_nvmlInit || !s_nvmlDeviceGetHandleByIndex) return;

    if (s_nvmlInit() != NVML_SUCCESS) return;

    nvmlDevice_t dev = nullptr;
    if (s_nvmlDeviceGetHandleByIndex(0, &dev) != NVML_SUCCESS) return;

    m_nvmlLib = (void*)hMod;
    m_nvmlDevice = dev;
    m_nvmlLoaded = true;
#endif
}

void EngineProfiler::queryNVML() {
#ifdef _WIN32
    if (!m_nvmlLoaded || !m_nvmlDevice) return;
    nvmlDevice_t dev = (nvmlDevice_t)m_nvmlDevice;

    // Utilization
    if (s_nvmlDeviceGetUtilizationRates) {
        nvmlUtilization_t util = {};
        if (s_nvmlDeviceGetUtilizationRates(dev, &util) == NVML_SUCCESS) {
            m_gpuPercent = (float)util.gpu;
        }
    }

    // Temperature
    if (s_nvmlDeviceGetTemperature) {
        unsigned int temp = 0;
        if (s_nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
            m_gpuTempC = (float)temp;
        }
    }

    // Clock speeds
    if (s_nvmlDeviceGetClockInfo) {
        unsigned int clk = 0;
        if (s_nvmlDeviceGetClockInfo(dev, NVML_CLOCK_GRAPHICS, &clk) == NVML_SUCCESS) {
            m_gpuClockMHz = (float)clk;
        }
        if (s_nvmlDeviceGetClockInfo(dev, NVML_CLOCK_MEM, &clk) == NVML_SUCCESS) {
            m_gpuMemClockMHz = (float)clk;
        }
    }
#endif
}

void EngineProfiler::initPDH() {
#ifdef _WIN32
    if (m_pdhInitialized) return;
    m_pdhInitialized = true;

    m_coreCount = (int)std::thread::hardware_concurrency();
    if (m_coreCount > kMaxCores) m_coreCount = kMaxCores;
    if (m_coreCount <= 0) return;

    // Get base MaxMhz from CallNtPowerInformation (for scaling % Processor Performance)
    std::vector<PROCESSOR_POWER_INFORMATION> ppi(m_coreCount);
    ULONG bufSize = (ULONG)(sizeof(PROCESSOR_POWER_INFORMATION) * m_coreCount);
    NTSTATUS status = CallNtPowerInformation(ProcessorInformation, NULL, 0, ppi.data(), bufSize);
    if (status == 0 && ppi[0].MaxMhz > 0) {
        m_cpuMaxMhz = (float)ppi[0].MaxMhz;
    } else {
        m_cpuMaxMhz = 3000.0f; // fallback
    }

    // Open PDH query for per-core % Processor Performance (includes turbo boost)
    PDH_HQUERY query = nullptr;
    if (PdhOpenQueryA(NULL, 0, &query) != ERROR_SUCCESS) return;
    m_pdhQuery = (void*)query;

    for (int i = 0; i < m_coreCount; i++) {
        char path[256];
        snprintf(path, sizeof(path),
                 "\\Processor Information(0,%d)\\%% Processor Performance", i);
        PDH_HCOUNTER ctr = nullptr;
        if (PdhAddCounterA(query, path, 0, &ctr) == ERROR_SUCCESS) {
            m_pdhCoreCounters[i] = (void*)ctr;
        }
    }

    // First collect establishes baseline for rate counters
    PdhCollectQueryData(query);

    // Try thermal zone counter for CPU temperature
    {
        PDH_HCOUNTER ctr = nullptr;
        const char* paths[] = {
            "\\Thermal Zone Information(\\_TZ.THRM)\\Temperature",
            "\\Thermal Zone Information(\\_TZ.TZ00)\\Temperature",
            "\\Thermal Zone Information(\\_TZ.TZ01)\\Temperature",
            "\\Thermal Zone Information(\\_TZ.CPUZ)\\Temperature"
        };
        for (auto p : paths) {
            ctr = nullptr;
            if (PdhAddCounterA(query, p, 0, &ctr) == ERROR_SUCCESS) {
                m_pdhThermalCounter = (void*)ctr;
                break;
            }
        }
    }
#endif
}

void EngineProfiler::queryCpuFrequencies() {
#ifdef _WIN32
    if (!m_pdhQuery || m_coreCount <= 0) return;

    PDH_HQUERY query = (PDH_HQUERY)m_pdhQuery;
    if (PdhCollectQueryData(query) != ERROR_SUCCESS) return;

    float freqSum = 0.0f;
    int validCores = 0;
    for (int i = 0; i < m_coreCount; i++) {
        if (!m_pdhCoreCounters[i]) continue;
        PDH_FMT_COUNTERVALUE val;
        if (PdhGetFormattedCounterValue((PDH_HCOUNTER)m_pdhCoreCounters[i],
                                        PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) {
            // % Processor Performance can exceed 100% with turbo boost
            // Actual MHz = MaxMhz * (performance% / 100)
            float actualMhz = m_cpuMaxMhz * (float)(val.doubleValue / 100.0);
            if (actualMhz < 100.0f) actualMhz = m_cpuMaxMhz; // sanity
            m_perCoreFreq[i] = actualMhz;
            freqSum += actualMhz;
            validCores++;
        }
    }
    if (validCores > 0)
        m_cpuFreqAvg = freqSum / (float)validCores;

    // CPU temperature from thermal zone counter
    if (m_pdhThermalCounter) {
        PDH_FMT_COUNTERVALUE tval;
        if (PdhGetFormattedCounterValue((PDH_HCOUNTER)m_pdhThermalCounter,
                                        PDH_FMT_DOUBLE, NULL, &tval) == ERROR_SUCCESS) {
            double tempK = tval.doubleValue;
            if (tempK > 1000.0) tempK /= 10.0;  // tenths of Kelvin
            if (tempK > 200.0) tempK -= 273.15;  // Kelvin to Celsius
            m_cpuTempC = (float)tempK;
        }
    }
#endif
}

void EngineProfiler::show(float frameTime, bool& vsync, GLFWwindow* window) {
    // Try loading NVML and PDH on first call
    tryLoadNVML();
    initPDH();

    const int idx = m_head;
    float dt = frameTime * 0.001f; // ms -> seconds

    // Always store frame time
    m_frameMsHistory[(size_t)m_head] = frameTime;
    float currentFPS = (frameTime > 0.0001f) ? (1000.0f / frameTime) : 0.0f;
    m_fpsHistory[(size_t)idx] = currentFPS;
    m_head = (m_head + 1) % kHistorySize;
    m_count = std::min(m_count + 1, kHistorySize);

    // Always write latest known values to ring buffer so graphs are smooth
    m_cpuHistory[idx] = m_cpuPercent;
    m_gpuHistory[idx] = m_gpuPercent;
    m_cpuFreqHistory[idx] = m_cpuFreqAvg;
    m_gpuFreqHistory[idx] = m_gpuClockMHz;
    m_gpuTempHistory[idx] = (m_gpuTempC >= 0) ? m_gpuTempC : 0.0f;
    for (int i = 0; i < m_coreCount; i++)
        m_perCoreFreqHistory[i][idx] = m_perCoreFreq[i];

    // Advance timers
    m_fpsTimer += dt;
    m_usageTimer += dt;
    m_freqTimer += dt;
    m_tempTimer += dt;

    // ---- FPS stats ----
    if (m_fpsTimer >= m_fpsInterval) {
        m_fpsTimer -= m_fpsInterval; // subtract instead of zeroing to avoid drift
        int count = m_count;
        if (count > 10) {
            std::vector<float> sorted;
            sorted.reserve(count);
            int start = (m_head - count + kHistorySize) % kHistorySize;
            float sum = 0.0f;
            for (int j = 0; j < count; j++) {
                int k = (start + j) % kHistorySize;
                float fps = (m_frameMsHistory[k] > 0.0001f) ? (1000.0f / m_frameMsHistory[k]) : 0.0f;
                sorted.push_back(fps);
                sum += fps;
            }
            std::sort(sorted.begin(), sorted.end());
            int onePercent = std::max(1, count / 100);
            float low1Sum = 0.0f;
            for (int j = 0; j < onePercent; j++) low1Sum += sorted[j];
            m_fps1Low = low1Sum / (float)onePercent;
            m_fpsAvg = sum / (float)count;
            m_fpsHigh = sorted.back();
        }
        m_dFps1Low = m_fps1Low;
        m_dFpsAvg = m_fpsAvg;
        m_dFpsHigh = m_fpsHigh;
    }

    // ---- CPU/GPU Usage ----
    if (m_usageTimer >= m_usageInterval) {
        m_usageTimer -= m_usageInterval;
#ifdef _WIN32
        FILETIME ftSysIdle, ftSysKernel, ftSysUser;
        FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
        GetSystemTimeAsFileTime(&ftSysIdle);
        if (GetProcessTimes(GetCurrentProcess(), &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser)) {
            ULARGE_INTEGER now, proc;
            now.LowPart = ftSysIdle.dwLowDateTime;
            now.HighPart = ftSysIdle.dwHighDateTime;
            proc.LowPart = ftProcKernel.dwLowDateTime + ftProcUser.dwLowDateTime;
            proc.HighPart = ftProcKernel.dwHighDateTime + ftProcUser.dwHighDateTime;
            if (m_lastCPUTime > 0) {
                unsigned long long diffTime = now.QuadPart - m_lastCPUTime;
                unsigned long long diffProc = proc.QuadPart - m_lastProcTime;
                if (diffTime > 0) {
                    m_cpuPercent = (float)((double)diffProc / (double)diffTime) * 100.0f;
                    m_cpuPercent = std::clamp(m_cpuPercent, 0.0f, 100.0f);
                }
            }
            m_lastCPUTime = now.QuadPart;
            m_lastProcTime = proc.QuadPart;
        }
#endif
        // GPU usage: use NVML if available, else fallback estimation
        if (!m_nvmlLoaded) {
            float targetMs = vsync ? 16.667f : 1.0f;
            m_gpuPercent = std::clamp((frameTime / targetMs) * 60.0f, 0.0f, 100.0f);
        }

        m_dCpuPct = m_cpuPercent;
        m_dGpuPct = m_gpuPercent;
    }

    // ---- Frequency ----
    if (m_freqTimer >= m_freqInterval) {
        m_freqTimer -= m_freqInterval;

        // Real-time per-core CPU frequency via CallNtPowerInformation
        queryCpuFrequencies();

        // GPU clock via NVML
        if (m_nvmlLoaded) {
            queryNVML(); // this also updates gpuPercent and gpuTemp
        }

        m_dCpuFreq = m_cpuFreqAvg;
        m_dGpuClock = m_gpuClockMHz;
        for (int i = 0; i < m_coreCount; i++) m_dPerCoreFreq[i] = m_perCoreFreq[i];
    }

    // ---- Temperature ----
    if (m_tempTimer >= m_tempInterval) {
        m_tempTimer -= m_tempInterval;
        // GPU temp is updated by queryNVML() in frequency section
        m_dGpuTemp = m_gpuTempC;
        m_dCpuTemp = m_cpuTempC;
    }

    // ====================== RENDER ======================
    ImGui::SetNextWindowSize(ImVec2(580, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Engine Profiler");

    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* glVersion = (const char*)glGetString(GL_VERSION);
    unsigned int cores = std::thread::hardware_concurrency();

    // ===== HARDWARE HEADER =====
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "HARDWARE");
    ImGui::Separator();
    ImGui::Text("GPU: %s", renderer ? renderer : "Unknown");
    ImGui::Text("Vendor: %s", vendor ? vendor : "Unknown");
    ImGui::Text("OpenGL: %s", glVersion ? glVersion : "Unknown");
    ImGui::Text("CPU Cores: %u (logical)", cores);
    if (m_nvmlLoaded) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "[NVML Active]");
    }

    if (ImGui::Checkbox("VSync", &vsync)) {
        if (window) glfwSwapInterval(vsync ? 1 : 0);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ===== REFRESH RATE SETTINGS =====
    if (ImGui::CollapsingHeader("Refresh Intervals")) {
        ImGui::SliderFloat("FPS Refresh (s)", &m_fpsInterval, 0.1f, 5.0f, "%.1f s");
        ImGui::SliderFloat("Usage Refresh (s)", &m_usageInterval, 0.1f, 5.0f, "%.1f s");
        ImGui::SliderFloat("Frequency Refresh (s)", &m_freqInterval, 0.1f, 5.0f, "%.1f s");
        ImGui::SliderFloat("Temperature Refresh (s)", &m_tempInterval, 0.5f, 10.0f, "%.1f s");
        ImGui::Spacing();
    }

    ImGui::Separator();

    // ===== PERFORMANCE (FPS) =====
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "PERFORMANCE");
    ImGui::Separator();

    ImGui::BeginGroup();
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("1%% Low: %.0f", m_dFps1Low);
        ImGui::PopStyleColor();

        ImGui::SameLine(140);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
        ImGui::Text("Avg: %.0f", m_dFpsAvg);
        ImGui::PopStyleColor();

        ImGui::SameLine(280);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("High: %.0f FPS", m_dFpsHigh);
        ImGui::PopStyleColor();
    }
    ImGui::EndGroup();

    ImGui::Text("Frame: %.3f ms (%.0f FPS)", frameTime, currentFPS);

    // FPS Graph — display count scales with FPS refresh interval
    {
        int fpsDisplayCount = std::min(m_count, std::max(30, (int)(m_fpsInterval * 120.0f)));
        struct PlotCtx { const EngineProfiler* self; int dispCount; };
        PlotCtx ctx{this, fpsDisplayCount};
        auto getter = [](void* data, int i) -> float {
            auto* c = static_cast<PlotCtx*>(data);
            int start = (c->self->m_head - c->dispCount + kHistorySize) % kHistorySize;
            int k = (start + i) % kHistorySize;
            return c->self->m_fpsHistory[k];
        };
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));
        float graphMax = (m_dFpsHigh > 10.0f) ? m_dFpsHigh * 1.2f : 200.0f;
        ImGui::PlotLines("##FPSGraph", getter, &ctx, fpsDisplayCount, 0, "FPS", 0.0f, graphMax, ImVec2(-1, 60));
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ===== USAGE =====
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "USAGE");
    ImGui::Separator();

    {
        float usageWidth = ImGui::GetContentRegionAvail().x;
        float halfWidth = (usageWidth - 12.0f) * 0.5f;

        // CPU on left
        ImGui::BeginChild("##CPUUsage", ImVec2(halfWidth, 100), false);
        {
            ImGui::Text("CPU: %.1f%%", m_dCpuPct);
            struct PlotCtx { const EngineProfiler* self; };
            PlotCtx ctx{this};
            auto getter = [](void* data, int i) -> float {
                auto* c = static_cast<PlotCtx*>(data);
                int start = (c->self->m_head - c->self->m_count + kHistorySize) % kHistorySize;
                int k = (start + i) % kHistorySize;
                return c->self->m_cpuHistory[k];
            };
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
            ImGui::PlotLines("##CPUGraph", getter, &ctx, m_count, 0, nullptr, 0.0f, 100.0f, ImVec2(-1, 65));
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // GPU on right
        ImGui::BeginChild("##GPUUsage", ImVec2(halfWidth, 100), false);
        {
            ImGui::Text("GPU: %.1f%%", m_dGpuPct);
            if (!m_nvmlLoaded) { ImGui::SameLine(); ImGui::TextDisabled("(est.)"); }
            struct PlotCtx2 { const EngineProfiler* self; };
            PlotCtx2 ctx2{this};
            auto getter2 = [](void* data, int i) -> float {
                auto* c = static_cast<PlotCtx2*>(data);
                int start = (c->self->m_head - c->self->m_count + kHistorySize) % kHistorySize;
                int k = (start + i) % kHistorySize;
                return c->self->m_gpuHistory[k];
            };
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 1.0f, 0.4f, 1.0f));
            ImGui::PlotLines("##GPUGraph", getter2, &ctx2, m_count, 0, nullptr, 0.0f, 100.0f, ImVec2(-1, 65));
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ===== FREQUENCY =====
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 1.0f, 1.0f), "FREQUENCY");
    ImGui::Separator();

    // Per-core CPU frequency with individual graphs
    if (m_coreCount > 0) {
        ImGui::Text("CPU Average: %.0f MHz", m_dCpuFreq);
        ImGui::Spacing();

        int coresPerRow = 4;
        float colWidth = ImGui::GetContentRegionAvail().x / (float)coresPerRow - 4.0f;
        if (colWidth < 80.0f) colWidth = 80.0f;

        // Find max freq for graph scaling
        float maxFreqAll = 1.0f;
        for (int i = 0; i < m_coreCount; i++) {
            if (m_dPerCoreFreq[i] > maxFreqAll) maxFreqAll = m_dPerCoreFreq[i];
        }
        maxFreqAll *= 1.3f;
        if (maxFreqAll < 500.0f) maxFreqAll = 5000.0f;

        for (int c = 0; c < m_coreCount; c++) {
            if (c % coresPerRow != 0) ImGui::SameLine();

            ImGui::BeginGroup();
            {
                ImGui::PushID(700 + c);
                ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Core %d", c);
                ImGui::Text("%.0f MHz", m_dPerCoreFreq[c]);

                // Per-core frequency graph
                struct CoreCtx { const EngineProfiler* self; int core; };
                CoreCtx cctx{this, c};
                auto coreGetter = [](void* data, int i) -> float {
                    auto* cc = static_cast<CoreCtx*>(data);
                    int start = (cc->self->m_head - cc->self->m_count + kHistorySize) % kHistorySize;
                    int k = (start + i) % kHistorySize;
                    return cc->self->m_perCoreFreqHistory[cc->core][k];
                };
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.6f, 0.3f, 1.0f, 1.0f));
                ImGui::PlotLines("##CoreFreq", coreGetter, &cctx, m_count, 0, nullptr, 0.0f, maxFreqAll, ImVec2(colWidth - 8, 35));
                ImGui::PopStyleColor();

                ImGui::PopID();
            }
            ImGui::EndGroup();
        }
    } else {
        ImGui::TextDisabled("CPU frequency: N/A");
    }

    // GPU clocks — text only, no graph
    ImGui::Spacing();
    if (m_dGpuClock > 0) {
        ImGui::Text("GPU Core Clock: %.0f MHz", m_dGpuClock);
        if (m_gpuMemClockMHz > 0) {
            ImGui::SameLine();
            ImGui::Text("  |  Mem Clock: %.0f MHz", m_gpuMemClockMHz);
        }
    } else {
        ImGui::TextDisabled("GPU clock: N/A (NVML not loaded)");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ===== TEMPERATURE =====
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "TEMPERATURE");
    ImGui::Separator();

    // CPU temperature
    if (m_dCpuTemp >= 0) {
        ImVec4 cpuTempCol = (m_dCpuTemp > 90) ? ImVec4(1,0.2f,0.2f,1)
                         : (m_dCpuTemp > 75) ? ImVec4(1,0.7f,0.2f,1)
                         : ImVec4(0.3f,1,0.5f,1);
        ImGui::TextColored(cpuTempCol, "CPU: %.0f \xC2\xB0" "C", m_dCpuTemp);
    } else {
        ImGui::TextDisabled("CPU temp: N/A");
    }

    // GPU temperature
    ImGui::SameLine(200);
    if (m_dGpuTemp >= 0) {
        ImVec4 gpuTempCol = (m_dGpuTemp > 85) ? ImVec4(1,0.2f,0.2f,1)
                         : (m_dGpuTemp > 70) ? ImVec4(1,0.7f,0.2f,1)
                         : ImVec4(0.3f,1,0.5f,1);
        ImGui::TextColored(gpuTempCol, "GPU: %.0f \xC2\xB0" "C", m_dGpuTemp);
    } else {
        ImGui::TextDisabled("GPU temp: N/A");
    }

    ImGui::End();
}
