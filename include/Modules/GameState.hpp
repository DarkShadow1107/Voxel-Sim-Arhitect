#pragma once
#include <vector>
#include <string>
#include "Modules/Components.hpp"
#include "Modules/PlayerSystem.hpp"
#include "World.hpp"
#include "ECS.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"
#include "Framebuffer.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "TaskScheduler.hpp"
#include "FastNoiseLite.h"

struct GameState {
    // Systems & Resources
    World world;
    Registry registry;
    Camera camera;
    PlayerSystem::PlayerState playerState;
    TaskScheduler scheduler;
    FastNoiseLite noise;
    
    // World Settings
    int worldSeed = 1337;
    float worldFrequency = 0.02f;
    int worldBaseHeight = 10;
    int renderDistance = 4;
    float worldTime = 6000.0f;
    
    // Weather & Environment
    WeatherType currentWeather = WEATHER_CLEAR;
    std::vector<SnowParticle> snowParticles;
    
    // UI & Chat
    std::vector<std::string> chatHistory;
    char chatInput[256] = "";
    bool chatOpen = false;
    bool menuMode = true;
    
    // UI Visibility
    bool showProfiler = false;
    bool showMemory = false;
    bool showECS = false;
    bool showWorldEditor = false;
    bool showSettings = false;
    bool viewportHovered = false;
    
    // Renderer Settings
    bool vsync = true;
    bool wireframe = false;
    bool backfaceCulling = false;
    bool fullscreen = true;
    
    // Entities
    Entity playerEntity;
};
