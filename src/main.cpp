#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Allocator.hpp"
#include "Camera.hpp"
#include "Chunk.hpp"
#include "ECS.hpp"
#include "Framebuffer.hpp"
#include "GUIManager.hpp"
#include "GLMesh.hpp"
#include "MeshBuilder.hpp"
#include "Renderer.hpp"
#include "AudioManager.hpp"
#include "Shader.hpp"
#include "TaskScheduler.hpp"
#include "Texture.hpp"
#include "World.hpp"
#include "WaterPhysics.hpp"  // legacy — kept for reference; FluidSimulator is the active system
#include "FluidSimulator.hpp"
#include "BiomeRegistry.hpp"
#include "MobAI.hpp"
#include "Registry.hpp"
#include "IntroScreen.hpp"

#include "FastNoiseLite.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>
#include <cstring>
#include <csignal>
#include <ctime>
#ifdef _WIN32
#  include <windows.h>
#endif

// ---------------------------------------------------------------------------
// Crash Logger — writes a timestamped report to logs/ next to the executable
// ---------------------------------------------------------------------------
namespace CrashLogger {

static std::string s_logsDir;

static std::string currentTimestamp() {
    std::time_t t = std::time(nullptr);
    char buf[32] = {};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", std::localtime(&t));
    return buf;
}

static void writeReport(const std::string& reason, const std::string& detail = "") {
    if (s_logsDir.empty()) return;
    try {
        std::filesystem::create_directories(s_logsDir);
        std::string path = s_logsDir + "/crash_" + currentTimestamp() + ".log";
        std::ofstream f(path);
        if (!f.is_open()) return;
        f << "=== Voxel-Sim Architect Crash Report ===\n";
        f << "Timestamp : " << currentTimestamp() << "\n";
        f << "Version   : Beta-Dev v0.6\n";
        f << "Reason    : " << reason << "\n";
        if (!detail.empty()) f << "Detail    : " << detail << "\n";
#ifdef _WIN32
        f << "Platform  : Windows\n";
#else
        f << "Platform  : Linux/Other\n";
#endif
        f << "\nIf the crash is reproducible, check the last operation in the engine log.\n";
        f.flush();
    } catch (...) {}
}

static void signalHandler(int sig) {
    const char* name = "UNKNOWN";
    switch (sig) {
        case SIGSEGV: name = "SIGSEGV (Segmentation Fault)";   break;
        case SIGABRT: name = "SIGABRT (Abort / Assert)";       break;
        case SIGFPE:  name = "SIGFPE (Floating Point Error)";  break;
        case SIGILL:  name = "SIGILL (Illegal Instruction)";   break;
        case SIGTERM: name = "SIGTERM (Termination Request)";  break;
    }
    writeReport("Signal", name);
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

#ifdef _WIN32
static LONG WINAPI sehHandler(EXCEPTION_POINTERS* ep) {
    char detail[128] = {};
    std::snprintf(detail, sizeof(detail),
                  "Exception code 0x%08lX at address %p",
                  static_cast<unsigned long>(ep->ExceptionRecord->ExceptionCode),
                  ep->ExceptionRecord->ExceptionAddress);
    writeReport("Unhandled SEH Exception", detail);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static void setup(const std::string& logsDir) {
    s_logsDir = logsDir;
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE,  signalHandler);
    std::signal(SIGILL,  signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    SetUnhandledExceptionFilter(sehHandler);
#endif
}

} // namespace CrashLogger

enum WeatherType { WEATHER_CLEAR, WEATHER_RAIN, WEATHER_SNOW };

struct SnowParticle {
    Vec3 position;
    Vec3 velocity;
    float life;
};

// Lava: rising smoke (grey), embers (orange sparks), and fire wisps
struct LavaParticle {
    Vec3 position;  // World-space absolute position
    Vec3 velocity;
    float life;
    float maxLife;
    uint8_t type;   // 0 = smoke, 1 = ember, 2 = fire wisp
};

struct ChatMessage {
    std::string text;
    ImVec4 color = ImVec4(1,1,1,1);
};

void saveUIConfig(bool showProfiler, bool showMemory, bool showECS, bool showWorldEditor,
                  bool showSettings, bool showSoundEditor, bool showBlockDesigner,
                  bool showMobDesigner, bool showInteractionEditor, bool showToolDesigner,
                  bool showSoundDesigner, bool showAdvWorldEditor, bool fullscreen) {
    std::ofstream file("ui_config.txt");
    if (file.is_open()) {
        file << showProfiler << "\n";
        file << showMemory << "\n";
        file << showECS << "\n";
        file << showWorldEditor << "\n";
        file << showSettings << "\n";
        file << showSoundEditor << "\n";
        file << showBlockDesigner << "\n";
        file << showMobDesigner << "\n";
        file << showInteractionEditor << "\n";
        file << showToolDesigner << "\n";
        file << fullscreen << "\n";
        file << showSoundDesigner << "\n";
        file << showAdvWorldEditor << "\n";
        file.close();
    }
}

void loadUIConfig(bool& showProfiler, bool& showMemory, bool& showECS, bool& showWorldEditor,
                  bool& showSettings, bool& showSoundEditor, bool& showBlockDesigner,
                  bool& showMobDesigner, bool& showInteractionEditor, bool& showToolDesigner,
                  bool& showSoundDesigner, bool& showAdvWorldEditor, bool& fullscreen) {
    std::ifstream file("ui_config.txt");
    if (file.is_open()) {
        file >> showProfiler;
        file >> showMemory;
        file >> showECS;
        file >> showWorldEditor;
        file >> showSettings;
        file >> showSoundEditor;
        file >> showBlockDesigner;
        file >> showMobDesigner;
        file >> showInteractionEditor;
        file >> showToolDesigner;
        file >> fullscreen;
        file >> showSoundDesigner;
        file >> showAdvWorldEditor;
        file.close();
    }
}

int main() {
    // --- Crash logger: must be set up before anything else ---
    {
#ifdef _WIN32
        char exeBuf[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
        std::string logsDir = std::filesystem::path(exeBuf).parent_path().string() + "/logs";
#else
        std::string logsDir = std::filesystem::current_path().string() + "/logs";
#endif
        CrashLogger::setup(logsDir);
    }

    std::cout << "Voxel-Sim Architect Engine Starting..." << std::endl;
    GameRegistry::getInstance().init();

    // 1. Memory Management
    ArenaAllocator mainAllocator(1024 * 1024 * 100); // 100MB Arena
    
    // 2. Procedural Generation Setup
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(1337 + 20); // Same seed as worldSeed + 20
    biomeNoise.SetFrequency(0.02f * 0.04f); // Matches Chunk.cpp: frequency * 0.04f
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeNoise.SetFractalOctaves(2);

    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(1337 + 10);
    continentalNoise.SetFrequency(0.02f * 0.07f); // Matches Chunk.cpp: frequency * 0.07f
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentalNoise.SetFractalOctaves(3);

    FastNoiseLite mountainNoise;
    mountainNoise.SetSeed(1337 + 1);
    mountainNoise.SetFrequency(0.02f * 0.55f);
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainNoise.SetFractalOctaves(4);
    
    // 3. Multi-threaded Task Scheduler
    TaskScheduler scheduler(std::thread::hardware_concurrency());

    // 4. ECS Setup
    Registry registry;
    
    // 5. Renderer & GUI Initialization
    Renderer renderer;
    int screenW = 1280, screenH = 720;
    
    // Get monitor resolution for full resolution support
    if (glfwInit()) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        screenW = mode->width;
        screenH = mode->height;
    }

    if (!renderer.init(screenW, screenH, "Voxel-Sim Architect V0.6 Beta", true)) {
        return -1;
    }

    GUIManager gui;
    gui.init(renderer.getWindow());

    Framebuffer viewportBuffer;
    viewportBuffer.init(screenW, screenH);

    Texture atlas;
    atlas.generateAtlas();
    gui.setAtlasTextureID(atlas.getID());
    gui.setAtlasPointer(&atlas);

    // Game State
    WeatherType currentWeather = WEATHER_CLEAR;
    std::vector<std::string> chatHistory;
    char chatInput[256] = "";
    bool chatOpen = false;
    bool inventoryOpen = false;
    // Registry registry; // Already initialized in main() section 6
    std::vector<SnowParticle> snowParticles;
    std::vector<LavaParticle> lavaParticles; // Smoke, embers, wisps above lava/fire
    float worldTime = 6000.0f; // Start at noon
    bool isRaining = false;
    float playerOnFireSeconds = 0.0f;
    float playerHp = 20.0f;
    Vec3  spawnPosition = {0.0f, 50.0f, 0.0f}; // Updated when a world is loaded
    float playerDeathTimer = 0.0f;
    float playerHurtTimer = 0.0f;  // Red flash remaining after being hit
    float playerInvincTimer = 0.0f; // Invincibility frames after a hit
    float toolSwingT = 0.0f;        // 0=resting, 1=full swing (drives hand animation)

    // World save/load state
    char worldSaveName[64] = "My World";
    bool showWorldList = false;
    std::vector<WorldSaveInfo> worldSaves;
    std::string loadedWorldFile;

    // UI State
    bool showProfiler = true;
    bool showMemory = true;
    bool showECS = true;
    bool showWorldEditor = true;
    bool showSettings = false;
    bool showEscMenu  = false;  // ESC in-game pause / audio modal (separate from Preferences)
    bool showSoundEditor = false;
    bool showBlockDesigner = false;
    bool showMobDesigner = false;
    bool showInteractionEditor = false;
    bool showToolDesigner = false;
    bool showWeatherDesigner = false;
    bool showSoundDesigner = false;
    bool showAdvWorldEditor = false;
    bool showTextureDesigner = false;
    bool vsync = false; // Off by default — enables uncapped / 100+ FPS; toggle in Settings
    bool wireframe = false;
    bool backfaceCulling = false;
    bool fullscreen = true;
    bool menuMode = true; // Start in menu mode

    loadUIConfig(showProfiler, showMemory, showECS, showWorldEditor, showSettings, showSoundEditor, showBlockDesigner, showMobDesigner, showInteractionEditor, showToolDesigner, showSoundDesigner, showAdvWorldEditor, fullscreen);

    renderer.setVSync(vsync);
    renderer.setWireframe(wireframe);
    renderer.setBackfaceCulling(backfaceCulling);
    renderer.setFullscreen(fullscreen);

    // Resolve asset paths
    const auto findProjectRoot = []() -> std::filesystem::path {
        std::filesystem::path p = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i) {
            if (std::filesystem::exists(p / "assets" / "shaders" / "voxel.vert")) {
                return p;
            }
            if (!p.has_parent_path()) break;
            p = p.parent_path();
        }
        return std::filesystem::current_path();
    };
    const std::filesystem::path root = findProjectRoot();
    
    if (!AudioManager::getInstance().init(root.string())) {
        std::cerr << "Warning: Audio system failed to initialize." << std::endl;
    }

    const std::string voxelVert = (root / "assets" / "shaders" / "voxel.vert").string();
    const std::string voxelFrag = (root / "assets" / "shaders" / "voxel.frag").string();

    Shader voxelShader;
    if (!voxelShader.loadFromFiles(voxelVert, voxelFrag)) {
        std::cerr << "Failed to load voxel shader.\n";
        return -2;
    }

    auto buildCubeMeshForTile = [&](int tx, int ty) {
        GLMesh mesh;
        MeshBuilder mb;
        float u = (float)tx / 16.0f;
        float v = (float)ty / 16.0f;
        float s = 1.0f / 16.0f;
        mb.addFace({0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}, {0,0,-1}, u, v, u+s, v+s, 1.0f); // -Z
        mb.addFace({0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}, {0,0,1},  u, v, u+s, v+s, 1.0f); // +Z
        mb.addFace({0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}, {-1,0,0}, u, v, u+s, v+s, 1.0f); // -X
        mb.addFace({1,0,0}, {1,0,1}, {1,1,1}, {1,1,0}, {1,0,0},  u, v, u+s, v+s, 1.0f); // +X
        mb.addFace({0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}, {0,-1,0}, u, v, u+s, v+s, 1.0f); // -Y
        mb.addFace({0,1,0}, {1,1,0}, {1,1,1}, {0,1,1}, {0,1,0},  u, v, u+s, v+s, 1.0f); // +Y
        mesh.upload(mb.getVertices());
        return mesh;
    };

    // Mob meshes (atlas row 3, cols 0..8)
    std::vector<GLMesh> mobMeshes;
    mobMeshes.reserve((size_t)MOB_COUNT);
    for (int i = 0; i < (int)MOB_COUNT; ++i) {
        mobMeshes.push_back(buildCubeMeshForTile(i, 3));
    }

    // Block break crack overlay meshes (atlas row 2, cols 0..9)
    std::vector<GLMesh> crackMeshes;
    crackMeshes.reserve(10);
    for (int i = 0; i < 10; ++i) {
        crackMeshes.push_back(buildCubeMeshForTile(i, 2));
    }

    // Sun/Moon/Cloud billboard meshes (use cubes as simple sprites; scaled thin later)
    GLMesh sunMesh = buildCubeMeshForTile(14, 2);
    GLMesh moonMesh = buildCubeMeshForTile(13, 2);
    GLMesh cloudMesh = buildCubeMeshForTile(15, 2);
    GLMesh starMesh = buildCubeMeshForTile(12, 2);

    // World Management
    World world;
    int worldSeed = 1337;
    float worldFrequency = 0.02f;
    int worldBaseHeight = 32;
    int renderDistance = 4;
    uint8_t selectedBlock = 1; // 1=Dirt, 2=Grass, 3=Stone
    int equippedToolId = 0;    // 0 = no tool (hand)

    struct Inventory {
        int counts[256] = {0};
    } inventory;
    // Start with some blocks
    for (int i = 1; i < 256; ++i) {
        inventory.counts[i] = 64;
    }

    struct Advancement {
        std::string title;
        bool achieved = false;
        int requiredCount = 0;
        uint8_t blockType = 0;
    };
    std::vector<Advancement> advancements = {
        {"Stone Age", false, 10, BLOCK_STONE},
        {"Lumberjack", false, 5, BLOCK_WOOD},
        {"Gardener", false, 20, BLOCK_GRASS},
        {"Mountaineer", false, 1, BLOCK_SNOW}
    };

    Camera camera;
    camera.setPosition({0.0f, (float)(worldBaseHeight + 20), 0.0f});
    {
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(renderer.getWindow(), &fbW, &fbH);
        if (fbH > 0) {
            camera.setAspect((float)fbW / (float)fbH);
        }
    }

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool hadMouse = false;

    auto isWater = [&](uint8_t b) { return blockIsWater(b); };
    auto isLava = [&](uint8_t b) { return blockIsLava(b); };

    auto collideAABB = [&](const Vec3& pos, const Vec3& halfExt) -> bool {
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                for (int sy = -1; sy <= 1; ++sy) {
                    float px = pos.x + halfExt.x * (float)sx;
                    float pz = pos.z + halfExt.z * (float)sz;
                    float py = pos.y + halfExt.y * (float)sy;
                    if (world.isSolid((int)std::floor(px), (int)std::floor(py), (int)std::floor(pz))) return true;
                }
            }
        }
        return false;
    };

    auto findGroundY = [&](float wx, float wz, int startY) -> float {
        int ix = (int)std::floor(wx);
        int iz = (int)std::floor(wz);
        for (int y = std::min(startY, Chunk::SizeY - 2); y >= 1; --y) {
            if (world.isSolid(ix, y, iz)) {
                return (float)(y + 1);
            }
        }
        return 80.0f;
    };

    // 6. ECS Setup
    Entity player = registry.createEntity();
    registry.addComponent(player, Transform{{0, 0, 0}});
    
    std::cout << "Engine initialized successfully." << std::endl;

    // Animated intro screen
    IntroScreen intro;
    intro.init();
    bool introActive = true;

    // Main Loop
    bool breaking = false;
    int breakX = 0, breakY = 0, breakZ = 0;
    float breakProgress = 0.0f;
    uint8_t breakType = 0;

    auto lastTime = std::chrono::high_resolution_clock::now();
    // FluidSimulator (TDD §1) — event-driven dormant/active fluid state machine.
    // Generation water/lava stays STATE_DORMANT until a neighbour block changes.
    // ProcessTick() drains BlockUpdateEvents and transitions fluids to ACTIVE.
    FluidSimulator fluidSim;
    auto& fluidDistances  = fluidSim.distances();
    auto& waterFluidQueue = fluidSim.waterQueue();
    auto& lavaFluidQueue  = fluidSim.lavaQueue();
    auto  fluidKey        = [](int x, int y, int z) -> int64_t { return FluidSimulator::encodeKey(x, y, z); };
    auto  decodeFluidKey  = [](int64_t k, int& ox, int& oy, int& oz) { FluidSimulator::decodeKey(k, ox, oy, oz); };

    // Persistent mob part meshes — built once per (mobType, partIdx), reused every frame.
    // Previously these were created/uploaded/destroyed 80+ times per frame (critical bottleneck).
    std::unordered_map<int, GLMesh> mobMeshCache;
    mobMeshCache.reserve(MOB_COUNT * 20); // pre-size to avoid rehash during gameplay

    // Precompute star directions once — avoids srand(42) + 150 rand() calls every night frame
    struct StarDir { float az, el; };
    std::vector<StarDir> precomputedStars;
    precomputedStars.reserve(150);
    srand(42);
    for (int i = 0; i < 150; ++i) {
        StarDir sd;
        sd.az = (float)(rand() % 360) * 0.01745f;
        sd.el = (float)(rand() % 180) * 0.01745f;
        precomputedStars.push_back(sd);
    }

    while (!renderer.shouldClose()) {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaMs = (float)std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
        lastTime = currentTime;

        const float dt = deltaMs * 0.001f;

        // Intro screen: render and skip rest of frame while active
        if (introActive) {
            gui.beginFrame();
            introActive = !intro.update(dt);
            intro.render();
            gui.endFrame();
            renderer.swapBuffers();
            continue;
        }

        // Global Input Handling
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.getWindow(), true);
        }

        // Chat Toggle
        static bool tPressed = false;
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_T) == GLFW_PRESS && !chatOpen) {
            if (!tPressed) {
                chatOpen = true;
                menuMode = true;
                tPressed = true;
            }
        } else tPressed = false;

        static bool f10Pressed = false;
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_F10) == GLFW_PRESS) {
            if (!f10Pressed) {
                menuMode = !menuMode;
                f10Pressed = true;
            }
        } else {
            f10Pressed = false;
        }

        // World Update (Auto-generation)
        world.setRenderDistance(renderDistance);
        world.update(camera.position(), noise, worldSeed, worldFrequency, worldBaseHeight, &scheduler);

        // World tick — 20 ticks/second for fluid and fire simulation
        {
            static float tickAccumulator = 0.0f;
            tickAccumulator += dt;
            constexpr float TICK_INTERVAL = 1.0f / 20.0f; // 50 ms per tick
            while (tickAccumulator >= TICK_INTERVAL) {
                world.tick(isRaining);
                tickAccumulator -= TICK_INTERVAL;
            }
        }

        // Spawn mobs in new chunks, and register generation fluids as dormant
        for (const auto& coord : world.getNewChunks()) {
            Chunk* c = world.getChunk(coord.first, coord.second);
            if (c) {
                MobAI::spawnMobsInChunk(registry, c, coord.first, coord.second, biomeNoise, continentalNoise, mountainNoise);
                // Mark all generation-placed water/lava as dormant — they won't
                // spread until a neighbouring block is disturbed by the player.
                fluidSim.registerChunk(c, coord.first, coord.second);
            }
        }
        // Remove unloaded chunks from the dormant registry so m_activated doesn't leak.
        for (const auto& coord : world.getRemovedChunks())
            fluidSim.unregisterChunk(coord.first, coord.second);

        // Environmental Ambient Sounds Update
        {
            static float envUpdateTimer = 0.0f;
            envUpdateTimer += (float)dt;
            if (envUpdateTimer > 0.5f) { 
                envUpdateTimer = 0.0f;
                Vec3 p = camera.position();
                bool nearWater = false, nearLava = false, nearFire = false;
                int r = 5;
                for(int x = -r; x <= r; x++) {
                    for(int y = -r; y <= r; y++) {
                        for(int z = -r; z <= r; z++) {
                            uint8_t b = world.getBlock((int)p.x + x, (int)p.y + y, (int)p.z + z);
                            if(blockIsWater(b)) nearWater = true;
                            if(blockIsLava(b)) nearLava = true;
                            if(b == BLOCK_FIRE) nearFire = true;
                        }
                    }
                }
                auto& am = AudioManager::getInstance();
                am.setAmbientLoop("water", "assets/sounds/water_ambient.wav", nearWater, 0.45f);
                am.setAmbientLoop("lava", "assets/sounds/lava_ambient.wav", nearLava, 0.65f);
                am.setAmbientLoop("fire", "assets/sounds/fire_ambient.wav", nearFire, 0.55f);

                // Weather-driven ambient sound from Weather Designer
                const auto& wp = gui.getWeatherDesigner().getActivePreset();
                bool hasWeatherSound = !wp.ambientSound.empty() && wp.particleCount > 0;
                am.setAmbientLoop("rain", wp.ambientSound.empty() ? "assets/sounds/rain_ambient.wav" : wp.ambientSound, hasWeatherSound, wp.ambientVolume);
            }
            // Weather-driven thunder
            {
                const auto& wp = gui.getWeatherDesigner().getActivePreset();
                if (wp.hasThunder && wp.particleCount > 0) {
                    int thunderChance = std::max(1, (int)(1.0f / std::max(wp.thunderFrequency, 0.0001f)));
                    if ((std::rand() % thunderChance) == 0) {
                        Vec3 p = camera.position();
                        Vec3 tPos = {p.x + (std::rand()%160-80), p.y + 60, p.z + (std::rand()%160-80)};
                        AudioManager::getInstance().playThunder(tPos, p);
                    }
                }
            }
        }

        renderer.clear();

        // Update camera aspect
        {
            int fbW = 0, fbH = 0;
            glfwGetFramebufferSize(renderer.getWindow(), &fbW, &fbH);
            if (fbH > 0) {
                camera.setAspect((float)fbW / (float)fbH);
            }
        }

        // GUI Frame
        gui.beginFrame();
        ImGuizmo::BeginFrame();

        // Editor-style dockspace (production-feel).
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        
        gui.showMainMenuBar(showProfiler, showMemory, showECS, showWorldEditor, showSettings, showSoundEditor, showBlockDesigner, showMobDesigner, showInteractionEditor, showToolDesigner, showWeatherDesigner, showSoundDesigner, showAdvWorldEditor, showTextureDesigner);
        gui.showWindowTabBar(showProfiler, showMemory, showECS, showWorldEditor, showSettings, showSoundEditor, showBlockDesigner, showMobDesigner, showInteractionEditor, showToolDesigner, showWeatherDesigner, showSoundDesigner, showAdvWorldEditor, showTextureDesigner);

        if (showProfiler) gui.showProfiler(deltaMs);
        if (showMemory) {
            auto& chunkAlloc = Chunk::getAllocator();
            gui.showMemoryInspector(mainAllocator.getOffset(), mainAllocator.getSize(), chunkAlloc.getUsedCount(), chunkAlloc.getTotalCount());
        }
        if (showECS) gui.showECSEditor();
        gui.showSoundEditor(&showSoundEditor);
        gui.showBlockDesigner(&showBlockDesigner);
        gui.showMobDesigner(&showMobDesigner);
        gui.showToolDesigner(&showToolDesigner);
        gui.showInteractionEditor(&showInteractionEditor);
        gui.showWeatherDesigner(&showWeatherDesigner);
        gui.showAdvWorldEditor(&showAdvWorldEditor);
        gui.showSoundDesigner(&showSoundDesigner);
        gui.showTextureDesigner(&showTextureDesigner);
        gui.showHelperWindow();  // HelperWindow manages its own open/close state
        gui.coordinateMobBlockEdit(showBlockDesigner);
        if (showSettings) gui.showSettings(&showSettings, vsync, wireframe, fullscreen, backfaceCulling, renderer);

        World::RaycastResult raycastRes = world.raycast(camera.position(), camera.forward(), 10.0f);

        // Viewport Window
        ImVec2 viewportSize(0, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Viewport")) {
            viewportSize = ImGui::GetContentRegionAvail();
            if (viewportSize.x > 0 && viewportSize.y > 0) {
                viewportBuffer.resize((int)viewportSize.x, (int)viewportSize.y);
                camera.setAspect(viewportSize.x / viewportSize.y);
                
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImGui::Image((ImTextureID)(uintptr_t)viewportBuffer.getTexture(), viewportSize, ImVec2(0, 1), ImVec2(1, 0));

                // Draw Crosshair (Minecraft style with shadow)
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 center = ImVec2(screenPos.x + viewportSize.x * 0.5f, screenPos.y + viewportSize.y * 0.5f);
                float chSize = 8.0f;
                float chThick = 2.0f;
                
                // Shadow/Outline
                drawList->AddLine(ImVec2(center.x - chSize - 1, center.y + 1), ImVec2(center.x + chSize + 1, center.y + 1), IM_COL32(0, 0, 0, 150), chThick + 1.0f);
                drawList->AddLine(ImVec2(center.x + 1, center.y - chSize - 1), ImVec2(center.x + 1, center.y + chSize + 1), IM_COL32(0, 0, 0, 150), chThick + 1.0f);
                
                // Inner white cross
                drawList->AddLine(ImVec2(center.x - chSize, center.y), ImVec2(center.x + chSize, center.y), IM_COL32(255, 255, 255, 220), chThick);
                drawList->AddLine(ImVec2(center.x, center.y - chSize), ImVec2(center.x, center.y + chSize), IM_COL32(255, 255, 255, 220), chThick);

                // Draw Hotbar (Minecraft style)
                ImTextureID texId = (ImTextureID)(uintptr_t)atlas.getID();
                float slotSize = 44.0f;
                float hbPadding = 4.0f;
                float hbWidth = (slotSize * 9.0f) + (hbPadding * 10.0f);
                float hbHeight = slotSize + (hbPadding * 2.0f);
                ImVec2 hbPos = ImVec2(screenPos.x + (viewportSize.x - hbWidth) * 0.5f, screenPos.y + viewportSize.y - hbHeight - 10.0f);
                
                // Hotbar Background
                drawList->AddRectFilled(hbPos, ImVec2(hbPos.x + hbWidth, hbPos.y + hbHeight), IM_COL32(30, 30, 30, 200), 2.0f);
                drawList->AddRect(hbPos, ImVec2(hbPos.x + hbWidth, hbPos.y + hbHeight), IM_COL32(100, 100, 100, 200), 2.0f, 0, 2.0f);
                
                for (int i = 1; i <= 9; ++i) {
                    ImVec2 slotPos = ImVec2(hbPos.x + hbPadding + (i-1) * (slotSize + hbPadding), hbPos.y + hbPadding);
                    ImVec2 slotEnd = ImVec2(slotPos.x + slotSize, slotPos.y + slotSize);
                    
                    // Slot Background (inner shadow effect)
                    drawList->AddRectFilled(slotPos, slotEnd, IM_COL32(139, 139, 139, 150), 0.0f);
                    drawList->AddRect(slotPos, slotEnd, IM_COL32(55, 55, 55, 200), 0.0f, 0, 2.0f);
                    
                    // Selection Highlight
                    if (selectedBlock == i) {
                        drawList->AddRect(ImVec2(slotPos.x - 2, slotPos.y - 2), ImVec2(slotEnd.x + 2, slotEnd.y + 2), IM_COL32(255, 255, 255, 255), 2.0f, 0, 3.0f);
                    }

                    // Block Icon (Texture)
                    const auto& def = GameRegistry::getInstance().getBlock(i);
                    int tx = def.texX;
                    int ty = def.texY;
                    
                    ImVec2 uv0 = ImVec2(tx / 16.0f, ty / 16.0f);
                    ImVec2 uv1 = ImVec2((tx + 1) / 16.0f, (ty + 1) / 16.0f);
                    drawList->AddImage(texId, ImVec2(slotPos.x + 4, slotPos.y + 4), ImVec2(slotEnd.x - 4, slotEnd.y - 4), uv0, uv1);

                    // Count (Minecraft style shadow text)
                    char countBuf[16];
                    snprintf(countBuf, 16, "%d", inventory.counts[i]);
                    ImVec2 textPos = ImVec2(slotPos.x + slotSize - 14, slotPos.y + slotSize - 16);
                    drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(60, 60, 60, 255), countBuf); // Shadow
                    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), countBuf); // Text
                }

                // Tool-in-hand: bottom-right corner of viewport, with swing animation
                {
                    float margin = 14.0f;
                    float iconSz = 82.0f;
                    // Anchor above the hotbar
                    float cx = screenPos.x + viewportSize.x - margin - iconSz * 0.5f;
                    float cy = screenPos.y + viewportSize.y - hbHeight - margin - iconSz * 0.5f;

                    // Swing: resting = –25°, full swing adds +50° * sin(t*π)
                    float baseA   = -25.0f * 0.01745329f;
                    float swingA  =  50.0f * 0.01745329f * std::sin(toolSwingT * 3.14159f);
                    float angle   = baseA + swingA;
                    float cosA = std::cos(angle), sinA = std::sin(angle);

                    auto rot = [&](float ox, float oy) -> ImVec2 {
                        return ImVec2(cx + ox * cosA - oy * sinA,
                                      cy + ox * sinA + oy * cosA);
                    };

                    if (equippedToolId > 0) {
                        const ToolDefinition* tool = GameRegistry::getInstance().getTool(equippedToolId);
                        if (tool) {
                            ImU32 headCol = IM_COL32(
                                (int)(tool->color.x * 220),
                                (int)(tool->color.y * 220),
                                (int)(tool->color.z * 220), 235);
                            ImU32 rimCol = IM_COL32(
                                (int)(tool->color.x * 130),
                                (int)(tool->color.y * 130),
                                (int)(tool->color.z * 130), 235);

                            // Handle (brown stick)
                            float hw = 7.0f, hh = 44.0f;
                            drawList->AddQuadFilled(
                                rot(-hw*0.5f, -hh*0.5f), rot(hw*0.5f, -hh*0.5f),
                                rot(hw*0.5f,  hh*0.5f),  rot(-hw*0.5f, hh*0.5f),
                                IM_COL32(105, 72, 38, 230));
                            // Tool head
                            float tw = 28.0f, th = 22.0f, ty = -hh * 0.5f - th;
                            drawList->AddQuadFilled(
                                rot(-tw*0.5f, ty),       rot(tw*0.5f, ty),
                                rot(tw*0.5f,  ty + th),  rot(-tw*0.5f, ty + th),
                                headCol);
                            drawList->AddQuad(
                                rot(-tw*0.5f, ty),       rot(tw*0.5f, ty),
                                rot(tw*0.5f,  ty + th),  rot(-tw*0.5f, ty + th),
                                rimCol, 1.5f);
                            // Tool name below the icon
                            float labelX = cx - iconSz * 0.4f;
                            float labelY = cy + iconSz * 0.42f;
                            drawList->AddText(ImVec2(labelX, labelY),
                                IM_COL32(220, 220, 220, 200), tool->name.c_str());
                        }
                    } else {
                        // Bare fist
                        float fw = 22.0f, fh = 18.0f;
                        drawList->AddQuadFilled(
                            rot(-fw*0.5f,-fh*0.5f), rot(fw*0.5f,-fh*0.5f),
                            rot(fw*0.5f, fh*0.5f),  rot(-fw*0.5f, fh*0.5f),
                            IM_COL32(245, 210, 165, 220));
                        drawList->AddQuad(
                            rot(-fw*0.5f,-fh*0.5f), rot(fw*0.5f,-fh*0.5f),
                            rot(fw*0.5f, fh*0.5f),  rot(-fw*0.5f, fh*0.5f),
                            IM_COL32(180, 140, 100, 200), 1.0f);
                    }
                }

                // Debug Info Overlay
                ImGui::SetCursorScreenPos(ImVec2(screenPos.x + 10, screenPos.y + 10));
                ImGui::BeginGroup();
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Voxel-Sim Architect V0.6 Beta");
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Pos: %.1f, %.1f, %.1f", camera.position().x, camera.position().y, camera.position().z);
                ImGui::Text("Chunks: %zu", world.getChunkCount());
                ImGui::EndGroup();

                // ── Biome Name HUD ─────────────────────────────────────────────────────
                // Displayed centre-bottom of viewport just above the hotbar.
                // Uses the same noise thresholds as Chunk::getBiomeAt() for accuracy.
                {
                    static float     biomeUpdateTimer = 0.0f;
                    static BiomeType currentBiome     = BIOME_PLAINS;

                    biomeUpdateTimer += dt;
                    if (biomeUpdateTimer >= 0.4f) {
                        biomeUpdateTimer = 0.0f;
                        const Vec3& cp = camera.position();
                        float pwx = cp.x, pwz = cp.z;

                        float cn_v = continentalNoise.GetNoise(pwx, pwz);
                        if (cn_v < -0.30f) {
                            currentBiome = BIOME_OCEAN;
                        } else {
                            float bn_v = biomeNoise.GetNoise(pwx, pwz);
                            float mn_v = mountainNoise.GetNoise(pwx, pwz);
                            if      (mn_v > 0.42f && bn_v > -0.18f && bn_v < 0.82f) {
                                currentBiome = BIOME_MOUNTAINS;
                            } else if (bn_v < -0.20f) {
                                currentBiome = BIOME_POLAR;
                            } else if (bn_v <  0.05f) {
                                currentBiome = BIOME_SNOWY;
                            } else if (bn_v <  0.35f) {
                                currentBiome = BIOME_PLAINS;
                            } else if (bn_v <  0.50f) {
                                currentBiome = BIOME_SAVANNA;
                            } else if (bn_v <  0.65f) {
                                currentBiome = BIOME_DESERT;
                            } else if (bn_v <  0.85f) {
                                currentBiome = BIOME_JUNGLE;
                            } else {
                                currentBiome = BIOME_ASHWORLD;
                            }
                        }
                    }

                    const BiomeDef& bdef       = BiomeRegistry::get(currentBiome);
                    const char*     biomeName  = bdef.name;
                    ImU32 biomeTextColor       = IM_COL32(bdef.hudR, bdef.hudG, bdef.hudB, 220);

                    // Centre-bottom pill — 26 px above the hotbar
                    ImVec2 biomeTextSz = ImGui::CalcTextSize(biomeName);
                    float  biomeBx = screenPos.x + (viewportSize.x - biomeTextSz.x) * 0.5f;
                    float  biomeBy = screenPos.y + viewportSize.y - hbHeight - 46.0f;
                    constexpr float kBiomePad = 9.0f;
                    drawList->AddRectFilled(
                        ImVec2(biomeBx - kBiomePad,              biomeBy - 4.0f),
                        ImVec2(biomeBx + biomeTextSz.x + kBiomePad, biomeBy + biomeTextSz.y + 4.0f),
                        IM_COL32(0, 0, 0, 110), 7.0f);
                    drawList->AddText(ImVec2(biomeBx + 1.0f, biomeBy + 1.0f), IM_COL32(0, 0, 0, 100), biomeName);
                    drawList->AddText(ImVec2(biomeBx, biomeBy), biomeTextColor, biomeName);
                }

                // ImGuizmo View Manipulator (Production-grade viewport compass)
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(screenPos.x, screenPos.y, viewportSize.x, viewportSize.y);

                Mat4 view = camera.viewMatrix();
                Mat4 originalView = view;
                
                // Orbit distance for the view manipulator
                float camDistance = 10.0f;
                
                // Draw the view manipulator in the top right corner
                ImGuizmo::ViewManipulate(view.m, camDistance, ImVec2(screenPos.x + viewportSize.x - 128, screenPos.y), ImVec2(128, 128), 0x10101010);

                // If the view matrix was modified by the manipulator, update the camera
                bool viewChanged = false;
                for (int i = 0; i < 16; ++i) {
                    if (std::abs(view.m[i] - originalView.m[i]) > 0.0001f) {
                        viewChanged = true;
                        break;
                    }
                }

                if (viewChanged) {
                    // Extract forward vector from view matrix
                    Vec3 f = {-view.m[2], -view.m[6], -view.m[10]};
                    
                    // Extract position from view matrix
                    float px = -(view.m[0] * view.m[12] + view.m[1] * view.m[13] + view.m[2] * view.m[14]);
                    float py = -(view.m[4] * view.m[12] + view.m[5] * view.m[13] + view.m[6] * view.m[14]);
                    float pz = -(view.m[8] * view.m[12] + view.m[9] * view.m[13] + view.m[10] * view.m[14]);
                    
                    // Calculate yaw and pitch
                    float yaw = std::atan2(f.z, f.x) * 180.0f / 3.14159265359f;
                    float pitch = std::asin(f.y) * 180.0f / 3.14159265359f;
                    
                    camera.setPosition({px, py, pz});
                    camera.setYawPitch(yaw, pitch);
                }

                // Draw Advancements
                float advY = screenPos.y + 20.0f;
                for (const auto& adv : advancements) {
                    if (adv.achieved) {
                        ImVec2 p = ImVec2(screenPos.x + viewportSize.x - 220.0f, advY);
                        drawList->AddRectFilled(p, ImVec2(p.x + 200, p.y + 40), IM_COL32(0, 150, 0, 180), 5.0f);
                        drawList->AddText(ImVec2(p.x + 10, p.y + 10), IM_COL32(255, 255, 255, 255), "Advancement Made!");
                        drawList->AddText(ImVec2(p.x + 10, p.y + 22), IM_COL32(255, 255, 0, 255), adv.title.c_str());
                        advY += 50.0f;
                    }
                }

                // Warm ambient lava glow at screen edges when near lava (heat shimmer)
                // Throttled: re-check at most every 0.2s instead of every frame
                {
                    static int  nLavaNearby       = 0;
                    static float lavaGlowCheckTimer = 0.0f;
                    lavaGlowCheckTimer += dt;
                    if (lavaGlowCheckTimer >= 0.2f) {
                        lavaGlowCheckTimer = 0.0f;
                        const Vec3 cpos3 = camera.position();
                        nLavaNearby = 0;
                        for (int dx = -3; dx <= 3 && nLavaNearby == 0; ++dx) {
                            for (int dz = -3; dz <= 3 && nLavaNearby == 0; ++dz) {
                                for (int dy = -1; dy <= 2 && nLavaNearby == 0; ++dy) {
                                    if (world.getBlock((int)cpos3.x + dx, (int)cpos3.y + dy, (int)cpos3.z + dz) == BLOCK_LAVA)
                                        nLavaNearby = 1;
                                }
                            }
                        }
                    }
                    if (nLavaNearby > 0 && playerOnFireSeconds <= 0.0f) {
                        // Warm orange vignette at screen edges
                        float t3 = (float)glfwGetTime();
                        float pulse = 0.5f + 0.5f * std::sin(t3 * 1.5f);
                        int edgeAlpha = (int)(18.0f + pulse * 14.0f);
                        float ew = viewportSize.x * 0.15f; // Edge width
                        float eh = viewportSize.y * 0.18f;
                        drawList->AddRectFilledMultiColor(
                            ImVec2(screenPos.x, screenPos.y),
                            ImVec2(screenPos.x + ew, screenPos.y + viewportSize.y),
                            IM_COL32(240, 100, 10, edgeAlpha), IM_COL32(240, 100, 10, 0),
                            IM_COL32(240, 100, 10, 0), IM_COL32(240, 100, 10, edgeAlpha));
                        drawList->AddRectFilledMultiColor(
                            ImVec2(screenPos.x + viewportSize.x - ew, screenPos.y),
                            ImVec2(screenPos.x + viewportSize.x, screenPos.y + viewportSize.y),
                            IM_COL32(240, 100, 10, 0), IM_COL32(240, 100, 10, edgeAlpha),
                            IM_COL32(240, 100, 10, edgeAlpha), IM_COL32(240, 100, 10, 0));
                        drawList->AddRectFilledMultiColor(
                            ImVec2(screenPos.x, screenPos.y + viewportSize.y - eh),
                            ImVec2(screenPos.x + viewportSize.x, screenPos.y + viewportSize.y),
                            IM_COL32(240, 100, 10, 0), IM_COL32(240, 100, 10, 0),
                            IM_COL32(240, 100, 10, edgeAlpha), IM_COL32(240, 100, 10, edgeAlpha));
                    }
                }

                // Player fire overlay (viewport only - dramatic flame bands)
                if (playerOnFireSeconds > 0.0f) {
                    float t = (float)glfwGetTime();
                    int bands = 18;
                    for (int i = 0; i < bands; ++i) {
                        float y0 = screenPos.y + (viewportSize.y / bands) * i;
                        float y1 = screenPos.y + (viewportSize.y / bands) * (i + 1);
                        float wobble  = std::sin(t * 3.5f + i * 1.7f) * 22.0f;
                        float wobble2 = std::sin(t * 5.0f + i * 2.4f) * 12.0f;
                        int r = 255;
                        int g = (int)(80 + 80 * std::sin(t * 2.0f + i * 0.5f));
                        int b = 15;
                        // More opaque at bottom (flames rise from below)
                        float heightFrac = (float)i / (float)(bands - 1);
                        int alpha = (int)(70.0f - heightFrac * 45.0f);
                        drawList->AddRectFilled(
                            ImVec2(screenPos.x + wobble + wobble2, y0),
                            ImVec2(screenPos.x + viewportSize.x + wobble + wobble2, y1),
                            IM_COL32(r, g, b, std::max(5, alpha)));
                    }
                }

                // Player damage flash overlay
                if (playerHurtTimer > 0.0f) {
                    float alpha = (playerHurtTimer / 0.35f) * 90.0f;
                    drawList->AddRectFilled(screenPos,
                        ImVec2(screenPos.x + viewportSize.x, screenPos.y + viewportSize.y),
                        IM_COL32(220, 30, 30, (int)alpha));
                }

                // "You Died!" overlay
                if (playerHp <= 0.0f) {
                    // Dark red full-screen overlay
                    float pulse = 0.55f + 0.45f * std::sin((float)glfwGetTime() * 2.0f);
                    drawList->AddRectFilled(screenPos,
                        ImVec2(screenPos.x + viewportSize.x, screenPos.y + viewportSize.y),
                        IM_COL32(120, 0, 0, (int)(pulse * 160.0f)));
                    // "You Died!" text
                    ImGui::PushFont(nullptr);
                    const char* diedText = "You Died!";
                    ImVec2 diedSize = ImGui::CalcTextSize(diedText);
                    float scale = 3.0f;
                    ImVec2 diedPos = ImVec2(
                        screenPos.x + (viewportSize.x - diedSize.x * scale) * 0.5f,
                        screenPos.y + viewportSize.y * 0.42f);
                    // Shadow
                    drawList->AddText(nullptr, ImGui::GetFontSize() * scale,
                        ImVec2(diedPos.x + 3, diedPos.y + 3),
                        IM_COL32(0, 0, 0, 200), diedText);
                    // Main text
                    drawList->AddText(nullptr, ImGui::GetFontSize() * scale,
                        diedPos, IM_COL32(255, 70, 70, 240), diedText);
                    // Respawn countdown
                    float remaining = std::max(0.0f, 3.0f - playerDeathTimer);
                    char countBuf[32];
                    snprintf(countBuf, sizeof(countBuf), "Respawning in %.1fs...", remaining);
                    ImVec2 countSize = ImGui::CalcTextSize(countBuf);
                    drawList->AddText(nullptr, ImGui::GetFontSize(),
                        ImVec2(screenPos.x + (viewportSize.x - countSize.x) * 0.5f,
                               screenPos.y + viewportSize.y * 0.56f),
                        IM_COL32(220, 180, 180, 200), countBuf);
                    ImGui::PopFont();
                }

                // Player HP bar (Minecraft-style hearts, above hotbar, left-aligned)
                {
                    float slotSize = 44.0f;
                    float hbPadding = 4.0f;
                    float hbWidth = (slotSize * 9.0f) + (hbPadding * 10.0f);
                    float hbHeight = slotSize + (hbPadding * 2.0f);
                    float hbX = screenPos.x + (viewportSize.x - hbWidth) * 0.5f;
                    float hbY = screenPos.y + viewportSize.y - hbHeight - 10.0f;
                    
                    float heartSize = 18.0f;
                    float spacing = 16.0f; // Tighter spacing for hearts
                    float startX = hbX;
                    float startY = hbY - heartSize - 6.0f; // Just above the hotbar

                    auto drawHeart = [&](ImVec2 pos, float size, ImU32 col, bool half, bool empty) {
                        ImVec2 bottom(pos.x + size * 0.5f, pos.y + size * 0.9f);
                        ImVec2 midDip(pos.x + size * 0.5f, pos.y + size * 0.3f);
                        ImVec2 leftTop(pos.x + size * 0.15f, pos.y + size * 0.1f);
                        ImVec2 leftEdge(pos.x + size * 0.05f, pos.y + size * 0.4f);
                        ImVec2 rightTop(pos.x + size * 0.85f, pos.y + size * 0.1f);
                        ImVec2 rightEdge(pos.x + size * 0.95f, pos.y + size * 0.4f);

                        ImVec2 leftHalf[4] = { bottom, leftEdge, leftTop, midDip };
                        ImVec2 rightHalf[4] = { bottom, midDip, rightTop, rightEdge };

                        ImU32 bgCol = IM_COL32(40, 40, 40, 200);
                        
                        // Draw background
                        drawList->AddConvexPolyFilled(leftHalf, 4, bgCol);
                        drawList->AddConvexPolyFilled(rightHalf, 4, bgCol);

                        if (!empty) {
                            drawList->AddConvexPolyFilled(leftHalf, 4, col);
                            if (!half) {
                                drawList->AddConvexPolyFilled(rightHalf, 4, col);
                            }
                        }

                        // Outline
                        ImVec2 outline[6] = { bottom, leftEdge, leftTop, midDip, rightTop, rightEdge };
                        drawList->AddPolyline(outline, 6, IM_COL32(0, 0, 0, 255), ImDrawFlags_Closed, 1.5f);
                    };

                    int totalHearts = 10;
                    int currentHp = (int)std::round(playerHp); // 0 to 20
                    
                    for (int i = 0; i < totalHearts; ++i) {
                        ImVec2 pos(startX + i * spacing, startY);
                        
                        // Add a little jump animation if HP is low (<= 4)
                        if (currentHp <= 4 && currentHp > 0) {
                            pos.y += sin(ImGui::GetTime() * 10.0f + i) * 2.0f;
                        }

                        int heartValue = (i + 1) * 2;
                        bool empty = currentHp < heartValue - 1;
                        bool half = currentHp == heartValue - 1;
                        
                        ImU32 heartCol = IM_COL32(220, 30, 30, 255); // Red
                        
                        drawHeart(pos, heartSize, heartCol, half, empty);
                    }
                }

                // Snowfall rendering (viewport only)
                if (!snowParticles.empty()) {
                    Mat4 viewProj = camera.projectionMatrix() * camera.viewMatrix();
                    for (auto& sp : snowParticles) {
                        Vec3 worldPos = camera.position() + sp.position;
                        Vec4 clipPos = viewProj * Vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
                        if (clipPos.w > 0.1f) {
                            float ndcX = clipPos.x / clipPos.w;
                            float ndcY = clipPos.y / clipPos.w;
                            if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
                                float sx = screenPos.x + (ndcX * 0.5f + 0.5f) * viewportSize.x;
                                float sy = screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * viewportSize.y;
                                float size = 3.0f / clipPos.w * 15.0f;
                                size = std::clamp(size, 1.5f, 6.0f);
                                drawList->AddCircleFilled(ImVec2(sx, sy), size, IM_COL32(255, 255, 255, 200));
                            }
                        }
                    }
                }

                // Lava particles: smoke (grey), embers (orange), fire wisps (yellow)
                if (!lavaParticles.empty()) {
                    Mat4 viewProj = camera.projectionMatrix() * camera.viewMatrix();
                    for (auto& lp : lavaParticles) {
                        Vec4 clipPos = viewProj * Vec4(lp.position.x, lp.position.y, lp.position.z, 1.0f);
                        if (clipPos.w > 0.1f) {
                            float ndcX = clipPos.x / clipPos.w;
                            float ndcY = clipPos.y / clipPos.w;
                            if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
                                float sx = screenPos.x + (ndcX * 0.5f + 0.5f) * viewportSize.x;
                                float sy = screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * viewportSize.y;
                                float lifeFrac = lp.life / lp.maxLife; // 1=fresh, 0=dying
                                float size = 2.0f / clipPos.w * 12.0f;

                                if (lp.type == 0) { // Smoke: grey, fades out as it rises
                                    size = std::clamp(size * (1.5f + (1.0f - lifeFrac) * 1.5f), 1.5f, 9.0f);
                                    int alpha = (int)(lifeFrac * 90.0f);
                                    int grey = 140 + (int)((1.0f - lifeFrac) * 40.0f);
                                    drawList->AddCircleFilled(ImVec2(sx, sy), size, IM_COL32(grey, grey, grey, alpha));
                                } else if (lp.type == 1) { // Ember: bright orange, fades quickly
                                    size = std::clamp(size * 0.5f, 1.0f, 3.5f);
                                    int alpha = (int)(lifeFrac * 230.0f);
                                    // Ember color shifts orange->red as it cools
                                    int g = (int)(180.0f * lifeFrac);
                                    drawList->AddCircleFilled(ImVec2(sx, sy), size, IM_COL32(255, g, 10, alpha));
                                    // Bright core
                                    if (size > 1.5f) drawList->AddCircleFilled(ImVec2(sx, sy), size * 0.4f, IM_COL32(255, 240, 100, alpha));
                                } else { // Fire wisp: bright yellow-white
                                    size = std::clamp(size * 0.7f, 1.0f, 4.5f);
                                    int alpha = (int)(lifeFrac * 180.0f);
                                    drawList->AddCircleFilled(ImVec2(sx, sy), size, IM_COL32(255, 200, 50, alpha));
                                    drawList->AddCircleFilled(ImVec2(sx, sy), size * 0.5f, IM_COL32(255, 255, 200, alpha));
                                }
                            }
                        }
                    }
                }

                // Block Outline (Minecraft style)
                if (raycastRes.hit) {
                    Mat4 viewProj = camera.projectionMatrix() * camera.viewMatrix();
                    Vec3 minP = { (float)raycastRes.x - 0.005f, (float)raycastRes.y - 0.005f, (float)raycastRes.z - 0.005f };
                    Vec3 maxP = { (float)raycastRes.x + 1.005f, (float)raycastRes.y + 1.005f, (float)raycastRes.z + 1.005f };
                    
                    Vec3 corners[8] = {
                        {minP.x, minP.y, minP.z}, {maxP.x, minP.y, minP.z}, {maxP.x, maxP.y, minP.z}, {minP.x, maxP.y, minP.z},
                        {minP.x, minP.y, maxP.z}, {maxP.x, minP.y, maxP.z}, {maxP.x, maxP.y, maxP.z}, {minP.x, maxP.y, maxP.z}
                    };
                    
                    ImVec2 screenCorners[8];
                    bool valid[8];
                    for (int i = 0; i < 8; ++i) {
                        Vec4 clipPos = viewProj * Vec4(corners[i].x, corners[i].y, corners[i].z, 1.0f);
                        valid[i] = clipPos.w > 0.1f;
                        if (valid[i]) {
                            float ndcX = clipPos.x / clipPos.w;
                            float ndcY = clipPos.y / clipPos.w;
                            screenCorners[i].x = screenPos.x + (ndcX * 0.5f + 0.5f) * viewportSize.x;
                            screenCorners[i].y = screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * viewportSize.y;
                        }
                    }
                    
                    auto drawEdge = [&](int i, int j) {
                        if (valid[i] && valid[j]) {
                            drawList->AddLine(screenCorners[i], screenCorners[j], IM_COL32(0, 0, 0, 150), 2.0f);
                        }
                    };
                    
                    // Bottom face
                    drawEdge(0, 1); drawEdge(1, 2); drawEdge(2, 3); drawEdge(3, 0);
                    // Top face
                    drawEdge(4, 5); drawEdge(5, 6); drawEdge(6, 7); drawEdge(7, 4);
                    // Vertical edges
                    drawEdge(0, 4); drawEdge(1, 5); drawEdge(2, 6); drawEdge(3, 7);
                }

                // Weather Effects (viewport only - from Weather Designer)
                const auto& weatherPreset = gui.getWeatherDesigner().getActivePreset();
                if (weatherPreset.particleCount > 0) {
                    float wt = (float)glfwGetTime();
                    float windDrift = weatherPreset.windStrength * std::cos(weatherPreset.windDirection * 3.14159f / 180.0f) * 3.0f;

                    // Inline fast hash: stable per-particle base positions (no rand())
                    auto pHash = [](unsigned int seed) -> unsigned int {
                        seed ^= seed >> 16; seed *= 0x45d9f3bu; seed ^= seed >> 16; return seed;
                    };

                    if (!weatherPreset.snowStyle) {
                        // Rain – stable per-particle X, smooth vertical fall
                        for (int i = 0; i < weatherPreset.particleCount; ++i) {
                            unsigned int h  = pHash((unsigned)i * 2531011u + 1u);
                            unsigned int h2 = pHash(h + 7u);
                            float rx   = (float)(h  & 0xFFFFu) / 65535.0f * viewportSize.x;
                            float baseY= (float)(h2 & 0xFFFFu) / 65535.0f * viewportSize.y;
                            float off  = std::fmod(baseY + wt * weatherPreset.particleSpeed, viewportSize.y);
                            float lineLen = std::clamp(weatherPreset.particleSpeed * 0.015f, 5.0f, 25.0f);
                            // Slight horizontal drift accumulates with fall progress
                            float xDrift = windDrift * (off / viewportSize.y) * 0.4f;
                            // Alpha fades near viewport edges for a softer fringe
                            float edgeFade = std::min(rx / 20.0f, (viewportSize.x - rx) / 20.0f);
                            edgeFade = std::clamp(edgeFade, 0.0f, 1.0f);
                            ImU32 rainCol = IM_COL32(
                                (int)(weatherPreset.particleColor.x * 255),
                                (int)(weatherPreset.particleColor.y * 255),
                                (int)(weatherPreset.particleColor.z * 255),
                                (int)(weatherPreset.particleAlpha * 255 * edgeFade));
                            drawList->AddLine(
                                ImVec2(screenPos.x + rx + xDrift, screenPos.y + off),
                                ImVec2(screenPos.x + rx + xDrift + windDrift * 0.15f, screenPos.y + off + lineLen),
                                rainCol, std::clamp(weatherPreset.particleSize * 0.5f, 0.5f, 2.0f));
                        }
                    } else {
                        // Snow – stable positions, natural oscillating drift, size variation
                        for (int i = 0; i < weatherPreset.particleCount; ++i) {
                            unsigned int h  = pHash((unsigned)i * 2531011u + 1u);
                            unsigned int h2 = pHash(h + 13u);
                            unsigned int h3 = pHash(h2 + 7u);
                            float rx    = (float)(h  & 0xFFFFu) / 65535.0f * viewportSize.x;
                            float baseY = (float)(h2 & 0xFFFFu) / 65535.0f * viewportSize.y;
                            float off   = std::fmod(baseY + wt * weatherPreset.particleSpeed, viewportSize.y);
                            // Per-flake oscillation phase gives unique sway
                            float phase = (float)(h3 & 0xFFu) / 255.0f * 6.28318f;
                            float drift = std::sin(wt * 0.65f + phase) * windDrift * 2.8f;
                            // Vary size: 60–130 % of base size
                            float sizeVar = 0.6f + (float)((h3 >> 8) & 0xFFu) / 255.0f * 0.7f;
                            float flakeR  = std::clamp(weatherPreset.particleSize * sizeVar, 1.0f, 5.5f);
                            // Gentle fade at top/bottom edges
                            float topFade = std::min(off / 30.0f, (viewportSize.y - off) / 30.0f);
                            topFade = std::clamp(topFade, 0.0f, 1.0f);
                            ImU32 flakeCol = IM_COL32(
                                (int)(weatherPreset.particleColor.x * 255),
                                (int)(weatherPreset.particleColor.y * 255),
                                (int)(weatherPreset.particleColor.z * 255),
                                (int)(weatherPreset.particleAlpha * 255 * topFade));
                            drawList->AddCircleFilled(
                                ImVec2(screenPos.x + rx + drift, screenPos.y + off),
                                flakeR, flakeCol);
                            // Larger flakes get a soft halo ring for depth
                            if (flakeR > 2.5f) {
                                ImU32 haloCol = IM_COL32(220, 235, 255,
                                    (int)(weatherPreset.particleAlpha * 60 * topFade));
                                drawList->AddCircle(
                                    ImVec2(screenPos.x + rx + drift, screenPos.y + off),
                                    flakeR * 1.7f, haloCol, 8, 0.6f);
                            }
                        }
                    }

                    // Fog overlay (viewport only)
                    if (weatherPreset.fogDensity > 0.01f) {
                        int fogAlpha = (int)(weatherPreset.fogDensity * 80.0f);
                        ImU32 fc = IM_COL32((int)(weatherPreset.fogColor.x * 255), (int)(weatherPreset.fogColor.y * 255), (int)(weatherPreset.fogColor.z * 255), fogAlpha);
                        drawList->AddRectFilled(screenPos, ImVec2(screenPos.x + viewportSize.x, screenPos.y + viewportSize.y), fc);
                    }
                }
            }
        }
        const bool viewportHovered = ImGui::IsWindowHovered();
        ImGui::End();
        ImGui::PopStyleVar();

        // Minimal Editor UI
        if (showWorldEditor)
        {
            ImGui::Begin("World Editor", &showWorldEditor);
            ImGui::Text("Mode: %s (F10 to toggle)", menuMode ? "MENU" : "WORLD");
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                static float fov = 70.0f;
                if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f)) {
                    camera.setFovDegrees(fov);
                }
                if (ImGui::Button("Reset Camera")) {
                    camera.setPosition({0.0f, (float)(worldBaseHeight + 20), 0.0f});
                }
            }

            if (ImGui::CollapsingHeader("World Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                int rd = world.getRenderDistance();
                if (ImGui::SliderInt("Render Distance", &rd, 2, 16)) {
                    world.setRenderDistance(rd);
                }
                ImGui::InputInt("Seed", &worldSeed);
                ImGui::SliderFloat("Frequency", &worldFrequency, 0.01f, 0.20f, "%.3f");
                ImGui::SliderInt("Base Height", &worldBaseHeight, 1, Chunk::SizeY - 2);
                ImGui::Text("Active Chunks: %zu", world.getChunkCount());
            }

            if (ImGui::CollapsingHeader("Time & Weather")) {
                ImGui::SliderFloat("Time", &worldTime, 0.0f, 24000.0f);
                const char* weatherNames[] = { "Clear", "Rain", "Snow" };
                int w = (int)currentWeather;
                if (ImGui::Combo("Weather", &w, weatherNames, 3)) {
                    currentWeather = (WeatherType)w;
                }
            }

            ImGui::Separator();
            if (ImGui::CollapsingHeader("Save / Load", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::InputText("World Name", worldSaveName, sizeof(worldSaveName));

                if (ImGui::Button("Save World")) {
                    // Sanitize name for filename
                    std::string safeName;
                    for (const char* p = worldSaveName; *p; ++p) {
                        char c = *p;
                        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ')
                            safeName += c;
                    }
                    if (safeName.empty()) safeName = "Unnamed";

                    WorldMetadata meta;
                    std::strncpy(meta.name, worldSaveName, sizeof(meta.name) - 1);
                    meta.name[sizeof(meta.name) - 1] = '\0';
                    meta.seed = worldSeed;
                    meta.frequency = worldFrequency;
                    meta.baseHeight = worldBaseHeight;
                    meta.worldTime = worldTime;

                    std::string path = "saves/" + safeName + ".vsa";
                    world.save(path, meta);
                    loadedWorldFile = path;
                }
                ImGui::SameLine();
                if (ImGui::Button("Load World...")) {
                    worldSaves = World::listSaves("saves");
                    showWorldList = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("New World")) {
                    world.clear();
                    fluidSim.clear();
                    loadedWorldFile.clear();
                    camera.setPosition({0.0f, (float)(worldBaseHeight + 20), 0.0f});
                    spawnPosition = {0.0f, (float)(worldBaseHeight + 20), 0.0f};
                    menuMode = false; // enter world mode immediately
                }

                if (!loadedWorldFile.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "File: %s", loadedWorldFile.c_str());
                }
            }

            // World list popup
            if (showWorldList) {
                ImGui::OpenPopup("World List");
            }
            if (ImGui::BeginPopupModal("World List", &showWorldList, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Saved Worlds:");
                ImGui::Separator();

                if (worldSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No saved worlds found.");
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Save a world first!");
                } else {
                    ImGui::BeginChild("WorldListScroll", ImVec2(400, 300), true);
                    for (size_t i = 0; i < worldSaves.size(); ++i) {
                        const auto& save = worldSaves[i];
                        ImGui::PushID((int)i);

                        bool selected = false;
                        if (ImGui::Selectable("##worldEntry", &selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 50))) {
                            WorldMetadata loadedMeta;
                            if (world.load(save.filename, loadedMeta)) {
                                fluidSim.clear();
                                std::strncpy(worldSaveName, loadedMeta.name, sizeof(worldSaveName) - 1);
                                worldSaveName[sizeof(worldSaveName) - 1] = '\0';
                                worldSeed = loadedMeta.seed;
                                worldFrequency = loadedMeta.frequency;
                                worldBaseHeight = loadedMeta.baseHeight;
                                worldTime = loadedMeta.worldTime;
                                loadedWorldFile = save.filename;

                                // Update noise generators with loaded seed
                                noise.SetSeed(worldSeed);
                                biomeNoise.SetSeed(worldSeed + 20);
                                biomeNoise.SetFrequency(worldFrequency * 0.04f);
                                continentalNoise.SetSeed(worldSeed + 10);
                                continentalNoise.SetFrequency(worldFrequency * 0.07f);
                                mountainNoise.SetSeed(worldSeed + 1);
                                mountainNoise.SetFrequency(worldFrequency * 0.55f);

                                camera.setPosition({0.0f, (float)(worldBaseHeight + 20), 0.0f});
                                spawnPosition = {0.0f, (float)(worldBaseHeight + 20), 0.0f}; // record spawn for respawn
                                menuMode = false; // enter world mode so player can move immediately
                            }
                            showWorldList = false;
                        }

                        // Draw world info over the selectable
                        ImVec2 pos = ImGui::GetItemRectMin();
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        char nameLabel[128];
                        snprintf(nameLabel, sizeof(nameLabel), "%s", save.displayName.c_str());
                        dl->AddText(ImVec2(pos.x + 8, pos.y + 4), IM_COL32(255, 255, 255, 255), nameLabel);

                        char infoLabel[128];
                        snprintf(infoLabel, sizeof(infoLabel), "Chunks: %zu  |  Seed: %d", save.chunkCount, save.metadata.seed);
                        dl->AddText(ImVec2(pos.x + 8, pos.y + 24), IM_COL32(180, 180, 180, 255), infoLabel);

                        ImGui::PopID();
                    }
                    ImGui::EndChild();
                }

                ImGui::Separator();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    showWorldList = false;
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();
            ImGui::Text("Block Palette:");
            auto& blocks = GameRegistry::getInstance().getAllBlocks();
            int count = 0;
            for (auto& [id, def] : blocks) {
                if (id == 0) continue;
                if (ImGui::RadioButton(def.name.c_str(), selectedBlock == id)) selectedBlock = id;
                if (++count % 4 != 0) ImGui::SameLine();
            }
            ImGui::NewLine();

            ImGui::Separator();
            ImGui::TextUnformatted("Controls:");
            ImGui::BulletText("F10: Toggle Menu/World input");
            ImGui::BulletText("E: Inventory");
            ImGui::BulletText("1-9: Select Block");
            ImGui::BulletText("WASD / Arrows: Move");
            ImGui::BulletText("Double W / Up: Sprint");
            ImGui::BulletText("LMB: Break Block, RMB: Place Block");

            ImGui::Separator();
            ImGui::TextUnformatted("Chat command codes:");
            ImGui::BulletText("/clear  /rain  /snow");
            ImGui::BulletText("/time set <0..24000>");
            ImGui::BulletText("/save  /load  /new");
            
            const Vec3 camPos = camera.position();
            ImGui::Text("Camera: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
            ImGui::End();
        }

        // Inventory window
        if (inventoryOpen) {
            ImVec2 mvCenter = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(mvCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Inventory", &inventoryOpen, ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Block Inventory");
            ImGui::Separator();

            ImTextureID texId = (ImTextureID)(intptr_t)atlas.getID();
            const float icon = 42.0f;
            const int cols = 8;
            int shown = 0;
            
            ImGui::BeginChild("InvScroll", ImVec2(0, 0), true);
            auto& allBlocks = GameRegistry::getInstance().getAllBlocks();
            for (int type = 1; type <= 255; ++type) {
                if (allBlocks.find(type) == allBlocks.end()) continue;
                const auto& def = allBlocks[type];
                int tx = def.texX;
                int ty = def.texY;
                const char* name = def.name.c_str();

                ImVec2 uv0 = ImVec2(tx / 16.0f, ty / 16.0f);
                ImVec2 uv1 = ImVec2((tx + 1) / 16.0f, (ty + 1) / 16.0f);
                
                bool selected = (selectedBlock == type);
                if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 0, 0.4f));
                else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.4f));

                char invBtnId[32];
                std::snprintf(invBtnId, sizeof(invBtnId), "##inv_%d", type);
                if (ImGui::ImageButton(invBtnId, texId, ImVec2(icon, icon), uv0, uv1)) {
                    selectedBlock = (uint8_t)type;
                }
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s (ID: %d)", name, type);
                    ImGui::Text("Count: %d", inventory.counts[type]);
                    ImGui::EndTooltip();
                }

                shown++;
                if (shown % cols != 0) ImGui::SameLine();
            }
            ImGui::EndChild();
            ImGui::End();
        }

        // Input + camera update
        {
            static float lastWPressTime = 0.0f;
            static bool isSprinting = false;
            static bool wWasDown = false;
            static float totalTime = 0.0f;
            totalTime += dt;

            static bool isFlying = false;
            static float lastSpacePressTime = 0.0f;
            static bool spaceWasDown = false;

            ImGuiIO& io = ImGui::GetIO();
            const bool uiCapturing = menuMode || chatOpen || inventoryOpen;
            const bool allowMouse = !uiCapturing && (viewportHovered || hadMouse);
            const bool allowKeyboard = !uiCapturing;

            glfwSetInputMode(renderer.getWindow(), GLFW_CURSOR, allowMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

            // Inventory toggle
            static bool eWasDown = false;
            bool eDown = (glfwGetKey(renderer.getWindow(), GLFW_KEY_E) == GLFW_PRESS);
            if (eDown && !eWasDown && !chatOpen) {
                inventoryOpen = !inventoryOpen;
            }
            eWasDown = eDown;

            if (allowKeyboard) {
                // Fly Toggle (Double Tap Space)
                bool spaceDown = (glfwGetKey(renderer.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS);
                if (spaceDown && !spaceWasDown) {
                    if (totalTime - lastSpacePressTime < 0.25f) {
                        isFlying = !isFlying;
                    }
                    lastSpacePressTime = totalTime;
                }
                spaceWasDown = spaceDown;
            }

            if (allowMouse) {
                double mx = 0.0, my = 0.0;
                glfwGetCursorPos(renderer.getWindow(), &mx, &my);
                if (!hadMouse) {
                    lastMouseX = mx;
                    lastMouseY = my;
                    hadMouse = true;
                }
                const float dx = (float)(mx - lastMouseX);
                const float dy = (float)(my - lastMouseY);
                lastMouseX = mx;
                lastMouseY = my;

                const float sensitivity = 0.10f;
                camera.addYawPitch(dx * sensitivity, -dy * sensitivity);

                // Block Interaction
                static bool lmbPressed = false;
                static bool rmbPressed = false;

                auto breakSecondsFor = [&](uint8_t type) -> float {
                    const auto& blockDef = GameRegistry::getInstance().getBlock(type);
                    float baseTime = blockDef.breakTime;
                    if (baseTime <= 0.0f || baseTime >= 1e8f) return std::numeric_limits<float>::infinity();

                    // Check tool requirements and apply speed multiplier
                    const ToolDefinition* tool = (equippedToolId > 0) ? GameRegistry::getInstance().getTool(equippedToolId) : nullptr;
                    if (tool && !blockDef.requiredToolType.empty() && tool->toolType == blockDef.requiredToolType) {
                        if (tool->tier >= blockDef.requiredToolTier) {
                            baseTime /= tool->speedMultiplier;
                        }
                    }
                    return baseTime;
                };

                auto canHarvest = [&](uint8_t type) -> bool {
                    const auto& blockDef = GameRegistry::getInstance().getBlock(type);
                    if (blockDef.requiredToolType.empty()) return true;
                    if (blockDef.requiredToolTier == 0) return true;
                    const ToolDefinition* tool = (equippedToolId > 0) ? GameRegistry::getInstance().getTool(equippedToolId) : nullptr;
                    if (!tool) return false;
                    return tool->toolType == blockDef.requiredToolType && tool->tier >= blockDef.requiredToolTier;
                };

                // Raycast every frame for interaction and outline
                auto res = raycastRes;

                if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    // Advance swing animation
                    toolSwingT = std::min(1.0f, toolSwingT + (float)dt * 7.0f);

                    // ---- Player Attack: Hit nearby mobs (Minecraft-like) ----
                    static float playerAttackCooldown = 0.0f;
                    if (playerAttackCooldown > 0.0f) playerAttackCooldown = std::max(0.0f, playerAttackCooldown - (float)dt);

                    // Attack fires at mid-swing when cooldown has expired
                    if (toolSwingT >= 0.45f && toolSwingT <= 0.55f && playerAttackCooldown <= 0.0f) {
                        const ToolDefinition* attackTool = (equippedToolId > 0) ? GameRegistry::getInstance().getTool(equippedToolId) : nullptr;
                        float playerDmg = attackTool ? attackTool->damage : 1.0f; // Bare fist = 1 damage
                        float playerKb  = attackTool ? attackTool->knockback : 0.2f;
                        float atkSpeed  = attackTool ? attackTool->attackSpeed : 1.6f;

                        // Fire Aspect: check specialEffect field (Iron/Diamond sword have "fire_aspect")
                        bool fireAspect = attackTool && attackTool->specialEffect == "fire_aspect";

                        auto mobPool2 = registry.getPool<Mob>();
                        float closestDist = 3.5f; // Max melee reach (Minecraft ~3.0)
                        int hitIdx = -1;
                        if (mobPool2) {
                            for (size_t i = 0; i < mobPool2->components.size(); ++i) {
                                Mob& candidate = mobPool2->components[i];
                                Transform* ct = registry.getComponent<Transform>(mobPool2->indexToEntity[i]);
                                if (!ct || candidate.hp <= 0.0f) continue;

                                Vec3 toMob = ct->position - camera.position();
                                float dist = length(toMob);
                                if (dist < closestDist) {
                                    // Check if mob is roughly in front of player (forward hemisphere)
                                    Vec3 fwd = camera.forward();
                                    Vec3 toNorm = normalize(toMob);
                                    float dot3 = fwd.x * toNorm.x + fwd.y * toNorm.y + fwd.z * toNorm.z;
                                    if (dot3 > 0.3f) { // Within ~72-degree cone
                                        closestDist = dist;
                                        hitIdx = (int)i;
                                    }
                                }
                            }
                        }

                        if (hitIdx >= 0 && mobPool2) {
                            Mob& hitMob = mobPool2->components[(size_t)hitIdx];
                            Transform* ht = registry.getComponent<Transform>(mobPool2->indexToEntity[(size_t)hitIdx]);
                            if (ht) {
                                hitMob.hp -= playerDmg;
                                hitMob.hurtFlashTimer = 0.35f;

                                // Knockback: push mob away from player
                                Vec3 awayDir = normalize(ht->position - camera.position());
                                hitMob.knockbackVel.x = awayDir.x * (playerKb + 4.0f);
                                hitMob.knockbackVel.y = 3.0f; // Slight upward bounce
                                hitMob.knockbackVel.z = awayDir.z * (playerKb + 4.0f);

                                // Fire Aspect: set mob on fire for 5 seconds
                                if (fireAspect) {
                                    hitMob.onFireSeconds = std::max(hitMob.onFireSeconds, 5.0f);
                                }

                                // Set hostile mobs to attack player when hit
                                if (hitMob.type == MOB_ZOMBIE || hitMob.type == MOB_SKELETON || hitMob.type == MOB_CREEPER) {
                                    hitMob.state = Mob::ATTACK;
                                    hitMob.stateTimer = 10.0f;
                                }

                                playerAttackCooldown = 1.0f / atkSpeed;
                            }
                        }
                    }

                    if (res.hit) {
                        uint8_t type = world.getBlock(res.x, res.y, res.z);
                        // Players cannot delete water, lava, or bedrock
                        if (type != BLOCK_WATER && type != BLOCK_LAVA && type != BLOCK_BEDROCK) {
                            if (!breaking || res.x != breakX || res.y != breakY || res.z != breakZ) {
                                breaking = true;
                                breakX = res.x; breakY = res.y; breakZ = res.z;
                                breakProgress = 0.0f;
                                breakType = type;
                            }

                            float secs = breakSecondsFor(type);
                            if (secs < std::numeric_limits<float>::infinity()) {
                                breakProgress += (float)dt / secs;
                                if (breakProgress >= 1.0f) {
                                    // Handle drops based on registry
                                    if (canHarvest(type)) {
                                        const auto& blockDef = GameRegistry::getInstance().getBlock(type);
                                        if (blockDef.dropsItself) {
                                            inventory.counts[type]++;
                                        } else {
                                            for (const auto& drop : blockDef.drops) {
                                                float roll = (float)(rand() % 1000) / 1000.0f;
                                                if (roll <= drop.chance) {
                                                    int count = drop.minCount;
                                                    if (drop.maxCount > drop.minCount)
                                                        count += rand() % (drop.maxCount - drop.minCount + 1);
                                                    inventory.counts[drop.blockId] += count;
                                                }
                                            }
                                        }
                                    }
                                    for (auto& adv : advancements) {
                                        if (!adv.achieved && adv.blockType == type && inventory.counts[type] >= adv.requiredCount) {
                                            adv.achieved = true;
                                            std::cout << "Advancement Made! [" << adv.title << "]" << std::endl;
                                        }
                                    }
                                    world.setBlock(res.x, res.y, res.z, 0);
                                    AudioManager::getInstance().playBlockBreakSound(type, { (float)res.x, (float)res.y, (float)res.z }, camera.position());

                                    // Activate any dormant generation fluids that now have a path to flow.
                                    // This is the "block update" event: fluid only wakes up when a
                                    // neighbour is disturbed, never spontaneously on generation.
                                    fluidSim.onBlockChanged(res.x, res.y, res.z);

                                    breaking = false;
                                    breakProgress = 0.0f;
                                    breakType = 0;
                                }
                            }
                        } else {
                            breaking = false;
                            breakProgress = 0.0f;
                            breakType = 0;
                        }
                    } else {
                        breaking = false;
                        breakProgress = 0.0f;
                        breakType = 0;
                    }
                    lmbPressed = true;
                } else {
                    lmbPressed = false;
                    // Decay swing animation back to resting
                    toolSwingT = std::max(0.0f, toolSwingT - (float)dt * 5.0f);
                }

                if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                    if (!rmbPressed) {
                        if (inventory.counts[selectedBlock] > 0) {
                            if (res.hit) {
                                world.setBlock(res.x + res.nx, res.y + res.ny, res.z + res.nz, selectedBlock);
                                if (selectedBlock == BLOCK_WATER || selectedBlock == BLOCK_LAVA) {
                                    fluidSim.onFluidPlaced(
                                        res.x + res.nx, res.y + res.ny, res.z + res.nz,
                                        selectedBlock == BLOCK_LAVA);
                                }
                                inventory.counts[selectedBlock]--;
                            }
                        }
                        rmbPressed = true;
                    }
                } else rmbPressed = false;

            } else {
                hadMouse = false;
            }

            if (allowKeyboard) {
                // Sprint Detection (Double Tap W or Up)
                bool wDown = (glfwGetKey(renderer.getWindow(), GLFW_KEY_W) == GLFW_PRESS) || 
                             (glfwGetKey(renderer.getWindow(), GLFW_KEY_UP) == GLFW_PRESS);
                
                if (wDown && !wWasDown) {
                    if (totalTime - lastWPressTime < 0.25f) {
                        isSprinting = true;
                    }
                    lastWPressTime = totalTime;
                }
                if (!wDown) isSprinting = false;
                wWasDown = wDown;

                const float baseSpeed = 7.2f; // Reduced by 40% from 12.0f
                float speedMultiplier = (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ? 2.5f : 1.0f;
                if (isSprinting) speedMultiplier *= 1.8f;
                if (isFlying) speedMultiplier *= 2.5f;

                // Fluid movement modifiers
                const Vec3 camPos = camera.position();
                const int px = (int)std::floor(camPos.x);
                const int pz = (int)std::floor(camPos.z);
                const int pyFeet = (int)std::floor(camPos.y - 1.4f);
                const int pyBody = (int)std::floor(camPos.y - 0.2f);
                uint8_t bFeet = world.getBlock(px, pyFeet, pz);
                uint8_t bBody = world.getBlock(px, pyBody, pz);
                bool inWater = isWater(bFeet) || isWater(bBody);
                bool inLava = isLava(bFeet) || isLava(bBody);

                // Detect water surface: feet in water but body in air
                bool onWaterSurface = false;
                if (!isFlying && inWater && !inLava) {
                    bool feetInWater = isWater(bFeet);
                    bool bodyInAir = !isWater(bBody);
                    if (feetInWater && bodyInAir) {
                        onWaterSurface = true;
                    }
                }

                if (inWater) speedMultiplier *= 0.65f;
                if (inLava) speedMultiplier *= 0.25f;
                // Lava sets player on fire (managed in environmental damage section below)

                const float speed = baseSpeed * speedMultiplier;

                float forward = 0.0f;
                float right = 0.0f;
                float up = 0.0f;

                if (glfwGetKey(renderer.getWindow(), GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(renderer.getWindow(), GLFW_KEY_UP) == GLFW_PRESS) forward += speed * dt;
                if (glfwGetKey(renderer.getWindow(), GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(renderer.getWindow(), GLFW_KEY_DOWN) == GLFW_PRESS) forward -= speed * dt;
                if (glfwGetKey(renderer.getWindow(), GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(renderer.getWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS) right += speed * dt;
                if (glfwGetKey(renderer.getWindow(), GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT) == GLFW_PRESS) right -= speed * dt;
                if (glfwGetKey(renderer.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) up += speed * dt;
                if (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) up -= speed * dt;

                // Water physics: surface floating, swimming, diving
                if (inWater && !isFlying) {
                    if (onWaterSurface) {
                        // At water surface: float in place, no oscillation
                        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
                            up = speed * dt * 0.8f; // Jump out / swim up
                        } else if (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                            up = -speed * dt * 0.5f; // Dive down
                        } else {
                            up = 0.0f; // Float in place
                        }
                    } else {
                        // Fully submerged: buoyancy
                        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
                            up = speed * dt * 0.8f; // Swim up
                        } else if (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                            up = speed * dt * 0.15f; // Gentle buoyancy
                        }
                    }
                }

                // Gravity if not flying and not in water
                static float verticalVelocity = 0.0f;
                if (!isFlying && !inWater) {
                    bool onGround = world.isSolid(px, (int)std::floor(camPos.y - 1.6f), pz);
                    if (!onGround) {
                        verticalVelocity -= 28.0f * dt;
                    } else {
                        if (verticalVelocity < 0) verticalVelocity = 0;
                        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
                            verticalVelocity = 10.0f;
                        }
                    }
                    up += verticalVelocity * dt;
                } else {
                    verticalVelocity = 0.0f;
                }

                // Collision Detection (Sliding + Step-Up)
                Vec3 oldPos = camera.position();
                auto checkColl = [&](const Vec3& p) {
                    float r = 0.3f;
                    for (float ox = -r; ox <= r; ox += r*2) {
                        for (float oz = -r; oz <= r; oz += r*2) {
                            for (float oy = -1.5f; oy <= 0.1f; oy += 0.8f) {
                                if (world.isSolid((int)std::floor(p.x + ox), (int)std::floor(p.y + oy), (int)std::floor(p.z + oz))) return true;
                            }
                        }
                    }
                    return false;
                };

                // Desired movement vectors
                float dX = camera.right().x * right + camera.forward().x * forward;
                float dZ = camera.right().z * right + camera.forward().z * forward;

                // 1. Try moving X (sliding independent of Z)
                Vec3 posWithX = oldPos;
                posWithX.x += dX;
                if (!checkColl(posWithX)) {
                    camera.setPosition(posWithX);
                } else {
                    // Step-up check for X
                    Vec3 stepUpX = posWithX;
                    stepUpX.y += 1.1f; 
                    if (!checkColl(stepUpX)) {
                        camera.setPosition(stepUpX); // Successfully stepped up
                    }
                }

                // 2. Try moving Z (sliding independent of X)
                Vec3 posWithZ = camera.position();
                posWithZ.z += dZ;
                if (!checkColl(posWithZ)) {
                    camera.setPosition(posWithZ);
                } else {
                    // Step-up check for Z
                    Vec3 stepUpZ = posWithZ;
                    stepUpZ.y += 1.1f;
                    if (!checkColl(stepUpZ)) {
                        camera.setPosition(stepUpZ); // Successfully stepped up
                    }
                }

                // 3. Try moving Y (Vertical)
                Vec3 posWithY = camera.position();
                posWithY.y += up;
                if (!checkColl(posWithY)) {
                    camera.setPosition(posWithY);
                } else {
                    if (up < 0) { // Hit ground
                        verticalVelocity = 0.0f;
                        // Snap to exact block height to prevent jitter
                        Vec3 snapped = camera.position();
                        snapped.y = std::ceil(snapped.y - 1.6f) + 1.6f;
                        camera.setPosition(snapped);
                    } else { // Hit ceiling
                        verticalVelocity = -2.0f; // Bonk head, start falling
                    }
                }

                // Hard floor: prevent player from falling below world boundary.
                // y=0 is bedrock; player eye height is 1.6 above feet, so
                // the minimum safe position is 1.65 (feet at y≥0.05, above bedrock).
                {
                    Vec3 p = camera.position();
                    if (p.y < 1.65f) {
                        p.y = 1.65f;
                        camera.setPosition(p);
                        if (verticalVelocity < 0.0f) verticalVelocity = 0.0f;
                    }
                }

                // Block Selection
                for (int i = 1; i <= 9; ++i) {
                    if (glfwGetKey(renderer.getWindow(), GLFW_KEY_0 + i) == GLFW_PRESS) {
                        selectedBlock = (uint8_t)i;
                    }
                }

                // Controller Support (Xbox)
                if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
                    int axesCount;
                    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);
                    if (axesCount >= 4) {
                        float lx = axes[0]; // Left Stick X
                        float ly = -axes[1]; // Left Stick Y
                        float rx = axes[2]; // Right Stick X
                        float ry = -axes[3]; // Right Stick Y
                        
                        if (std::abs(lx) > 0.1f) camera.moveLocal(0, lx * speed * dt, 0);
                        if (std::abs(ly) > 0.1f) camera.moveLocal(ly * speed * dt, 0, 0);
                        if (std::abs(rx) > 0.1f) camera.addYawPitch(rx * 2.0f, 0);
                        if (std::abs(ry) > 0.1f) camera.addYawPitch(0, ry * 2.0f);
                    }
                }
            }
        }

        // Player fire overlay (rendered inside viewport only below)

        // Snowfall effect: track biome state for viewport rendering
        bool isSnowingBiome = false;
        {
            Vec3 p = camera.position();
            float bn = biomeNoise.GetNoise(p.x, p.z);
            isSnowingBiome = (bn < 0.1f);

            if (isSnowingBiome) {
                if (snowParticles.size() < 800) {
                    for (int i = 0; i < 15; ++i) {
                        SnowParticle sp;
                        sp.position = {
                            (float)(rand() % 60 - 30),
                            (float)(rand() % 30 + 15),
                            (float)(rand() % 60 - 30)
                        };
                        sp.velocity = { (float)(rand() % 10 - 5) * 0.05f, -4.0f - (float)(rand() % 10) * 0.2f, (float)(rand() % 10 - 5) * 0.05f };
                        sp.life = 4.0f + (float)(rand() % 40) * 0.1f;
                        snowParticles.push_back(sp);
                    }
                }
            }

            // Update particle positions (but don't render yet)
            for (auto it = snowParticles.begin(); it != snowParticles.end(); ) {
                it->position.x += it->velocity.x * dt;
                it->position.y += it->velocity.y * dt;
                it->position.z += it->velocity.z * dt;
                it->life -= dt;
                if (it->position.y < -10.0f) it->position.y = 20.0f;
                if (it->life <= 0) {
                    it = snowParticles.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // ---- Lava Smoke / Ember / Fire Wisp Particles ----
        // Spawn particles above nearby lava and fire blocks
        {
            static float lavaParticleTimer = 0.0f;
            lavaParticleTimer += (float)dt;
            if (lavaParticleTimer > 0.08f && lavaParticles.size() < 500) {
                lavaParticleTimer = 0.0f;
                const Vec3 cpos = camera.position();
                const int cpx = (int)std::floor(cpos.x);
                const int cpz = (int)std::floor(cpos.z);
                // Sample random blocks near player for lava/fire
                for (int attempt = 0; attempt < 20; ++attempt) {
                    int rx = cpx + (rand() % 24 - 12);
                    int rz = cpz + (rand() % 24 - 12);
                    // Scan only a window around the player's Y to avoid scanning 126 levels
                    int scanYmin = std::max(1, (int)cpos.y - 20);
                    int scanYmax = std::min(Chunk::SizeY - 2, (int)cpos.y + 6);
                    for (int ry = scanYmin; ry < scanYmax; ++ry) {
                        uint8_t blk = world.getBlock(rx, ry, rz);
                        if (blk == BLOCK_LAVA || blk == BLOCK_FIRE) {
                            // Only spawn if above block is air
                            if (world.getBlock(rx, ry + 1, rz) == BLOCK_AIR) {
                                LavaParticle lp;
                                lp.position = {
                                    (float)rx + 0.5f + ((float)(rand() % 100) - 50.0f) * 0.008f,
                                    (float)ry + 1.0f + (float)(rand() % 100) * 0.005f,
                                    (float)rz + 0.5f + ((float)(rand() % 100) - 50.0f) * 0.008f
                                };
                                // Spawn type: 60% smoke, 30% ember, 10% fire wisp
                                int roll = rand() % 10;
                                lp.type = (roll < 6) ? 0 : (roll < 9) ? 1 : 2;

                                if (lp.type == 0) { // Smoke: rises slowly, drifts
                                    lp.velocity = {
                                        ((float)(rand() % 100) - 50.0f) * 0.005f,
                                        0.4f + (float)(rand() % 100) * 0.004f,
                                        ((float)(rand() % 100) - 50.0f) * 0.005f
                                    };
                                    lp.maxLife = 2.5f + (float)(rand() % 30) * 0.1f;
                                } else if (lp.type == 1) { // Ember: bounces up fast
                                    lp.velocity = {
                                        ((float)(rand() % 100) - 50.0f) * 0.02f,
                                        1.5f + (float)(rand() % 100) * 0.015f,
                                        ((float)(rand() % 100) - 50.0f) * 0.02f
                                    };
                                    lp.maxLife = 0.8f + (float)(rand() % 20) * 0.05f;
                                } else { // Fire wisp: dances upward
                                    lp.velocity = {
                                        ((float)(rand() % 100) - 50.0f) * 0.015f,
                                        0.8f + (float)(rand() % 100) * 0.01f,
                                        ((float)(rand() % 100) - 50.0f) * 0.015f
                                    };
                                    lp.maxLife = 1.2f + (float)(rand() % 20) * 0.06f;
                                }
                                lp.life = lp.maxLife;
                                lavaParticles.push_back(lp);
                                break; // One particle per sample attempt
                            }
                        }
                    }
                }
            }

            // Update existing lava particles
            for (auto it = lavaParticles.begin(); it != lavaParticles.end(); ) {
                it->position.x += it->velocity.x * (float)dt;
                it->position.y += it->velocity.y * (float)dt;
                it->position.z += it->velocity.z * (float)dt;
                // Slow down horizontal drift; embers fade velocity
                it->velocity.x *= (1.0f - (float)dt * 0.5f);
                it->velocity.z *= (1.0f - (float)dt * 0.5f);
                // Smoke/wisps slow rising over time; embers fall after
                if (it->type == 1) it->velocity.y -= (float)dt * 3.0f; // Ember falls
                it->life -= (float)dt;
                if (it->life <= 0.0f) {
                    it = lavaParticles.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Weather & Time Update
        worldTime += dt * 13.33f; // Gradual cycle: 24000 units / (30 mins * 60 secs) = ~13.33 units/sec
        if (worldTime > 24000) worldTime = 0;

        // Fluid Flow Simulation (Minecraft-like)
        // Water: fast (0.10s), Lava: slow-viscous (0.50s) - separate timers
        static float waterFluidTimer = 0.0f;
        static float lavaFluidTimer  = 0.0f;
        waterFluidTimer += (float)dt;
        lavaFluidTimer  += (float)dt;

        // Drain BlockUpdateEvents — transitions dormant generation fluids to ACTIVE
        // when disturbed by a neighbouring block change. Called every frame.
        fluidSim.processTick(world);

        auto tryFlow = [&](int x, int y, int z, uint8_t type) {
            if (y <= 1) return;
            int currentDist = 0;
            auto distIt = fluidDistances.find(fluidKey(x, y, z));
            if (distIt != fluidDistances.end()) currentDist = distIt->second;

            // Blocks a fluid can displace / flow into
            auto canReplace = [&](uint8_t blk) -> bool {
                return blk == BLOCK_AIR || blk == BLOCK_TALL_GRASS
                    || blk == BLOCK_FLOWER_RED || blk == BLOCK_FLOWER_BLUE;
            };

            // ----------------------------------------------------------------
            // 1. Flow DOWN — always the highest priority, resets spread dist
            // ----------------------------------------------------------------
            uint8_t below = world.getBlock(x, y - 1, z);
            if (canReplace(below)) {
                world.setBlock(x, y - 1, z, type);
                int64_t dk = fluidKey(x, y - 1, z);
                fluidDistances[dk] = 0;          // downward fall restarts the spread counter
                if (type == BLOCK_WATER) waterFluidQueue.push_back(dk);
                else                     lavaFluidQueue.push_back(dk);
                return;
            }
            if ((type == BLOCK_WATER && below == BLOCK_LAVA) ||
                (type == BLOCK_LAVA  && below == BLOCK_WATER)) {
                world.setBlock(x, y - 1, z, BLOCK_COBBLESTONE);
                return;
            }

            // ----------------------------------------------------------------
            // 2. Flow HORIZONTALLY
            // ----------------------------------------------------------------
            const int maxDist = (type == BLOCK_LAVA) ? 3 : 7;
            if (currentDist >= maxDist) return;

            const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

            if (type == BLOCK_LAVA) {
                // ---- Minecraft flow-cost algorithm ----
                // For each open direction, count how many steps it takes to reach
                // a position that has a downward drop (air below).  Lava prefers
                // to flow toward the closest drop — this makes it run off cliff
                // edges and down mountains naturally.
                const int kReach = 4;
                int cost[4] = {kReach + 1, kReach + 1, kReach + 1, kReach + 1};

                for (int i = 0; i < 4; ++i) {
                    for (int step = 1; step <= kReach; ++step) {
                        int cx = x + dirs[i][0] * step;
                        int cz = z + dirs[i][1] * step;
                        uint8_t blk = world.getBlock(cx, y, cz);
                        if (!canReplace(blk)) break;          // path blocked
                        // Is there a drop here?
                        if (canReplace(world.getBlock(cx, y - 1, cz))) {
                            cost[i] = step;
                            break;
                        }
                    }
                }

                // Pick the minimum cost among open directions
                int minCost = kReach + 2;
                for (int i = 0; i < 4; ++i) {
                    uint8_t nb = world.getBlock(x + dirs[i][0], y, z + dirs[i][1]);
                    if (canReplace(nb)) minCost = std::min(minCost, cost[i]);
                }

                // Spread to the first direction that matches the minimum cost.
                // One block per lava tick — true Minecraft viscosity.
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dirs[i][0];
                    int nz = z + dirs[i][1];
                    uint8_t neighbor = world.getBlock(nx, y, nz);
                    if (canReplace(neighbor) && cost[i] == minCost) {
                        world.setBlock(nx, y, nz, BLOCK_LAVA);
                        int64_t hk = fluidKey(nx, y, nz);
                        fluidDistances[hk] = currentDist + 1;
                        lavaFluidQueue.push_back(hk);
                        break;   // viscous: only one direction per tick
                    } else if (neighbor == BLOCK_WATER) {
                        world.setBlock(nx, y, nz, BLOCK_COBBLESTONE);
                        break;
                    }
                }

            } else {
                // ---- Water: spread to all open neighbours at once ----
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dirs[i][0];
                    int nz = z + dirs[i][1];
                    uint8_t neighbor = world.getBlock(nx, y, nz);
                    if (canReplace(neighbor)) {
                        world.setBlock(nx, y, nz, BLOCK_WATER);
                        int64_t hk = fluidKey(nx, y, nz);
                        fluidDistances[hk] = currentDist + 1;
                        waterFluidQueue.push_back(hk);
                    } else if (neighbor == BLOCK_LAVA) {
                        world.setBlock(nx, y, nz, BLOCK_COBBLESTONE);
                    }
                }
            }

            // ----------------------------------------------------------------
            // 3. Lava scorches adjacent flammable blocks
            // ----------------------------------------------------------------
            if (type == BLOCK_LAVA) {
                const int ldx[6] = {1,-1, 0, 0, 0, 0};
                const int ldy[6] = {0, 0, 1,-1, 0, 0};
                const int ldz[6] = {0, 0, 0, 0, 1,-1};
                for (int f = 0; f < 6; ++f) {
                    int ax = x + ldx[f], ay = y + ldy[f], az = z + ldz[f];
                    if (ay < 1 || ay >= Chunk::SizeY - 1) continue;
                    uint8_t adjBlock = world.getBlock(ax, ay, az);
                    if (GameRegistry::getInstance().getBlock(adjBlock).flammable) {
                        int fy = ay + 1;
                        if (fy < Chunk::SizeY && world.getBlock(ax, fy, az) == BLOCK_AIR) {
                            if ((rand() % 15) == 0)
                                world.setBlock(ax, fy, az, BLOCK_FIRE);
                        }
                    }
                }
            }
        };

        // Water update: every 0.10s
        if (waterFluidTimer > 0.10f) {
            waterFluidTimer = 0.0f;
            // --- Queue-based propagation (primary, reliable) ---
            // Process up to 400 queued water blocks per tick so new placements
            // propagate immediately without depending on random sampling.
            int qProcessed = 0;
            while (!waterFluidQueue.empty() && qProcessed < 400) {
                int64_t key = waterFluidQueue.front();
                waterFluidQueue.pop_front();
                int qx, qy, qz;
                decodeFluidKey(key, qx, qy, qz);
                // Double-guard: only flow if block is water AND not a dormant
                // generation-placed fluid. Handles edge cases where a dormant
                // block somehow ends up in the queue (e.g., chunk reload race).
                if (world.getBlock(qx, qy, qz) == BLOCK_WATER
                    && !fluidSim.isDormant(qx, qy, qz))
                    tryFlow(qx, qy, qz, BLOCK_WATER);
                ++qProcessed;
            }
            // Cap queue to avoid unbounded growth from large water bodies
            while (waterFluidQueue.size() > 3000) waterFluidQueue.pop_front();
            // --- Random sampling (secondary, keeps active streams ticking) ---
            // Dormant (generation-placed) fluids are excluded — they must be
            // activated explicitly via fluidSim.onBlockChanged().
            int px = (int)std::floor(camera.position().x / Chunk::SizeX);
            int pz = (int)std::floor(camera.position().z / Chunk::SizeZ);
            for (int i = 0; i < 4; ++i) {
                int rx = px + (rand() % 7 - 3);
                int rz = pz + (rand() % 7 - 3);
                for (int j = 0; j < 60; ++j) {
                    int vx = rx * Chunk::SizeX + (rand() % Chunk::SizeX);
                    int vz = rz * Chunk::SizeZ + (rand() % Chunk::SizeZ);
                    int vy = rand() % (Chunk::SizeY - 2) + 1;
                    if (world.getBlock(vx, vy, vz) == BLOCK_WATER
                        && !fluidSim.isDormant(vx, vy, vz))
                        tryFlow(vx, vy, vz, BLOCK_WATER);
                }
            }
        }

        // Lava update: every 1.25s — one block per tick, truly viscous (Minecraft overworld rate)
        if (lavaFluidTimer > 1.25f) {
            lavaFluidTimer = 0.0f;
            // Queue-based propagation (primary)
            int qProcessed = 0;
            while (!lavaFluidQueue.empty() && qProcessed < 200) {
                int64_t key = lavaFluidQueue.front();
                lavaFluidQueue.pop_front();
                int qx, qy, qz;
                decodeFluidKey(key, qx, qy, qz);
                if (world.getBlock(qx, qy, qz) == BLOCK_LAVA
                    && !fluidSim.isDormant(qx, qy, qz))
                    tryFlow(qx, qy, qz, BLOCK_LAVA);
                ++qProcessed;
            }
            while (lavaFluidQueue.size() > 1500) lavaFluidQueue.pop_front();
            // Random sampling (secondary)
            int px = (int)std::floor(camera.position().x / Chunk::SizeX);
            int pz = (int)std::floor(camera.position().z / Chunk::SizeZ);
            for (int i = 0; i < 3; ++i) {
                int rx = px + (rand() % 9 - 4);
                int rz = pz + (rand() % 9 - 4);
                for (int j = 0; j < 50; ++j) {
                    int vx = rx * Chunk::SizeX + (rand() % Chunk::SizeX);
                    int vz = rz * Chunk::SizeZ + (rand() % Chunk::SizeZ);
                    int vy = rand() % (Chunk::SizeY - 2) + 1;
                    if (world.getBlock(vx, vy, vz) == BLOCK_LAVA && !fluidSim.isDormant(vx, vy, vz))
                        tryFlow(vx, vy, vz, BLOCK_LAVA);
                }
            }
        }

        // Fire Propagation System (Minecraft-like: 3D spread, blocks burn down)
        static float fireSimTimer = 0.0f;
        static std::unordered_map<int64_t, float> burnTimers; // packed coord → remaining seconds
        static std::unordered_map<int64_t, float> blockBurnTimers; // flammable block burning-down timer
        fireSimTimer += (float)dt;
        if (fireSimTimer >= 0.4f) {
            fireSimTimer = 0.0f;

            auto fireKey = [](int x, int y, int z) -> int64_t {
                return ((int64_t)((x + 32768) & 0xFFFF) << 32)
                     | ((int64_t)(y       & 0xFF  ) << 16)
                     | ((int64_t)((z + 32768) & 0xFFFF));
            };
            auto decodeFireKey = [](int64_t k, int& ox, int& oy, int& oz) {
                oz = (int)(k & 0xFFFF)         - 32768;
                oy = (int)((k >> 16) & 0xFF);
                ox = (int)((k >> 32) & 0xFFFF) - 32768;
            };

            // 3D spread offsets: 6-face + upward biased (Minecraft spreads up easily)
            const int ndx[14] = {1,-1, 0, 0, 0, 0,  1,-1, 1,-1, 0, 0, 0, 0};
            const int ndy[14] = {0, 0, 1,-1, 0, 0,  1, 1, 0, 0, 2, 2, 1,-1};
            const int ndz[14] = {0, 0, 0, 0, 1,-1,  0, 0, 1,-1, 0, 0, 0, 0};

            auto isFlammable = [](uint8_t b) -> bool {
                return GameRegistry::getInstance().getBlock(b).flammable;
            };

            int fpx = (int)std::floor(camera.position().x);
            int fpz = (int)std::floor(camera.position().z);

            // 1. Lava ignites adjacent flammable blocks (3D scan, tightened range for performance)
            // Reduced from ±12/step2/full-Y to ±8/step2/Y<60 — ~9x fewer iterations
            for (int ix = fpx - 8; ix <= fpx + 8; ix += 2) {
                for (int iz = fpz - 8; iz <= fpz + 8; iz += 2) {
                    for (int iy = 1; iy < std::min(60, Chunk::SizeY - 2); iy += 2) {
                        if (world.getBlock(ix, iy, iz) != BLOCK_LAVA) continue;
                        for (int f = 0; f < 6; ++f) {
                            int ax = ix + ndx[f], ay = iy + ndy[f], az = iz + ndz[f];
                            if (ay < 1 || ay >= Chunk::SizeY - 1) continue;
                            uint8_t adjBlock = world.getBlock(ax, ay, az);
                            if (!isFlammable(adjBlock)) {
                                // Try to place fire directly ON lava-adjacent air
                                if (adjBlock == BLOCK_AIR && (rand() % 20) == 0) {
                                    world.setBlock(ax, ay, az, BLOCK_FIRE);
                                    burnTimers[fireKey(ax, ay, az)] = 3.0f + (float)(rand() % 6);
                                }
                                continue;
                            }
                            // Flammable block: set on fire (place fire on top or on the side)
                            int fy = ay + 1;
                            if (fy < Chunk::SizeY && world.getBlock(ax, fy, az) == BLOCK_AIR) {
                                if ((rand() % 12) == 0) { // ~8% chance per tick
                                    world.setBlock(ax, fy, az, BLOCK_FIRE);
                                    burnTimers[fireKey(ax, fy, az)] = 6.0f + (float)(rand() % 10);
                                    // Start burn timer on the flammable block itself
                                    auto bkey = fireKey(ax, ay, az);
                                    if (blockBurnTimers.find(bkey) == blockBurnTimers.end()) {
                                        float burnTime = (float)GameRegistry::getInstance().getBlock(adjBlock).burnTime;
                                        blockBurnTimers[bkey] = (burnTime > 0.0f ? burnTime * 0.1f : 15.0f);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 2. Tick burning blocks: gradual destruction over time
            {
                std::vector<int64_t> burnedOut;
                for (auto& [key, timer] : blockBurnTimers) {
                    int bx, by, bz;
                    decodeFireKey(key, bx, by, bz);
                    uint8_t blk = world.getBlock(bx, by, bz);
                    if (!isFlammable(blk)) { burnedOut.push_back(key); continue; }
                    timer -= 0.4f;
                    if (timer <= 0.0f) {
                        world.setBlock(bx, by, bz, BLOCK_AIR); // Block burned away
                        burnedOut.push_back(key);
                    }
                }
                for (auto k : burnedOut) blockBurnTimers.erase(k);
            }

            // 3. Tick existing fire blocks: spread to neighbors and burn out
            std::vector<int64_t> toRemove;
            // Deferred inserts: inserting into burnTimers while iterating it causes
            // iterator invalidation (undefined behavior / crash). Collect and apply after.
            std::vector<std::pair<int64_t, float>> newFireEntries;
            for (auto& [key, timer] : burnTimers) {
                int bx, by, bz;
                decodeFireKey(key, bx, by, bz);

                if (world.getBlock(bx, by, bz) != BLOCK_FIRE) {
                    toRemove.push_back(key);
                    continue;
                }

                // Check if fire has a solid/flammable base to survive on
                uint8_t belowFire = (by > 0) ? world.getBlock(bx, by - 1, bz) : 0;
                bool hasBase = isFlammable(belowFire) || (belowFire != BLOCK_AIR && belowFire != BLOCK_FIRE && belowFire != BLOCK_WATER);

                // Fire source timeout: if no base and no adjacent lava, cap at 3 seconds
                // (implements "fire disappears 3s after last source interaction")
                bool nearLava = false;
                for (int f = 0; f < 6 && !nearLava; ++f) {
                    if (world.getBlock(bx + ndx[f], by + ndy[f], bz + ndz[f]) == BLOCK_LAVA) nearLava = true;
                }
                if (!hasBase && !nearLava && timer > 3.0f) timer = 3.0f;

                // Fire without a valid base dies quicker
                if (!hasBase) timer -= 0.4f * 2.5f; else timer -= 0.4f;

                // Spread fire: Minecraft-like 3D spread with upward bias
                if ((rand() % 4) == 0) { // 25% chance per tick to attempt spread
                    for (int f = 0; f < 14; ++f) {
                        int ax = bx + ndx[f], ay = by + ndy[f], az = bz + ndz[f];
                        if (ax < -30000 || ax > 30000 || ay < 1 || ay >= Chunk::SizeY - 1) continue;

                        uint8_t adjBlock = world.getBlock(ax, ay, az);

                        // Spread fire: place it in air next to a flammable block
                        if (adjBlock == BLOCK_AIR) {
                            // Check if there's any flammable block touching this air cell
                            bool nearFlammable = false;
                            const int chdx[6] = {1,-1,0,0,0,0};
                            const int chdy[6] = {0,0,1,-1,0,0};
                            const int chdz[6] = {0,0,0,0,1,-1};
                            for (int c = 0; c < 6 && !nearFlammable; ++c) {
                                uint8_t nb = world.getBlock(ax+chdx[c], ay+chdy[c], az+chdz[c]);
                                if (isFlammable(nb)) nearFlammable = true;
                            }
                            if (nearFlammable && (rand() % 8) == 0) {
                                world.setBlock(ax, ay, az, BLOCK_FIRE);
                                newFireEntries.emplace_back(fireKey(ax, ay, az), 4.0f + (float)(rand() % 8));
                            }
                        }
                        // Set flammable block on fire (place fire on top if possible)
                        else if (isFlammable(adjBlock)) {
                            int fy = ay + 1;
                            if (fy < Chunk::SizeY && world.getBlock(ax, fy, az) == BLOCK_AIR && (rand() % 7) == 0) {
                                world.setBlock(ax, fy, az, BLOCK_FIRE);
                                newFireEntries.emplace_back(fireKey(ax, fy, az), 4.0f + (float)(rand() % 9));
                                // Start burning the block
                                int64_t bk = fireKey(ax, ay, az);
                                if (blockBurnTimers.find(bk) == blockBurnTimers.end()) {
                                    float burnTime = (float)GameRegistry::getInstance().getBlock(adjBlock).burnTime;
                                    blockBurnTimers[bk] = (burnTime > 0.0f ? burnTime * 0.1f : 15.0f);
                                }
                            }
                        }
                    }
                }

                // When fire burns out: remove it; if on flammable block, destroy block too
                if (timer <= 0.0f) {
                    world.setBlock(bx, by, bz, BLOCK_AIR);
                    if (by > 0 && isFlammable(belowFire)) {
                        // Destroy the underlying block and potentially spread
                        world.setBlock(bx, by - 1, bz, BLOCK_AIR);
                        blockBurnTimers.erase(fireKey(bx, by - 1, bz));
                    }
                    toRemove.push_back(key);
                }
            }
            for (auto k : toRemove) burnTimers.erase(k);
            // Apply deferred new fire entries (safe: no longer inside burnTimers iteration)
            for (auto& [nk, nv] : newFireEntries) {
                if (burnTimers.find(nk) == burnTimers.end())
                    burnTimers[nk] = nv;
            }
        }

        // ---- Player Environmental Damage (Lava, Fire) ----
        {
            const Vec3 camPos2 = camera.position();
            const int epx = (int)std::floor(camPos2.x);
            const int epz = (int)std::floor(camPos2.z);
            const int epyFeet = (int)std::floor(camPos2.y - 1.4f);
            const int epyBody = (int)std::floor(camPos2.y - 0.2f);
            uint8_t eFeet = world.getBlock(epx, epyFeet, epz);
            uint8_t eBody = world.getBlock(epx, epyBody, epz);
            bool epInLava  = (eFeet == BLOCK_LAVA  || eBody == BLOCK_LAVA);
            bool epInWater = (eFeet == BLOCK_WATER || eBody == BLOCK_WATER);
            bool epInFire  = (eFeet == BLOCK_FIRE  || eBody == BLOCK_FIRE);

            // Water extinguishes fire on player
            if (epInWater) playerOnFireSeconds = 0.0f;

            // Lava: direct contact damage (4 HP/sec, like Minecraft) + set on fire for 15s
            static float lavaPlayerDmgTimer = 0.0f;
            if (epInLava) {
                playerOnFireSeconds = 15.0f; // Lava sets you on fire for a long time
                lavaPlayerDmgTimer += (float)dt;
                if (lavaPlayerDmgTimer >= 0.5f) { // 4 HP/sec = 2 HP per 0.5s
                    lavaPlayerDmgTimer = 0.0f;
                    if (playerInvincTimer <= 0.0f) {
                        playerHp = std::max(0.0f, playerHp - 2.0f);
                        playerHurtTimer   = 0.35f;
                        playerInvincTimer = 0.5f;
                    }
                }
            } else {
                lavaPlayerDmgTimer = 0.0f;
            }

            // Fire block contact damage: 1 HP/sec
            static float firePlayerDmgTimer = 0.0f;
            if (epInFire && !epInLava) {
                playerOnFireSeconds = std::max(playerOnFireSeconds, 8.0f);
                firePlayerDmgTimer += (float)dt;
                if (firePlayerDmgTimer >= 1.0f) {
                    firePlayerDmgTimer = 0.0f;
                    if (playerInvincTimer <= 0.0f) {
                        playerHp = std::max(0.0f, playerHp - 1.0f);
                        playerHurtTimer   = 0.35f;
                        playerInvincTimer = 0.5f;
                    }
                }
            } else {
                firePlayerDmgTimer = 0.0f;
            }

            // Burning damage: 1 HP/sec while on fire (when not in lava/fire block)
            static float burnPlayerDmgTimer = 0.0f;
            if (playerOnFireSeconds > 0.0f && !epInLava && !epInFire) {
                playerOnFireSeconds = std::max(0.0f, playerOnFireSeconds - (float)dt);
                burnPlayerDmgTimer += (float)dt;
                if (burnPlayerDmgTimer >= 1.0f) {
                    burnPlayerDmgTimer = 0.0f;
                    if (playerInvincTimer <= 0.0f) {
                        playerHp = std::max(0.0f, playerHp - 1.0f);
                        playerHurtTimer   = 0.35f;
                        playerInvincTimer = 0.5f;
                    }
                }
            } else if (!epInFire && !epInLava) {
                burnPlayerDmgTimer = 0.0f;
            }
        }

        // ---- Player Death & Respawn ----
        if (playerHp <= 0.0f) {
            playerDeathTimer += dt;
            // Block all input/movement while dead; respawn after 3 seconds
            if (playerDeathTimer >= 3.0f) {
                playerHp            = 20.0f;
                playerOnFireSeconds = 0.0f;
                playerHurtTimer     = 0.0f;
                playerInvincTimer   = 0.0f;
                playerDeathTimer    = 0.0f;
                breaking            = false;
                breakProgress       = 0.0f;
                camera.setPosition(spawnPosition);
            }
        }

        // Update Mobs (ECS)
        if (playerHurtTimer > 0.0f)  playerHurtTimer  = std::max(0.0f, playerHurtTimer  - dt);
        if (playerInvincTimer > 0.0f) playerInvincTimer = std::max(0.0f, playerInvincTimer - dt);

        auto mobPool = registry.getPool<Mob>();
        if (mobPool) {
            // Mob-mob separation pass (keeps mobs from stacking)
            for (size_t i = 0; i < mobPool->components.size(); ++i) {
                Transform* ti = registry.getComponent<Transform>(mobPool->indexToEntity[i]);
                if (!ti) continue;
                for (size_t j = i + 1; j < mobPool->components.size(); ++j) {
                    Transform* tj = registry.getComponent<Transform>(mobPool->indexToEntity[j]);
                    if (!tj) continue;
                    Vec3 diff = ti->position - tj->position;
                    float d2 = diff.x*diff.x + diff.z*diff.z;
                    if (d2 < 1.0f && d2 > 0.0001f) {
                        float push = (1.0f - std::sqrt(d2)) * 0.5f * dt * 6.0f;
                        Vec3 n = normalize(Vec3{diff.x, 0.0f, diff.z});
                        ti->position.x += n.x * push;
                        ti->position.z += n.z * push;
                        tj->position.x -= n.x * push;
                        tj->position.z -= n.z * push;
                    }
                }
            }

            for (size_t i = 0; i < mobPool->components.size(); ++i) {
                Entity ent = mobPool->indexToEntity[i];
                Mob& mob = mobPool->components[i];
                Transform* transform = registry.getComponent<Transform>(ent);
                if (!transform || mob.hp <= 0.0f) continue;

                MobAI::update(mob, *transform, world, dt, camera.position());

                // Hostile mob melee damage to player
                if (mob.state == Mob::ATTACK && mob.attackCooldown <= 0.0f && playerInvincTimer <= 0.0f) {
                    float distSq = (transform->position.x - camera.position().x) * (transform->position.x - camera.position().x)
                                 + (transform->position.z - camera.position().z) * (transform->position.z - camera.position().z);
                    if (distSq < 2.5f * 2.5f) {
                        playerHp        = std::max(0.0f, playerHp - 2.0f);
                        playerHurtTimer = 0.35f;
                        playerInvincTimer = 0.5f;
                        mob.attackCooldown = 1.2f;
                    }
                }

                if (transform->position.y < 0.0f) transform->position.y = 100.0f;
            }
        }

        // Update Weather Designer
        gui.getWeatherDesigner().update(dt, worldTime);

        // Get weather preset for rendering
        const auto& weatherPreset = gui.getWeatherDesigner().getActivePreset();

        // Day/Night Cycle Simulation
        float dayProgress = worldTime / 24000.0f;
        float sunAngle = dayProgress * 2.0f * 3.14159f;
        float sunY = std::sin(sunAngle);
        Vec3 lightDir = normalize(Vec3{std::cos(sunAngle), sunY, -0.25f});

        // Sky color from Weather Designer preset
        Vec3 skyColorDay = weatherPreset.skyColorDay;
        Vec3 skyColorNight = weatherPreset.skyColorNight;
        Vec3 skyColor;
        if (sunY > 0) {
            skyColor = skyColorDay * std::max(0.1f, sunY) * weatherPreset.skyBrightness;
        } else {
            skyColor = skyColorNight * weatherPreset.skyBrightness;
        }
        // Apply fog tint
        if (weatherPreset.fogDensity > 0.01f) {
            float fogMix = weatherPreset.fogDensity * 0.5f;
            skyColor.x = skyColor.x * (1.0f - fogMix) + weatherPreset.fogColor.x * fogMix;
            skyColor.y = skyColor.y * (1.0f - fogMix) + weatherPreset.fogColor.y * fogMix;
            skyColor.z = skyColor.z * (1.0f - fogMix) + weatherPreset.fogColor.z * fogMix;
        }

        // Render world into framebuffer
        if (viewportSize.x > 0 && viewportSize.y > 0) {
            viewportBuffer.bind();
            renderer.clear(skyColor.x, skyColor.y, skyColor.z); 

            voxelShader.use();
            voxelShader.setMat4("uView", camera.viewMatrix());
            voxelShader.setMat4("uProjection", camera.projectionMatrix());
            voxelShader.setVec3("uLightDir", lightDir);
            voxelShader.setVec3("uViewPos", camera.position());
            voxelShader.setFloat("uTime", (float)glfwGetTime());
            voxelShader.setVec2("uResolution", Vec2{viewportSize.x, viewportSize.y});
            voxelShader.setVec3("uColorTint", Vec3{1.0f, 1.0f, 1.0f});
            
            atlas.bind(0);
            voxelShader.setInt("uTexture", 0);

            // Render World
            voxelShader.setMat4("uModel", Mat4::identity());
            world.render(voxelShader, camera.projectionMatrix() * camera.viewMatrix());

            // Render Mobs (ECS) - Improved Complex Mobs
            auto mobPool = registry.getPool<Mob>();
            if (mobPool) {
                for (size_t i = 0; i < mobPool->components.size(); ++i) {
                    Entity ent = mobPool->indexToEntity[i];
                    Mob& mob = mobPool->components[i];
                    Transform* transform = registry.getComponent<Transform>(ent);
                    if (!transform || mob.hp <= 0.0f) continue;

                    float yawRad = mob.yawDeg * 0.01745329252f;
                    float legAngle = std::sin(mob.animTime) * 0.6f;
                    if (!mob.isMoving && mob.type != MOB_BIRD && mob.type != MOB_FISH && mob.type != MOB_SALMON) legAngle = 0.0f;

                    // Idle breathing: subtle Y bob when standing still
                    float breathBob = 0.0f;
                    if (!mob.isMoving) {
                        breathBob = std::sin(mob.animTime * 0.8f + mob.idleAnimOffset) * 0.018f;
                    }

                    // Head look-at: compute extra Y-rotation for head parts toward player
                    float headLookYaw = 0.0f;
                    if (mob.state == Mob::FOLLOW || mob.state == Mob::ATTACK) {
                        Vec3 toPlayer = camera.position() - transform->position;
                        float targetYaw = std::atan2(toPlayer.x, toPlayer.z) * 57.2957795f;
                        float delta = targetYaw - mob.yawDeg;
                        while (delta > 180.0f) delta -= 360.0f;
                        while (delta < -180.0f) delta += 360.0f;
                        headLookYaw = std::clamp(delta, -50.0f, 50.0f) * 0.01745329252f;
                    }

                    // Tint: sheep color base, then hurt-flash lerp toward red
                    Vec3 tint = {1,1,1};
                    if (mob.type == MOB_SHEEP) tint = MobAI::getSheepColor(mob.sheepColor);
                    if (mob.hurtFlashTimer > 0.0f) {
                        float t = mob.hurtFlashTimer / 0.35f; // normalize 0..1
                        tint.x = tint.x * (1.0f - t) + 1.0f * t;
                        tint.y = tint.y * (1.0f - t) + 0.15f * t;
                        tint.z = tint.z * (1.0f - t) + 0.15f * t;
                    }
                    voxelShader.setVec3("uColorTint", tint);

                    Vec3 rootPos = transform->position;
                    rootPos.y += breathBob; // Apply idle breath bob to whole body
                    Mat4 root = translate(rootPos) * rotateY(yawRad);

                    auto& mobDef = GameRegistry::getInstance().getMob(mob.type);
                    int legPartIdx = 0; // tracks which leg-animated part this is (for alternation)
                    int partIdx    = 0; // global part counter for mesh cache key
                    for (const auto& part : mobDef.parts) {
                        float rotX = 0, rotY = 0, rotZ = 0;
                        if (part.affectedByLegAnim) {
                            // Alternate sign per leg index for natural gait
                            float sign = (legPartIdx % 2 == 0) ? 1.0f : -1.0f;
                            rotX = legAngle * sign;
                            ++legPartIdx;
                        }
                        if (part.affectedByHeadAnim) {
                            rotX += std::sin(mob.animTime * 0.4f) * 0.05f;
                            rotY += headLookYaw;
                        }

                        Mat4 p = root * translate(part.offset) * translate(part.pivot) * rotateX(rotX) * rotateY(rotY) * rotateZ(rotZ) * translate(-part.pivot) * scale(part.size);
                        voxelShader.setMat4("uModel", p);

                        // Cached per-part mesh: built once per (mob type, part index), reused every frame.
                        // Mesh vertices never change (UV/color baked in); only the uModel matrix changes.
                        int meshKey = (int)mob.type * 100 + partIdx;
                        auto cacheIt = mobMeshCache.find(meshKey);
                        if (cacheIt == mobMeshCache.end()) {
                            MeshBuilder partMb;
                            float ts = 1.0f / 16.0f;
                            auto addMobFace = [&](int fi, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
                                int tx = part.texX, ty = part.texY;
                                Vec3 fc = part.color;
                                if (part.usePerFace) {
                                    tx = part.faces[fi].texX;
                                    ty = part.faces[fi].texY;
                                    fc = part.faces[fi].color;
                                }
                                // Vertex color is the raw part color; mob tint applied via uColorTint
                                float u1 = tx * ts, v1 = ty * ts;
                                partMb.addFace(p1, p2, p3, p4, n, u1, v1, u1 + ts, v1 + ts, fc.x, fc.y, fc.z);
                            };
                            addMobFace(0, {1,0,0},{1,0,1},{1,1,1},{1,1,0}, {1,0,0});
                            addMobFace(1, {0,0,1},{0,0,0},{0,1,0},{0,1,1}, {-1,0,0});
                            addMobFace(2, {0,1,0},{1,1,0},{1,1,1},{0,1,1}, {0,1,0});
                            addMobFace(3, {0,0,1},{1,0,1},{1,0,0},{0,0,0}, {0,-1,0});
                            addMobFace(4, {0,0,1},{1,0,1},{1,1,1},{0,1,1}, {0,0,1});
                            addMobFace(5, {1,0,0},{0,0,0},{0,1,0},{1,1,0}, {0,0,-1});
                            GLMesh newMesh;
                            newMesh.upload(partMb.getVertices());
                            cacheIt = mobMeshCache.emplace(meshKey, std::move(newMesh)).first;
                        }
                        // uColorTint keeps the mob's tint (set before this loop)
                        cacheIt->second.draw();
                        ++partIdx;
                    }

                    voxelShader.setVec3("uColorTint", Vec3{1,1,1});
                }
            }

            // Sun / Moon
            {
                Vec3 sunDir = normalize(Vec3{std::cos(sunAngle), std::sin(sunAngle), -0.25f});
                Vec3 moonDir = sunDir * -1.0f;
                Vec3 sunPos = camera.position() + sunDir * 220.0f + Vec3{0.0f, 40.0f, 0.0f};
                Vec3 moonPos = camera.position() + moonDir * 220.0f + Vec3{0.0f, 40.0f, 0.0f};
                Mat4 face = rotateY(radians(camera.yawDegrees() + 90.0f)) * rotateX(radians(-camera.pitchDegrees()));

                voxelShader.setMat4("uModel", translate(sunPos) * face * scale({18.0f, 18.0f, 1.0f}));
                sunMesh.draw();
                voxelShader.setMat4("uModel", translate(moonPos) * face * scale({14.0f, 14.0f, 1.0f}));
                moonMesh.draw();
            }

            // Stars (only at night)
            if (sunY < 0.1f) {
                float starAlpha = std::clamp((0.1f - sunY) * 5.0f, 0.0f, 1.0f);
                Mat4 face = rotateY(radians(camera.yawDegrees() + 90.0f)) * rotateX(radians(-camera.pitchDegrees()));
                for (const auto& sd : precomputedStars) {
                    Vec3 sDir = {std::cos(sd.az) * std::cos(sd.el), std::sin(sd.el), std::sin(sd.az) * std::cos(sd.el)};
                    Vec3 sPos = camera.position() + sDir * 200.0f;
                    voxelShader.setMat4("uModel", translate(sPos) * face * scale({1.5f, 1.5f, 1.0f}));
                    starMesh.draw();
                }
            }

            // Clouds (Minecraft Style: Horizontal blocky layer)
            {
                float t = (float)glfwGetTime();
                float baseY = 140.0f; // Slightly higher
                float cloudSpeed = 1.2f;
                float offset = std::fmod(t * cloudSpeed, 256.0f);
                
                for (int cz = -9; cz <= 9; ++cz) {
                    for (int cx = -9; cx <= 9; ++cx) {
                        // Grid-based positioning around player (larger grid for more distance)
                        float gridScale = 96.0f;
                        float px = std::floor(camera.position().x / gridScale) * gridScale + (float)cx * gridScale + offset;
                        float pz = std::floor(camera.position().z / gridScale) * gridScale + (float)cz * gridScale;
                        
                        // Use a more complex noise for rarity and size
                        float n1 = std::sin(px * 0.005f) * std::cos(pz * 0.005f);
                        float n2 = std::sin(px * 0.015f + pz * 0.01f);
                        float noiseVal = (n1 + n2 * 0.5f);

                        if (noiseVal > 0.65f) { // Higher threshold = rarer
                            Vec3 p = {px, baseY + std::sin(px * 0.02f) * 2.0f, pz};
                            // Vary size based on noise
                            float sizeW = 64.0f + (noiseVal - 0.65f) * 120.0f;
                            float sizeH = 6.0f + (noiseVal - 0.65f) * 15.0f;
                            float sizeD = 64.0f + (noiseVal - 0.65f) * 120.0f;

                            voxelShader.setMat4("uModel", translate(p) * scale({sizeW, sizeH, sizeD}));
                            cloudMesh.draw();
                        }
                    }
                }
            }

            // Block break cracks overlay (visual)
            if (breaking) {
                int stage = std::clamp((int)(breakProgress * 10.0f), 0, 9);
                Mat4 model = translate({(float)breakX, (float)breakY, (float)breakZ}) * scale({1.02f, 1.02f, 1.02f});
                voxelShader.setMat4("uModel", model);
                crackMeshes[stage].draw();
            }

            viewportBuffer.unbind();
            viewportBuffer.resolve();
        }

        // Chat UI
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_T) == GLFW_PRESS && !chatOpen) {
            chatOpen = true;
            menuMode = true;
        }

        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS && !showEscMenu) {
            showEscMenu = true;
            menuMode = true;
        }

        // Ctrl+P — toggle Preferences window
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_P))
            showSettings = !showSettings;

        if (chatOpen) {
            // ... (keep chat UI)
        }

        // In-game pause / game menu (ESC)
        // Replaced BeginPopupModal with a regular dockable window so the
        // Preferences panel is never blocked by a modal dim-overlay.
        if (showEscMenu) {
            ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Appearing);
            ImGui::SetNextWindowPos(
                ImGui::GetMainViewport()->GetCenter(),
                ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGuiWindowFlags pauseFlags = ImGuiWindowFlags_NoCollapse
                                        | ImGuiWindowFlags_NoDocking
                                        | ImGuiWindowFlags_AlwaysAutoResize;
            if (ImGui::Begin("Game Menu", &showEscMenu, pauseFlags)) {
                // ---- Header ----
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.4f, 1.0f));
                ImGui::Text("  Voxel-Sim Architect");
                ImGui::PopStyleColor();
                ImGui::Separator();

                // ---- Audio Settings ----
                if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent(8.0f);
                    float master = AudioManager::getInstance().getMasterVolume();
                    if (ImGui::SliderFloat("Master Volume", &master, 0.0f, 1.0f))
                        AudioManager::getInstance().setMasterVolume(master);

                    float music = AudioManager::getInstance().getMusicVolume();
                    if (ImGui::SliderFloat("Music Volume", &music, 0.0f, 1.0f))
                        AudioManager::getInstance().setMusicVolume(music);

                    bool mobs = AudioManager::getInstance().isMobSoundsEnabled();
                    if (ImGui::Checkbox("Mob Sounds", &mobs))
                        AudioManager::getInstance().setMobSoundsEnabled(mobs);
                    ImGui::SameLine(160);
                    bool blks = AudioManager::getInstance().isBlockSoundsEnabled();
                    if (ImGui::Checkbox("Block Sounds", &blks))
                        AudioManager::getInstance().setBlockSoundsEnabled(blks);

                    bool musicOn = AudioManager::getInstance().isMusicEnabled();
                    if (ImGui::Checkbox("Background Music", &musicOn))
                        AudioManager::getInstance().setMusicEnabled(musicOn);
                    ImGui::Unindent(8.0f);
                }

                // ---- Display Settings ----
                if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent(8.0f);
                    if (ImGui::Checkbox("Fullscreen", &fullscreen))
                        renderer.setFullscreen(fullscreen);
                    ImGui::SameLine(160);
                    if (ImGui::Checkbox("VSync", &vsync))
                        renderer.setVSync(vsync);
                    if (ImGui::Checkbox("Wireframe", &wireframe))
                        renderer.setWireframe(wireframe);
                    ImGui::SameLine(160);
                    if (ImGui::Checkbox("Backface Culling", &backfaceCulling))
                        renderer.setBackfaceCulling(backfaceCulling);
                    ImGui::Unindent(8.0f);
                }

                ImGui::Separator();

                // ---- Actions ----
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.55f, 0.22f, 1.0f));
                if (ImGui::Button("Open Preferences (Ctrl+P)", ImVec2(-1, 0))) {
                    showSettings = true;
                    showEscMenu  = false;
                    menuMode     = false;
                }
                ImGui::PopStyleColor();
                ImGui::Spacing();
                if (ImGui::Button("Resume Game", ImVec2(-1, 0))) {
                    showEscMenu = false;
                    menuMode    = false;
                }
            }
            ImGui::End();
        }

        gui.endFrame();

        AudioManager::getInstance().update((float)dt);

        renderer.swapBuffers();
    }

    saveUIConfig(showProfiler, showMemory, showECS, showWorldEditor, showSettings, showSoundEditor, showBlockDesigner, showMobDesigner, showInteractionEditor, showToolDesigner, showSoundDesigner, showAdvWorldEditor, fullscreen);

    gui.shutdown();
    renderer.shutdown();
    AudioManager::getInstance().shutdown();
    return 0;
}
