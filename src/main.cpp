#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <thread>
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
#include "MobAI.hpp"
#include "Registry.hpp"

#include "FastNoiseLite.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>

enum WeatherType { WEATHER_CLEAR, WEATHER_RAIN, WEATHER_SNOW };

struct SnowParticle {
    Vec3 position;
    Vec3 velocity;
    float life;
};

struct ChatMessage {
    std::string text;
    ImVec4 color = ImVec4(1,1,1,1);
};

int main() {
    std::cout << "Voxel-Sim Architect Engine Starting..." << std::endl;
    GameRegistry::getInstance().init();

    // 1. Memory Management
    ArenaAllocator mainAllocator(1024 * 1024 * 100); // 100MB Arena
    
    // 2. Procedural Generation Setup
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(1337 + 20); // Same seed as worldSeed + 20
    biomeNoise.SetFrequency(0.02f * 0.05f); // Same frequency as worldFrequency * 0.05f
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(1337 + 10);
    continentalNoise.SetFrequency(0.02f * 0.08f);
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    
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

    if (!renderer.init(screenW, screenH, "Voxel-Sim Architect", true)) {
        return -1;
    }

    GUIManager gui;
    gui.init(renderer.getWindow());

    Framebuffer viewportBuffer;
    viewportBuffer.init(screenW, screenH);

    Texture atlas;
    atlas.generateAtlas();
    gui.setAtlasTextureID(atlas.getID());

    // Game State
    WeatherType currentWeather = WEATHER_CLEAR;
    std::vector<std::string> chatHistory;
    char chatInput[256] = "";
    bool chatOpen = false;
    bool inventoryOpen = false;
    // Registry registry; // Already initialized in main() section 6
    std::vector<SnowParticle> snowParticles;
    float worldTime = 6000.0f; // Start at noon
    bool isRaining = false;
    float playerOnFireSeconds = 0.0f;

    // UI State
    bool showProfiler = true;
    bool showMemory = true;
    bool showECS = true;
    bool showWorldEditor = true;
    bool showSettings = false;
    bool showSoundEditor = false;
    bool showBlockDesigner = false;
    bool showMobDesigner = false;
    bool showInteractionEditor = false;
    bool vsync = true;
    bool wireframe = false;
    bool backfaceCulling = false;
    bool fullscreen = true;
    bool menuMode = true; // Start in menu mode

    renderer.setVSync(vsync);
    renderer.setWireframe(wireframe);
    renderer.setBackfaceCulling(backfaceCulling);

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
    int worldBaseHeight = 10;
    int renderDistance = 4;
    uint8_t selectedBlock = 1; // 1=Dirt, 2=Grass, 3=Stone

    struct Inventory {
        int counts[256] = {0};
    } inventory;
    // Start with some blocks
    inventory.counts[BLOCK_DIRT] = 64;
    inventory.counts[BLOCK_GRASS] = 64;
    inventory.counts[BLOCK_STONE] = 64;

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
    camera.setPosition({0.0f, 30.0f, 0.0f});
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

    auto isWater = [&](uint8_t b) { return b == 4; };
    auto isLava = [&](uint8_t b) { return b == 5; };

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

    // Main Loop
    bool breaking = false;
    int breakX = 0, breakY = 0, breakZ = 0;
    float breakProgress = 0.0f;
    uint8_t breakType = 0;

    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!renderer.shouldClose()) {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaMs = (float)std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
        lastTime = currentTime;

        const float dt = deltaMs * 0.001f;

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

        // Spawn mobs in new chunks
        for (const auto& coord : world.getNewChunks()) {
            Chunk* c = world.getChunk(coord.first, coord.second);
            if (c) {
                MobAI::spawnMobsInChunk(registry, c, coord.first, coord.second, biomeNoise, continentalNoise);
            }
        }

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
                            if(b == BLOCK_WATER) nearWater = true;
                            if(b == BLOCK_LAVA) nearLava = true;
                            if(b == BLOCK_FIRE) nearFire = true;
                        }
                    }
                }
                auto& am = AudioManager::getInstance();
                am.setAmbientLoop("water", "assets/sounds/water_ambient.wav", nearWater, 0.45f);
                am.setAmbientLoop("lava", "assets/sounds/lava_ambient.wav", nearLava, 0.65f);
                am.setAmbientLoop("fire", "assets/sounds/fire_ambient.wav", nearFire, 0.55f);
                am.setAmbientLoop("rain", "assets/sounds/rain_ambient.wav", isRaining, 0.4f);
            }
            if (isRaining && (std::rand() % 2000 == 0)) {
                Vec3 p = camera.position();
                Vec3 tPos = {p.x + (std::rand()%160-80), p.y + 60, p.z + (std::rand()%160-80)};
                AudioManager::getInstance().playThunder(tPos, p);
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

        // Editor-style dockspace (production-feel).
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        
        gui.showMainMenuBar(showProfiler, showMemory, showECS, showWorldEditor, showSettings, showSoundEditor, showBlockDesigner, showMobDesigner, showInteractionEditor);

        if (showProfiler) gui.showProfiler(deltaMs);
        if (showMemory) {
            auto& chunkAlloc = Chunk::getAllocator();
            gui.showMemoryInspector(mainAllocator.getOffset(), mainAllocator.getSize(), chunkAlloc.getUsedCount(), chunkAlloc.getTotalCount());
        }
        if (showECS) gui.showECSEditor();
        gui.showSoundEditor(&showSoundEditor);
        gui.showBlockDesigner(&showBlockDesigner);
        gui.showMobDesigner(&showMobDesigner);
        gui.showInteractionEditor(&showInteractionEditor);
        if (showSettings) gui.showSettings(&showSettings, vsync, wireframe, fullscreen, backfaceCulling, renderer);

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

                // Draw Crosshair
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 center = ImVec2(screenPos.x + viewportSize.x * 0.5f, screenPos.y + viewportSize.y * 0.5f);
                float chSize = 10.0f;
                drawList->AddLine(ImVec2(center.x - chSize, center.y), ImVec2(center.x + chSize, center.y), IM_COL32(255, 255, 255, 200), 2.0f);
                drawList->AddLine(ImVec2(center.x, center.y - chSize), ImVec2(center.x, center.y + chSize), IM_COL32(255, 255, 255, 200), 2.0f);

                // Draw Hotbar
                ImTextureID texId = (ImTextureID)(uintptr_t)atlas.getID();
                float slotSize = 50.0f;
                float hbWidth = slotSize * 9.0f + 20.0f;
                float hbHeight = slotSize + 20.0f;
                ImVec2 hbPos = ImVec2(screenPos.x + (viewportSize.x - hbWidth) * 0.5f, screenPos.y + viewportSize.y - hbHeight - 20.0f);
                
                // Background
                drawList->AddRectFilled(hbPos, ImVec2(hbPos.x + hbWidth, hbPos.y + hbHeight), IM_COL32(20, 20, 20, 180), 5.0f);
                drawList->AddRect(hbPos, ImVec2(hbPos.x + hbWidth, hbPos.y + hbHeight), IM_COL32(200, 200, 200, 100), 5.0f, 0, 1.0f);
                
                for (int i = 1; i <= 9; ++i) {
                    ImVec2 slotPos = ImVec2(hbPos.x + 10.0f + (i-1) * slotSize, hbPos.y + 10.0f);
                    ImVec2 slotEnd = ImVec2(slotPos.x + slotSize - 4.0f, slotPos.y + slotSize - 4.0f);
                    
                    // Slot Background
                    drawList->AddRectFilled(slotPos, slotEnd, IM_COL32(40, 40, 40, 255), 2.0f);
                    
                    // Selection Highlight
                    if (selectedBlock == i) {
                        drawList->AddRect(slotPos, slotEnd, IM_COL32(255, 255, 255, 255), 2.0f, 0, 3.0f);
                    } else {
                        drawList->AddRect(slotPos, slotEnd, IM_COL32(80, 80, 80, 255), 2.0f, 0, 1.0f);
                    }

                    // Block Icon (Texture)
                    const auto& def = GameRegistry::getInstance().getBlock(i);
                    int tx = def.texX;
                    int ty = def.texY;
                    
                    ImVec2 uv0 = ImVec2(tx / 16.0f, ty / 16.0f);
                    ImVec2 uv1 = ImVec2((tx + 1) / 16.0f, (ty + 1) / 16.0f);
                    drawList->AddImage(texId, ImVec2(slotPos.x + 4, slotPos.y + 4), ImVec2(slotEnd.x - 4, slotEnd.y - 4), uv0, uv1);

                    // Count
                    char countBuf[16];
                    snprintf(countBuf, 16, "%d", inventory.counts[i]);
                    drawList->AddText(ImVec2(slotPos.x + 2, slotPos.y + slotSize - 18), IM_COL32(255, 255, 255, 255), countBuf);
                }

                // Debug Info Overlay
                ImGui::SetCursorScreenPos(ImVec2(screenPos.x + 10, screenPos.y + 10));
                ImGui::BeginGroup();
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Voxel-Sim Architect v0.8");
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Pos: %.1f, %.1f, %.1f", camera.position().x, camera.position().y, camera.position().z);
                ImGui::Text("Chunks: %zu", world.getChunkCount());
                ImGui::EndGroup();

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
                    camera.setPosition({0.0f, 30.0f, 0.0f});
                }
            }

            if (ImGui::CollapsingHeader("World Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                int rd = world.getRenderDistance();
                if (ImGui::SliderInt("Render Distance", &rd, 2, 16)) {
                    world.setRenderDistance(rd);
                }
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
            if (ImGui::Button("Save World")) world.save("world.dat");
            ImGui::SameLine();
            if (ImGui::Button("Load World")) world.load("world.dat");

            ImGui::SliderInt("Render Distance", &renderDistance, 1, 12);
            ImGui::InputInt("Seed", &worldSeed);
            ImGui::SliderFloat("Frequency", &worldFrequency, 0.01f, 0.20f, "%.3f");
            ImGui::SliderInt("Base Height", &worldBaseHeight, 1, Chunk::SizeY - 2);

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
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Inventory", &inventoryOpen, ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Block Inventory");
            ImGui::Separator();

            ImTextureID texId = (ImTextureID)(intptr_t)atlas.getID();
            const float icon = 42.0f;
            const int cols = 8;
            int shown = 0;
            
            ImGui::BeginChild("InvScroll", ImVec2(0, 0), true);
            for (int type = 1; type <= BLOCK_ICE; ++type) {
                const auto& def = GameRegistry::getInstance().getBlock(type);
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

                auto breakSecondsFor = [&](uint8_t type) {
                    switch (type) {
                        case BLOCK_STONE: return 1.0f;
                        case BLOCK_COAL_ORE:
                        case BLOCK_IRON_ORE:
                        case BLOCK_GOLD_ORE:
                        case BLOCK_DIAMOND_ORE:
                            return 1.2f;
                        case BLOCK_BEDROCK: return std::numeric_limits<float>::infinity();
                        case BLOCK_WOOD: return 0.9f;
                        default: return 0.45f;
                    }
                };

                if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    auto res = world.raycast(camera.position(), camera.forward(), 10.0f);
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
                                    inventory.counts[type]++;
                                    for (auto& adv : advancements) {
                                        if (!adv.achieved && adv.blockType == type && inventory.counts[type] >= adv.requiredCount) {
                                            adv.achieved = true;
                                            std::cout << "Advancement Made! [" << adv.title << "]" << std::endl;
                                        }
                                    }
                                    world.setBlock(res.x, res.y, res.z, 0);
                                    AudioManager::getInstance().playBlockBreakSound(type, { (float)res.x, (float)res.y, (float)res.z }, camera.position());
                                    
                                    // Trigger nearby fluid updates
                                    int dx[] = {1,-1,0,0,0,0}, dy[] = {0,0,1,-1,0,0}, dz[] = {0,0,0,0,1,-1};
                                    for(int i=0; i<6; ++i) {
                                        uint8_t nb = world.getBlock(res.x+dx[i], res.y+dy[i], res.z+dz[i]);
                                        if (nb == BLOCK_WATER || nb == BLOCK_LAVA) {
                                            // Immediate flow into the new hole
                                            world.setBlock(res.x, res.y, res.z, nb);
                                            break; 
                                        }
                                    }

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
                } else lmbPressed = false;

                if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                    if (!rmbPressed) {
                        if (inventory.counts[selectedBlock] > 0) {
                            auto res = world.raycast(camera.position(), camera.forward(), 10.0f);
                            if (res.hit) {
                                world.setBlock(res.x + res.nx, res.y + res.ny, res.z + res.nz, selectedBlock);
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
                if (inWater) speedMultiplier *= 0.65f;
                if (inLava) speedMultiplier *= 0.25f;
                if (inLava) playerOnFireSeconds = 2.0f;
                
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

                // Swim feel: slight buoyancy in water unless diving
                if (inWater && !isFlying) {
                    if (glfwGetKey(renderer.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
                        up = speed * dt * 0.8f; // Swim up
                    } else if (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                        up = speed * dt * 0.15f; // Buoyancy
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

                // 1. Try moving X
                Vec3 posWithX = oldPos;
                posWithX.x += camera.right().x * right + camera.forward().x * forward;
                if (!checkColl(posWithX)) {
                    camera.setPosition(posWithX);
                } else {
                    // Step-up check for X
                    Vec3 stepUpX = posWithX;
                    stepUpX.y += 1.1f; 
                    if (!checkColl(stepUpX)) {
                        camera.setPosition(stepUpX);
                    }
                }

                // 2. Try moving Z
                Vec3 posWithZ = camera.position();
                posWithZ.z += camera.right().z * right + camera.forward().z * forward;
                if (!checkColl(posWithZ)) {
                    camera.setPosition(posWithZ);
                } else {
                    // Step-up check for Z
                    Vec3 stepUpZ = posWithZ;
                    stepUpZ.y += 1.1f;
                    if (!checkColl(stepUpZ)) {
                        camera.setPosition(stepUpZ);
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
                    } else { // Hit ceiling
                        verticalVelocity = -2.0f;
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

        // Player fire overlay
        if (playerOnFireSeconds > 0.0f) {
            playerOnFireSeconds = std::max(0.0f, playerOnFireSeconds - (float)dt);
            ImDrawList* draw = ImGui::GetForegroundDrawList();
            ImVec2 s = ImGui::GetIO().DisplaySize;
            float t = (float)glfwGetTime();
            int bands = 14;
            for (int i = 0; i < bands; ++i) {
                float y0 = (s.y / bands) * i;
                float y1 = (s.y / bands) * (i + 1);
                float wobble = sin(t * 3.0f + i * 1.7f) * 18.0f;
                ImU32 col = IM_COL32(255, (int)(120 + 60 * sin(t + i)), 40, 55);
                draw->AddRectFilled(ImVec2(0 + wobble, y0), ImVec2(s.x + wobble, y1), col);
            }
        }

        // Snowfall effect in Polar/Snowy biomes
        {
            Vec3 p = camera.position();
            float bn = biomeNoise.GetNoise(p.x, p.z);
            bool isSnowingBiome = (bn < 0.1f); // Polar or Snowy
            
            if (isSnowingBiome) {
                // Spawn new particles
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

            ImDrawList* draw = ImGui::GetForegroundDrawList();
            ImVec2 s = ImGui::GetIO().DisplaySize;
            Mat4 viewProj = camera.projectionMatrix() * camera.viewMatrix();

            for (auto it = snowParticles.begin(); it != snowParticles.end(); ) {
                it->position.x += it->velocity.x * dt;
                it->position.y += it->velocity.y * dt;
                it->position.z += it->velocity.z * dt;
                it->life -= dt;

                // Wrap around player
                if (it->position.y < -10.0f) it->position.y = 20.0f;
                
                // Project to screen
                Vec3 worldPos = camera.position() + it->position;
                Vec4 clipPos = viewProj * Vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
                
                if (clipPos.w > 0.1f && it->life > 0) {
                    float ndcX = clipPos.x / clipPos.w;
                    float ndcY = clipPos.y / clipPos.w;
                    if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
                        float screenX = (ndcX * 0.5f + 0.5f) * s.x;
                        float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * s.y;
                        float size = 3.0f / clipPos.w * 15.0f;
                        size = std::clamp(size, 1.5f, 6.0f);
                        draw->AddCircleFilled(ImVec2(screenX, screenY), size, IM_COL32(255, 255, 255, 200));
                    }
                    ++it;
                } else if (it->life <= 0) {
                    it = snowParticles.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Weather & Time Update
        worldTime += dt * 13.33f; // Gradual cycle: 24000 units / (30 mins * 60 secs) = ~13.33 units/sec
        if (worldTime > 24000) worldTime = 0;

        // Fluid Flow Simulation (Minecraft-like)
        static float fluidTimer = 0.0f;
        fluidTimer += (float)dt;
        if (fluidTimer > 0.15f) { // Faster updates
            fluidTimer = 0.0f;
            int px = (int)std::floor(camera.position().x / Chunk::SizeX);
            int pz = (int)std::floor(camera.position().z / Chunk::SizeZ);
            
            auto tryFlow = [&](int x, int y, int z, uint8_t type) {
                if (y <= 1) return;
                // 1. Flow Down
                if (world.getBlock(x, y - 1, z) == 0) {
                    world.setBlock(x, y - 1, z, type);
                    return;
                }
                // 2. Flow Horizontally
                int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
                for (int i = 0; i < 4; ++i) {
                    if (world.getBlock(x + dirs[i][0], y, z + dirs[i][1]) == 0) {
                        // Only spread if there's a solid block or fluid below (don't float in air)
                        uint8_t below = world.getBlock(x + dirs[i][0], y - 1, z + dirs[i][1]);
                        if (below != 0 || y < 5) { // y < 5 for sea/lava floor
                            world.setBlock(x + dirs[i][0], y, z + dirs[i][1], type);
                        }
                    }
                }
            };

            for (int i = 0; i < 6; ++i) {
                int rx = px + (rand() % 7 - 3);
                int rz = pz + (rand() % 7 - 3);
                for (int j = 0; j < 150; ++j) {
                    int vx = rx * Chunk::SizeX + (rand() % Chunk::SizeX);
                    int vz = rz * Chunk::SizeZ + (rand() % Chunk::SizeZ);
                    int vy = rand() % (Chunk::SizeY - 2) + 1;
                    uint8_t b = world.getBlock(vx, vy, vz);
                    if (b == BLOCK_WATER || b == BLOCK_LAVA) {
                        tryFlow(vx, vy, vz, b);
                    }
                }
            }
        }

        // Update Mobs (ECS)
        auto mobPool = registry.getPool<Mob>();
        if (mobPool) {
            for (size_t i = 0; i < mobPool->components.size(); ++i) {
                Entity ent = mobPool->indexToEntity[i];
                Mob& mob = mobPool->components[i];
                Transform* transform = registry.getComponent<Transform>(ent);
                if (!transform || mob.hp <= 0.0f) continue;

                MobAI::update(mob, *transform, world, dt, camera.position());

                if (transform->position.y < 0.0f) transform->position.y = 100.0f;
            }
        }

        // Day/Night Cycle Simulation
        float dayProgress = worldTime / 24000.0f;
        float sunAngle = dayProgress * 2.0f * 3.14159f;
        float sunY = std::sin(sunAngle);
        Vec3 lightDir = normalize(Vec3{std::cos(sunAngle), sunY, -0.25f});
        Vec3 skyColor = Vec3{0.4f, 0.6f, 0.9f} * std::max(0.1f, sunY);
        if (sunY < 0) skyColor = Vec3{0.05f, 0.05f, 0.1f}; // Night sky

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

                    Mat4 root = translate(transform->position) * rotateY(yawRad);
                    
                    Vec3 tint = {1,1,1};
                    if (mob.type == MOB_SHEEP) tint = MobAI::getSheepColor(mob.sheepColor);
                    voxelShader.setVec3("uColorTint", tint);

                    auto drawPart = [&](Vec3 offset, Vec3 size, Vec3 pivot, Vec3 rot) {
                        Mat4 p = root * translate(offset) * rotateX(rot.x) * rotateY(rot.y) * rotateZ(rot.z) * translate(-pivot) * scale(size);
                        voxelShader.setMat4("uModel", p);
                        mobMeshes[(int)mob.type].draw();
                    };

                    switch(mob.type) {
                        case MOB_COW: {
                            // Body
                            drawPart({-0.45f, 0.4f, -0.6f}, {0.9f, 0.8f, 1.3f}, {0,0,0}, {0,0,0});
                            // Head
                            float hb = std::sin(mob.animTime * 0.4f) * 0.04f;
                            drawPart({-0.25f, 0.85f+hb, 0.45f}, {0.5f, 0.5f, 0.5f}, {0,0,0}, {0,0,0});
                            // Horns
                            drawPart({-0.35f, 1.3f+hb, 0.55f}, {0.1f, 0.2f, 0.1f}, {0,0,0}, {0,0,0});
                            drawPart({ 0.25f, 1.3f+hb, 0.55f}, {0.1f, 0.2f, 0.1f}, {0,0,0}, {0,0,0});
                            // Legs
                            drawPart({-0.4f, 0.0f, 0.4f}, {0.3f, 0.4f, 0.3f}, {0.15f, 0.4f, 0.15f}, {legAngle,0,0});
                            drawPart({ 0.1f, 0.0f, 0.4f}, {0.3f, 0.4f, 0.3f}, {0.15f, 0.4f, 0.15f}, {-legAngle,0,0});
                            drawPart({-0.4f, 0.0f, -0.5f}, {0.3f, 0.4f, 0.3f}, {0.15f, 0.4f, 0.15f}, {-legAngle,0,0});
                            drawPart({ 0.1f, 0.0f, -0.5f}, {0.3f, 0.4f, 0.3f}, {0.15f, 0.4f, 0.15f}, {legAngle,0,0});
                            break;
                        }
                        case MOB_PIG: {
                            // Body
                            drawPart({-0.4f, 0.35f, -0.5f}, {0.8f, 0.7f, 1.1f}, {0,0,0}, {0,0,0});
                            // Head
                            drawPart({-0.25f, 0.65f, 0.4f}, {0.5f, 0.5f, 0.4f}, {0,0,0}, {0,0,0});
                            // Snout
                            drawPart({-0.15f, 0.75f, 0.8f}, {0.3f, 0.2f, 0.1f}, {0,0,0}, {0,0,0});
                            // Legs
                            drawPart({-0.35f, 0.0f, 0.35f}, {0.25f, 0.35f, 0.25f}, {0.125f, 0.35f, 0.125f}, {legAngle,0,0});
                            drawPart({ 0.10f, 0.0f, 0.35f}, {0.25f, 0.35f, 0.25f}, {0.125f, 0.35f, 0.125f}, {-legAngle,0,0});
                            drawPart({-0.35f, 0.0f, -0.45f}, {0.25f, 0.35f, 0.25f}, {0.125f, 0.35f, 0.125f}, {-legAngle,0,0});
                            drawPart({ 0.10f, 0.0f, -0.45f}, {0.25f, 0.35f, 0.25f}, {0.125f, 0.35f, 0.125f}, {legAngle,0,0});
                            break;
                        }
                        case MOB_SHEEP: {
                            // Body (Wool)
                            drawPart({-0.45f, 0.45f, -0.6f}, {0.9f, 0.8f, 1.2f}, {0,0,0}, {0,0,0});
                            // Head (Smaller wool part)
                            drawPart({-0.25f, 0.85f, 0.4f}, {0.5f, 0.5f, 0.4f}, {0,0,0}, {0,0,0});
                            // Legs (No tint - skin color)
                            voxelShader.setVec3("uColorTint", {1.0f, 1.0f, 1.0f});
                            drawPart({-0.35f, 0.0f, 0.35f}, {0.25f, 0.5f, 0.25f}, {0,0,0}, {legAngle,0,0});
                            drawPart({ 0.10f, 0.0f, 0.35f}, {0.25f, 0.5f, 0.25f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({-0.35f, 0.0f, -0.55f}, {0.25f, 0.5f, 0.25f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({ 0.10f, 0.0f, -0.55f}, {0.25f, 0.5f, 0.25f}, {0,0,0}, {legAngle,0,0});
                            break;
                        }
                        case MOB_CHICKEN: {
                            float flap = std::sin(mob.animTime * 2.0f) * 0.7f;
                            // Body
                            drawPart({-0.2f, 0.3f, -0.2f}, {0.4f, 0.4f, 0.5f}, {0,0,0}, {0,0,0});
                            // Head
                            drawPart({-0.15f, 0.7f, 0.1f}, {0.3f, 0.3f, 0.25f}, {0,0,0}, {0,0,0});
                            // Beak
                            voxelShader.setVec3("uColorTint", {1.0f, 0.5f, 0.0f});
                            drawPart({-0.1f, 0.8f, 0.35f}, {0.2f, 0.1f, 0.2f}, {0,0,0}, {0,0,0});
                            // Legs
                            voxelShader.setVec3("uColorTint", {1.0f, 1.0f, 1.0f});
                            drawPart({-0.15f, 0.0f, 0.0f}, {0.1f, 0.35f, 0.1f}, {0,0,0}, {legAngle,0,0});
                            drawPart({ 0.05f, 0.0f, 0.0f}, {0.1f, 0.35f, 0.1f}, {0,0,0}, {-legAngle,0,0});
                            // Wings
                            drawPart({-0.35f, 0.45f, -0.1f}, {0.15f, 0.3f, 0.45f}, {0.15f, 0.3f, 0.25f}, {0,0,flap});
                            drawPart({ 0.2f, 0.45f, -0.1f}, {0.15f, 0.3f, 0.45f}, {0, 0.3f, 0.25f}, {0,0,-flap});
                            break;
                        }
                        case MOB_RABBIT: {
                             // Body
                            drawPart({-0.2f, 0.2f, -0.25f}, {0.4f, 0.35f, 0.5f}, {0,0,0}, {0,0,0});
                            // Head
                            drawPart({-0.15f, 0.45f, 0.1f}, {0.3f, 0.3f, 0.3f}, {0,0,0}, {0,0,0});
                            // Ears
                            drawPart({-0.12f, 0.75f, 0.15f}, {0.08f, 0.3f, 0.1f}, {0,0,0}, {0,0,0});
                            drawPart({ 0.04f, 0.75f, 0.15f}, {0.08f, 0.3f, 0.1f}, {0,0,0}, {0,0,0});
                            // Legs
                            drawPart({-0.18f, 0.0f, 0.15f}, {0.12f, 0.2f, 0.12f}, {0,0,0}, {legAngle,0,0});
                            drawPart({ 0.06f, 0.0f, 0.15f}, {0.12f, 0.2f, 0.12f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({-0.18f, 0.0f, -0.2f}, {0.12f, 0.2f, 0.12f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({ 0.06f, 0.0f, -0.2f}, {0.12f, 0.2f, 0.12f}, {0,0,0}, {legAngle,0,0});
                            break;
                        }
                        case MOB_BIRD: {
                            float flap = std::sin(mob.animTime * 3.0f) * 1.0f;
                            // Body
                            drawPart({-0.15f, 0.0f, -0.2f}, {0.3f, 0.25f, 0.4f}, {0,0,0}, {0,0,0});
                            // Head
                            drawPart({-0.1f, 0.25f, 0.15f}, {0.2f, 0.2f, 0.2f}, {0,0,0}, {0,0,0});
                            // Beak
                            voxelShader.setVec3("uColorTint", {1.0f, 1.0f, 0.0f});
                            drawPart({-0.05f, 0.35f, 0.35f}, {0.1f, 0.05f, 0.15f}, {0,0,0}, {0,0,0});
                            // Wings
                            voxelShader.setVec3("uColorTint", tint);
                            drawPart({-0.45f, 0.1f, -0.15f}, {0.35f, 0.1f, 0.35f}, {0.35f,0,0.2f}, {0,0,-flap});
                            drawPart({ 0.1f, 0.1f, -0.15f}, {0.35f, 0.1f, 0.35f}, {0,0,0.2f}, {0,0,flap});
                            break;
                        }
                        case MOB_FISH:
                        case MOB_SALMON: {
                            float wag = std::sin(mob.animTime * 1.5f) * 0.4f;
                            if (mob.type == MOB_SALMON) voxelShader.setVec3("uColorTint", {0.8f, 0.4f, 0.4f});
                            // Body
                            drawPart({-0.15f, 0.05f, -0.3f}, {0.3f, 0.4f, 0.7f}, {0,0,0}, {0,wag,0});
                            // Tail
                            drawPart({-0.05f, 0.1f, -0.65f}, {0.1f, 0.3f, 0.45f}, {0.05f, 0, 0.45f}, {0,wag*1.5f,0});
                            // Dorsal Fin
                            drawPart({-0.02f, 0.45f, -0.2f}, {0.04f, 0.2f, 0.3f}, {0,0,0}, {0,wag,0});
                            break;
                        }
                        case MOB_OCTOPUS: {
                            // Head
                            drawPart({-0.35f, 0.35f, -0.35f}, {0.7f, 0.8f, 0.7f}, {0,0,0}, {0,0,0});
                            // Eyes
                            voxelShader.setVec3("uColorTint", {1,1,1});
                            drawPart({-0.2f, 0.65f, 0.3f}, {0.15f, 0.2f, 0.1f}, {0,0,0}, {0,0,0});
                            drawPart({ 0.05f, 0.65f, 0.3f}, {0.15f, 0.2f, 0.1f}, {0,0,0}, {0,0,0});
                            // Tentacles (8)
                            voxelShader.setVec3("uColorTint", tint);
                            for(int j=0; j<8; ++j) {
                                float ang = (float)j * (6.28f / 8.0f);
                                float w = std::sin(mob.animTime + (float)j) * 0.4f;
                                drawPart({std::cos(ang)*0.25f, 0.0f, std::sin(ang)*0.25f}, {0.15f, 0.5f, 0.15f}, {0.075f, 0.5f, 0.075f}, {w, 0, w});
                            }
                            break;
                        }
                        case MOB_DOG: {
                            // Body
                            drawPart({-0.25f, 0.3f, -0.5f}, {0.5f, 0.5f, 1.0f}, {0,0,0}, {0,0,0});
                            // Head
                            float r = std::sin(mob.animTime * 0.2f) * 0.1f;
                            drawPart({-0.2f, 0.6f, 0.4f}, {0.4f, 0.4f, 0.4f}, {0.2f, 0, 0}, {0, r, 0});
                            // Snout
                            drawPart({-0.12f, 0.65f, 0.75f}, {0.25f, 0.2f, 0.25f}, {0,0,0}, {0, r, 0});
                            // Ears
                            drawPart({-0.25f, 0.95f, 0.45f}, {0.15f, 0.15f, 0.1f}, {0,0,0}, {0, r, 0});
                            drawPart({ 0.1f, 0.95f, 0.45f}, {0.15f, 0.15f, 0.1f}, {0,0,0}, {0, r, 0});
                            // Legs
                            drawPart({-0.2f, 0.0f, 0.35f}, {0.2f, 0.35f, 0.2f}, {0,0,0}, {legAngle,0,0});
                            drawPart({ 0.0f, 0.0f, 0.35f}, {0.2f, 0.35f, 0.2f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({-0.2f, 0.0f, -0.45f}, {0.2f, 0.35f, 0.2f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({ 0.0f, 0.0f, -0.45f}, {0.2f, 0.35f, 0.2f}, {0,0,0}, {legAngle,0,0});
                            // Tail
                            float twist = std::sin(mob.animTime * 2.0f) * 0.5f;
                            drawPart({-0.05f, 0.65f, -0.55f}, {0.1f, 0.1f, 0.5f}, {0.05f,0,0.5f}, {0, twist, 0});
                            break;
                        }
                        case MOB_CAT: {
                            // Body
                            drawPart({-0.15f, 0.25f, -0.4f}, {0.3f, 0.35f, 0.8f}, {0,0,0}, {0,0,0});
                            // Head
                            drawPart({-0.12f, 0.55f, 0.3f}, {0.25f, 0.25f, 0.25f}, {0,0,0}, {0,0,0});
                            // Ears
                            drawPart({-0.14f, 0.75f, 0.35f}, {0.1f, 0.15f, 0.05f}, {0,0,0}, {0,0,0});
                            drawPart({ 0.04f, 0.75f, 0.35f}, {0.1f, 0.15f, 0.05f}, {0,0,0}, {0,0,0});
                            // Legs
                            drawPart({-0.12f, 0.0f, 0.25f}, {0.12f, 0.3f, 0.12f}, {0,0,0}, {legAngle,0,0});
                            drawPart({ 0.02f, 0.0f, 0.25f}, {0.12f, 0.3f, 0.12f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({-0.12f, 0.0f, -0.3f}, {0.12f, 0.3f, 0.12f}, {0,0,0}, {-legAngle,0,0});
                            drawPart({ 0.02f, 0.0f, -0.3f}, {0.12f, 0.3f, 0.12f}, {0,0,0}, {legAngle,0,0});
                            break;
                        }
                        default: {
                            Vec3 half = MobAI::getHalfExtents(mob.type);
                            Mat4 model = translate(transform->position) * rotateY(yawRad) * scale({half.x * 2.0f, half.y * 2.0f, half.z * 2.0f}) * translate({-0.5f, 0, -0.5f});
                            voxelShader.setMat4("uModel", model);
                            mobMeshes[(int)mob.type].draw();
                        }
                    }
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
                srand(42); // Fixed seed for consistent stars
                for (int i = 0; i < 150; ++i) {
                    float az = (float)(rand() % 360) * 0.01745f;
                    float el = (float)(rand() % 180) * 0.01745f;
                    Vec3 sDir = {std::cos(az) * std::cos(el), std::sin(el), std::sin(az) * std::cos(el)};
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
                
                for (int cz = -12; cz <= 12; ++cz) {
                    for (int cx = -12; cx <= 12; ++cx) {
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

            // Weather Effects (Rain/Snow)
            if (currentWeather != WEATHER_CLEAR) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                float t = (float)glfwGetTime();
                
                if (currentWeather == WEATHER_RAIN) {
                    // Minecraft-style rain: vertical blue lines
                    for (int i = 0; i < 250; ++i) {
                        float rx = (float)(rand() % (int)viewportSize.x);
                        float ry = (float)(rand() % (int)viewportSize.y);
                        float speed = 800.0f;
                        float offset = std::fmod(ry + t * speed, viewportSize.y);
                        drawList->AddLine(
                            ImVec2(screenPos.x + rx, screenPos.y + offset), 
                            ImVec2(screenPos.x + rx, screenPos.y + offset + 15), 
                            IM_COL32(80, 120, 255, 180), 1.5f
                        );
                    }
                } else {
                    // Snow: drifting white circles
                    for (int i = 0; i < 150; ++i) {
                        float rx = (float)(rand() % (int)viewportSize.x);
                        float ry = (float)(rand() % (int)viewportSize.y);
                        float speed = 150.0f;
                        float offset = std::fmod(ry + t * speed, viewportSize.y);
                        float drift = std::sin(t + i) * 20.0f;
                        drawList->AddCircleFilled(
                            ImVec2(screenPos.x + rx + drift, screenPos.y + offset), 
                            2.5f, IM_COL32(255, 255, 255, 220)
                        );
                    }
                }
            }
            
            viewportBuffer.unbind();
        }

        // Chat UI
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_T) == GLFW_PRESS && !chatOpen) {
            chatOpen = true;
            menuMode = true;
        }

        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS && !showSettings) {
            showSettings = true;
            menuMode = true;
        }

        if (chatOpen) {
            // ... (keep chat UI)
        }

        // Settings Menu
        if (showSettings) {
            ImGui::OpenPopup("Settings Menu");
        }

        if (ImGui::BeginPopupModal("Settings Menu", &showSettings, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Audio Settings");
            ImGui::Separator();
            
            float master = AudioManager::getInstance().getMasterVolume();
            if (ImGui::SliderFloat("Master Volume", &master, 0.0f, 1.0f)) {
                AudioManager::getInstance().setMasterVolume(master);
            }

            float music = AudioManager::getInstance().getMusicVolume();
            if (ImGui::SliderFloat("Music Volume", &music, 0.0f, 1.0f)) {
                AudioManager::getInstance().setMusicVolume(music);
            }

            ImGui::Separator();
            bool mobs = AudioManager::getInstance().isMobSoundsEnabled();
            if (ImGui::Checkbox("Mob Sounds", &mobs)) {
                AudioManager::getInstance().setMobSoundsEnabled(mobs);
            }

            bool blocks = AudioManager::getInstance().isBlockSoundsEnabled();
            if (ImGui::Checkbox("Block Sounds", &blocks)) {
                AudioManager::getInstance().setBlockSoundsEnabled(blocks);
            }

            bool musicOn = AudioManager::getInstance().isMusicEnabled();
            if (ImGui::Checkbox("Background Music", &musicOn)) {
                AudioManager::getInstance().setMusicEnabled(musicOn);
            }

            ImGui::Separator();
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                showSettings = false;
                menuMode = false;
            }
            ImGui::EndPopup();
        }

        gui.endFrame();

        AudioManager::getInstance().update((float)dt);

        renderer.swapBuffers();
    }

    gui.shutdown();
    renderer.shutdown();
    AudioManager::getInstance().shutdown();
    return 0;
}
