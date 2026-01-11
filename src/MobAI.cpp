#include "MobAI.hpp"
#include <algorithm>
#include <cstdlib>
#include "FastNoiseLite.h"
#include "Chunk.hpp"
#include "AudioManager.hpp"

// Helper to get biome for spawning (simplified version of Chunk::getBiomeAt logic for now, or assume we can access it)
// We need to replicate Chunk::getBiomeAt logic if it's static, or make it public static in Chunk.
// It was static in main.cpp, but let's check Chunk.hpp. 
// Chunk::getBiomeAt seems to be available or I should check.

void MobAI::update(Mob& mob, Transform& transform, World& world, double dt, Vec3 viewerPos) {
    // 1. Update timers and status
    if (mob.onFireSeconds > 0.0f) {
        mob.onFireSeconds = std::max(0.0f, mob.onFireSeconds - (float)dt);
        mob.hp -= (float)dt * 1.5f;
    }

    // Ambient Sound Logic
    mob.ambientSoundTimer -= (float)dt;
    if (mob.ambientSoundTimer <= 0.0f) {
        std::string typeStr = "";
        switch(mob.type) {
            case MOB_COW: typeStr = "cow"; break;
            case MOB_PIG: typeStr = "pig"; break;
            case MOB_CHICKEN: typeStr = "chicken"; break;
            case MOB_RABBIT: typeStr = "rabbit"; break;
            case MOB_SHEEP: typeStr = "sheep"; break;
            default: break;
        }
        if (!typeStr.empty()) {
            AudioManager::getInstance().playAmbientMobSound(typeStr, transform.position, viewerPos);
        }
        mob.ambientSoundTimer = 10.0f + (float)(std::rand() % 20000) / 1000.0f; // 10-30 seconds
    }

    // 2. Wander Logic (Minecraft-like idle/move states)
    handleWander(mob, dt);

    // 3. Environmental checks (water, lava, falling)
    handleEnvironment(mob, transform, world, dt);

    // 4. Movement Logic
    handleMovement(mob, transform, world, dt);

    // 5. Animation progress
    if (mob.isMoving || isAquatic(mob.type) || mob.type == MOB_BIRD) {
        float animSpeed = isAquatic(mob.type) ? 3.0f : 5.0f;
        if (mob.type == MOB_BIRD) animSpeed = 8.0f;
        mob.animTime += (float)dt * animSpeed;
    }
}

void MobAI::handleWander(Mob& mob, double dt) {
    mob.wanderTimer -= (float)dt;
    if (mob.wanderTimer <= 0.0f) {
        if (mob.isMoving) {
            // Stop and idle
            mob.isMoving = false;
            mob.wanderTimer = 1.0f + (float)(std::rand() % 4000) / 1000.0f;
        } else {
            // Start moving in a new direction
            mob.isMoving = true;
            mob.wanderTimer = 2.0f + (float)(std::rand() % 6000) / 1000.0f;
            mob.wanderYawDeg += (float)(std::rand() % 180 - 90);
        }
    }

    // Smooth rotation towards wanderYawDeg
    float yawDelta = mob.wanderYawDeg - mob.yawDeg;
    while (yawDelta > 180.0f) yawDelta -= 360.0f;
    while (yawDelta < -180.0f) yawDelta += 360.0f;
    mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 4.0f);
}

void MobAI::handleEnvironment(Mob& mob, Transform& transform, World& world, double dt) {
    int mx = (int)std::floor(transform.position.x);
    int my = (int)std::floor(transform.position.y);
    int mz = (int)std::floor(transform.position.z);

    uint8_t feetBlock = world.getBlock(mx, my, mz);
    uint8_t headBlock = world.getBlock(mx, my + 1, mz);

    bool inWater = (feetBlock == 4 || headBlock == 4); // BLOCK_WATER
    bool inLava = (feetBlock == 5 || headBlock == 5);  // BLOCK_LAVA

    if (inLava) {
        mob.hp -= (float)dt * 6.0f;
        mob.onFireSeconds = 2.0f;
    }

    if (isAquatic(mob.type) && !inWater) {
        mob.hp -= (float)dt * 2.0f;
        mob.velocity.y = -4.0f; // Sink if out of ocean
        mob.isMoving = true;    // Struggle
    }

    // Obstacle / Cliff Avoidance
    float yawRad = mob.yawDeg * 0.01745329252f;
    Vec3 forward = {std::sin(yawRad), 0.0f, std::cos(yawRad)};
    Vec3 frontCheck = transform.position + forward * 1.5f;

    uint8_t bFront = world.getBlock((int)std::floor(frontCheck.x), (int)std::floor(frontCheck.y), (int)std::floor(frontCheck.z));
    uint8_t bBelow = world.getBlock((int)std::floor(frontCheck.x), (int)std::floor(frontCheck.y - 1.0f), (int)std::floor(frontCheck.z));

    if (bFront == 5 || (bBelow == 0 && !isAquatic(mob.type) && mob.type != MOB_BIRD)) {
        mob.wanderYawDeg += 90.0f + (float)(std::rand() % 180);
        if (mob.type == MOB_BIRD) mob.velocity.y = 5.0f;
    }
}

void MobAI::handleMovement(Mob& mob, Transform& transform, World& world, double dt) {
    float yawRad = mob.yawDeg * 0.01745329252f;
    Vec3 forward = {std::sin(yawRad), 0.0f, std::cos(yawRad)};
    
    float speed = mob.isMoving ? getBaseSpeed(mob.type) : 0.0f;
    
    int mx = (int)std::floor(transform.position.x);
    int my = (int)std::floor(transform.position.y);
    int mz = (int)std::floor(transform.position.z);
    bool inWater = (world.getBlock(mx, my, mz) == 4);
    
    if (inWater) speed *= 0.6f;

    Vec3 wishVel = forward * speed;

    if (isAquatic(mob.type)) {
        wishVel.y = std::sin(mob.animTime * 0.5f) * 1.5f;
    } else if (mob.type == MOB_BIRD) {
        wishVel.y = std::sin(mob.animTime * 0.3f) * 3.0f;
    } else {
        // Gravity
        mob.velocity.y -= 15.0f * (float)dt;
        
        // Jumping over blocks
        bool onGround = world.isSolid(mx, (int)std::floor(transform.position.y - 0.1f), mz);
        
        // Rabbit specific hopping logic
        if (mob.type == MOB_RABBIT && onGround && mob.isMoving) {
            if (std::fmod(mob.animTime, 2.0f) < 0.2f) { // Hop every cycle
                mob.velocity.y = 4.0f;
                // We'll skip the sound here for now to avoid distortion, or only play if onGround just became true
            }
        }

        if (onGround && mob.isMoving) {
            Vec3 jumpCheckPos = transform.position + forward * 0.7f;
            if (world.isSolid((int)std::floor(jumpCheckPos.x), (int)std::floor(jumpCheckPos.y), (int)std::floor(jumpCheckPos.z))) {
                mob.velocity.y = 5.5f;
            }
        }
    }

    mob.velocity.x = wishVel.x;
    mob.velocity.z = wishVel.z;
    if (isAquatic(mob.type) || mob.type == MOB_BIRD) mob.velocity.y = wishVel.y;

    transform.position.x += mob.velocity.x * (float)dt;
    transform.position.y += mob.velocity.y * (float)dt;
    transform.position.z += mob.velocity.z * (float)dt;
    
    // Simple ground collision to keep them from falling through world
    if (!isAquatic(mob.type) && mob.type != MOB_BIRD) {
        if (world.isSolid((int)std::floor(transform.position.x), (int)std::floor(transform.position.y), (int)std::floor(transform.position.z))) {
            transform.position.y = std::ceil(transform.position.y);
            mob.velocity.y = 0;
        }
    }
}

float MobAI::getBaseSpeed(MobType type) {
    switch (type) {
        case MOB_CHICKEN: return 2.2f;
        case MOB_RABBIT: return 3.5f;
        case MOB_CAT: return 2.8f;
        case MOB_DOG: return 3.0f;
        case MOB_BIRD: return 4.0f;
        case MOB_FISH:
        case MOB_SALMON: return 2.5f;
        default: return 2.0f;
    }
}

Vec3 MobAI::getHalfExtents(MobType type) {
    switch (type) {
        case MOB_CHICKEN: return {0.25f, 0.35f, 0.25f};
        case MOB_RABBIT: return {0.2f, 0.25f, 0.2f};
        case MOB_FISH:
        case MOB_SALMON: return {0.4f, 0.2f, 0.2f};
        case MOB_OCTOPUS: return {0.4f, 0.4f, 0.4f};
        case MOB_BIRD: return {0.2f, 0.2f, 0.2f};
        default: return {0.45f, 0.65f, 0.45f};
    }
}

bool MobAI::isAquatic(MobType type) {
    return type == MOB_FISH || type == MOB_SALMON || type == MOB_OCTOPUS;
}

Vec3 MobAI::getSheepColor(SheepColor color) {
    switch (color) {
        case COLOR_BLACK: return {0.1f, 0.1f, 0.1f};
        case COLOR_GREY: return {0.5f, 0.5f, 0.5f};
        case COLOR_BROWN: return {0.4f, 0.26f, 0.13f};
        case COLOR_RED: return {0.8f, 0.1f, 0.1f};
        case COLOR_YELLOW: return {0.9f, 0.9f, 0.1f};
        case COLOR_BLUE: return {0.1f, 0.1f, 0.8f};
        case COLOR_GREEN: return {0.1f, 0.6f, 0.1f};
        case COLOR_PINK: return {1.0f, 0.75f, 0.8f};
        case COLOR_PURPLE: return {0.5f, 0.0f, 0.5f};
        default: return {1.0f, 1.0f, 1.0f};
    }
}

void MobAI::spawnMobsInChunk(Registry& registry, Chunk* chunk, int chunkX, int chunkZ, FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise) {
    if (!chunk) return;

    // Chance to spawn pack in this chunk: 25%
    if (rand() % 100 > 25) return;

    // Pack size: 1-3
    int packSize = (rand() % 3) + 1;
    
    // Pick a random spot in the chunk for the pack center
    int cx = rand() % (Chunk::SizeX - 4) + 2; 
    int cz = rand() % (Chunk::SizeZ - 4) + 2;
    
    float wx = (float)(chunkX * Chunk::SizeX + cx);
    float wz = (float)(chunkZ * Chunk::SizeZ + cz);

    BiomeType biome = Chunk::getBiomeAt(biomeNoise, continentalNoise, wx, wz);

    // Determine Mob Type based on Biome
    std::vector<MobType> possibleMobs;
    if (biome == BIOME_POLAR || biome == BIOME_SNOWY) {
        possibleMobs = {MOB_SHEEP, MOB_BIRD, MOB_CHICKEN, MOB_RABBIT};
    } else if (biome == BIOME_JUNGLE) {
        possibleMobs = {MOB_CAT, MOB_CHICKEN, MOB_BIRD, MOB_PIG};
    } else if (biome == BIOME_OCEAN) {
        possibleMobs = {MOB_SALMON, MOB_OCTOPUS};
    } else if (biome == BIOME_DESERT) {
        possibleMobs = {MOB_COW, MOB_PIG, MOB_RABBIT};
    } else if (biome == BIOME_SAVANNA) { // Plains/Savanna
        possibleMobs = {MOB_COW, MOB_SHEEP, MOB_PIG, MOB_DOG, MOB_RABBIT};
    } else { // Plains / Default
         possibleMobs = {MOB_COW, MOB_SHEEP, MOB_PIG, MOB_CHICKEN, MOB_DOG, MOB_RABBIT};
    }

    if (possibleMobs.empty() && biome != BIOME_OCEAN) possibleMobs = {MOB_BIRD};
    
    MobType packType;
    if (biome == BIOME_OCEAN) {
         packType = (rand() % 2 == 0) ? MOB_SALMON : MOB_OCTOPUS;
    } else {
        if (possibleMobs.empty()) packType = MOB_COW;
        else packType = possibleMobs[rand() % possibleMobs.size()];
    }

    for (int i = 0; i < packSize; ++i) {
        // Offset within chunk
        int ox = cx + (rand() % 5 - 2);
        int oz = cz + (rand() % 5 - 2);
        
        // Clamp to chunk bounds (or wrap validly)
        ox = std::max(0, std::min(Chunk::SizeX - 1, ox));
        oz = std::max(0, std::min(Chunk::SizeZ - 1, oz));
        
        bool isWaterMob = isAquatic(packType);
        bool isBird = (packType == MOB_BIRD);
        
        // Find ground/water surface
        int spawnY = -1;
        
        // Start from top logic
        for (int y = Chunk::SizeY - 2; y >= 0; --y) {
            uint8_t b = chunk->get(ox, y, oz);
            uint8_t bAbove = chunk->get(ox, y+1, oz);
            
            if (isWaterMob) {
                if (b == 4 && (bAbove == 4 || bAbove == 0)) { // is water, space above is water or air
                     spawnY = y;
                     break;
                }
            } else {
                // Land mob: block solid, above is air
                if (b != 0 && b != 4 && b != 5 && b != 13 && bAbove == 0) {
                     spawnY = y + 1;
                     break;
                }
            }
        }

        if (spawnY > 0 && spawnY < Chunk::SizeY) {
            Entity ent = registry.createEntity();
            Mob m;
            m.type = packType;

             // Sheep colors
            if (m.type == MOB_SHEEP) {
                int r = rand() % 1000;
                if (r < 5) m.sheepColor = COLOR_PURPLE;
                else if (r < 15) m.sheepColor = COLOR_PINK;
                else if (r < 40) m.sheepColor = COLOR_RED;
                else if (r < 70) m.sheepColor = COLOR_YELLOW;
                else if (r < 100) m.sheepColor = COLOR_BLUE;
                else if (r < 130) m.sheepColor = COLOR_GREEN;
                else if (r < 200) m.sheepColor = COLOR_BLACK;
                else m.sheepColor = COLOR_WHITE;
            }

            Transform t;
            t.position = { (float)(chunkX * Chunk::SizeX + ox) + 0.5f, (float)spawnY, (float)(chunkZ * Chunk::SizeZ + oz) + 0.5f };
            
            if (isBird) t.position.y += 20.0f + (rand() % 20);

            m.yawDeg = (float)(rand() % 360);
            m.wanderYawDeg = m.yawDeg;
            m.wanderTimer = (float)(rand() % 300) / 10.0f;
            m.isMoving = false;
            m.hp = isWaterMob ? 8.0f : 10.0f;
            
            registry.addComponent(ent, m);
            registry.addComponent(ent, t);
        }
    }
}
