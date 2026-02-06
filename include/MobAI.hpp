#pragma once
#include "Math.hpp"
#include "World.hpp"
#include "ECS.hpp"
#include <vector>
#include <cmath>

enum MobType {
    MOB_COW,
    MOB_PIG,
    MOB_SHEEP,
    MOB_CHICKEN,
    MOB_FISH,
    MOB_SALMON,
    MOB_OCTOPUS,
    MOB_BIRD,
    MOB_CAT,
    MOB_DOG,
    MOB_RABBIT,
    MOB_COUNT
};

enum SheepColor {
    COLOR_WHITE,
    COLOR_BLACK,
    COLOR_GREY,
    COLOR_BROWN,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_PINK,
    COLOR_PURPLE
};

struct Mob {
    MobType type;
    float hp;
    float maxHp;
    float wanderTimer;
    float wanderYawDeg;
    float yawDeg;
    Vec3 velocity;
    float animTime;
    bool isMoving;
    float onFireSeconds;
    float ambientSoundTimer;
    SheepColor sheepColor;
    
    // Engine Improvements: Simple State Machine
    enum State { IDLE, WANDER, FOLLOW, FLEE } state;
    Vec3 targetPos;
    float stateTimer;

    Mob() : type(MOB_COW), hp(10.0f), maxHp(10.0f), wanderTimer(0.0f), wanderYawDeg(0.0f), 
           yawDeg(0.0f), velocity{0,0,0}, animTime(0.0f), isMoving(false), 
           onFireSeconds(0.0f), ambientSoundTimer(0.0f), sheepColor(COLOR_WHITE),
           state(IDLE), targetPos{0,0,0}, stateTimer(0.0f) {}
};

class FastNoiseLite;

class MobAI {
public:
    static void update(Mob& mob, Transform& transform, World& world, double dt, Vec3 viewerPos);
    static float getBaseSpeed(MobType type);
    static Vec3 getHalfExtents(MobType type);
    static bool isAquatic(MobType type);
    static Vec3 getSheepColor(SheepColor color);
    
    // Spawning
    static void spawnMobsInChunk(Registry& registry, Chunk* chunk, int chunkX, int chunkZ, FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise);

private:
    static void handleMovement(Mob& mob, Transform& transform, World& world, double dt);
    static void handleWander(Mob& mob, double dt);
    static void handleEnvironment(Mob& mob, Transform& transform, World& world, double dt);
};
