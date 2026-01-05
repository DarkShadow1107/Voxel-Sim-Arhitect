#pragma once
#include "Math.hpp"
#include "imgui.h"
#include <string>
#include <vector>

struct Transform {
    Vec3 position;
};

struct Velocity {
    Vec3 val;
};

enum WeatherType { WEATHER_CLEAR, WEATHER_RAIN, WEATHER_SNOW };

enum MobType {
    MOB_COW = 0,
    MOB_PIG,
    MOB_SHEEP,
    MOB_CHICKEN,
    MOB_ZOMBIE,
    MOB_CREEPER,
    MOB_FISH,
    MOB_SALMON,
    MOB_OCTOPUS,
    MOB_COUNT
};

struct Mob {
    float yawDeg = 0.0f;
    MobType type = MOB_COW;
    float hp = 10.0f;
    float onFireSeconds = 0.0f;

    float wanderTimer = 0.0f;
    float wanderYawDeg = 0.0f;
};

struct SnowParticle {
    Vec3 position;
    Vec3 velocity;
    float life;
};

struct ChatMessage {
    std::string text;
    ImVec4 color = ImVec4(1,1,1,1);
};
