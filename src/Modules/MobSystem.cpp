#include "Modules/MobSystem.hpp"
#include "Modules/Physics.hpp"
#include "Chunk.hpp"
#include <cmath>
#include <algorithm>
#include <GLFW/glfw3.h>

namespace MobSystem {
    static bool isWater(uint8_t b) { return b == BLOCK_WATER; }
    static bool isLava(uint8_t b) { return b == BLOCK_LAVA; }

    Vec3 getHalfExtents(MobType t) {
        switch (t) {
            case MOB_CHICKEN: return {0.25f, 0.35f, 0.25f};
            case MOB_FISH: return {0.35f, 0.15f, 0.15f};
            case MOB_SALMON: return {0.45f, 0.18f, 0.18f};
            case MOB_OCTOPUS: return {0.40f, 0.30f, 0.40f};
            default: return {0.45f, 0.65f, 0.45f};
        }
    }

    float getBaseSpeed(MobType t) {
        switch (t) {
            case MOB_CHICKEN: return 2.2f;
            case MOB_FISH:
            case MOB_SALMON:
            case MOB_OCTOPUS:
                return 2.0f;
            default: return 1.8f;
        }
    }

    bool isAquatic(MobType t) {
        return t == MOB_FISH || t == MOB_SALMON || t == MOB_OCTOPUS;
    }

    void spawnInitialMobs(Registry& registry, const World& world) {
        auto mobPool = registry.getPool<Mob>();
        if (mobPool && !mobPool->entities.empty()) return;
        for (int i = 0; i < 32; ++i) {
            Entity e = registry.createEntity();
            Mob m;
            m.type = (MobType)(rand() % (int)MOB_COUNT);
            float x = (float)(rand() % 160 - 80);
            float z = (float)(rand() % 160 - 80);
            
            Vec3 pos;
            if (isAquatic(m.type)) {
                pos = {x, (float)(rand() % 8 + 2), z};
            } else {
                float y = Physics::findGroundY(world, x, z, 120);
                pos = {x, y, z};
            }
            
            m.yawDeg = (float)(rand() % 360);
            m.hp = isAquatic(m.type) ? 6.0f : 10.0f;
            m.wanderYawDeg = m.yawDeg;
            m.wanderTimer = (float)(rand() % 1000) / 1000.0f;
            
            registry.addComponent(e, Transform{pos});
            registry.addComponent(e, Velocity{{0,0,0}});
            registry.addComponent(e, m);
        }
    }

    void update(Registry& registry, World& world, double dt) {
        auto mobPool = registry.getPool<Mob>();
        if (!mobPool) return;

        for (size_t i = 0; i < mobPool->entities.size(); ++i) {
            Entity e = mobPool->entities[i];
            Mob& mob = mobPool->components[i];
            if (mob.hp <= 0.0f) continue;

            Transform* transform = registry.getComponent<Transform>(e);
            Velocity* velocity = registry.getComponent<Velocity>(e);
            if (!transform || !velocity) continue;

            Vec3& position = transform->position;
            Vec3& vel = velocity->val;

            Vec3 half = getHalfExtents(mob.type);
            int mx = (int)std::floor(position.x);
            int mz = (int)std::floor(position.z);
            int myFeet = (int)std::floor(position.y);
            bool inWater = isWater(world.getBlock(mx, myFeet, mz)) || isWater(world.getBlock(mx, myFeet + 1, mz));
            bool inLava = isLava(world.getBlock(mx, myFeet, mz)) || isLava(world.getBlock(mx, myFeet + 1, mz));

            if (inLava) {
                mob.hp -= (float)dt * 6.0f;
                mob.onFireSeconds = 2.0f;
            }
            if (mob.onFireSeconds > 0.0f) {
                mob.onFireSeconds = std::max(0.0f, mob.onFireSeconds - (float)dt);
                mob.hp -= (float)dt * 1.5f;
            }
            if (isAquatic(mob.type) && !inWater) {
                mob.hp -= (float)dt * 2.0f;
            }

            mob.wanderTimer -= (float)dt;
            if (mob.wanderTimer <= 0.0f) {
                mob.wanderTimer = 2.0f + (float)(rand() % 3000) / 1000.0f;
                mob.wanderYawDeg += (float)(rand() % 180 - 90);
            }
            
            float yawRad = mob.yawDeg * 0.01745329252f;
            Vec3 frontCheck = position + Vec3{std::sin(yawRad) * 1.5f, 0.0f, std::cos(yawRad) * 1.5f};
            uint8_t bFront = world.getBlock((int)frontCheck.x, (int)frontCheck.y, (int)frontCheck.z);
            uint8_t bBelow = world.getBlock((int)frontCheck.x, (int)frontCheck.y - 1, (int)frontCheck.z);
            
            if (isLava(bFront) || (bBelow == 0 && !isAquatic(mob.type))) {
                mob.wanderYawDeg += 180.0f;
            }

            float yawDelta = mob.wanderYawDeg - mob.yawDeg;
            if (yawDelta > 180.0f) yawDelta -= 360.0f;
            if (yawDelta < -180.0f) yawDelta += 360.0f;
            mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 3.0f);

            float speed = getBaseSpeed(mob.type);
            if (inWater) speed *= 0.50f;
            if (inLava) speed *= 0.20f;
            
            Vec3 wish = {std::sin(yawRad) * speed, 0.0f, std::cos(yawRad) * speed};
            
            if (isAquatic(mob.type)) {
                wish.y = sin((float)glfwGetTime() * 1.5f + position.x * 0.2f) * 1.2f;
                if (!inWater) wish.y = -2.0f;
            }

            vel.x = wish.x;
            vel.z = wish.z;

            auto collideWithOtherMobs = [&](const Vec3& pos, Entity current) {
                auto otherMobPool = registry.getPool<Mob>();
                if (!otherMobPool) return false;
                for (size_t j = 0; j < otherMobPool->entities.size(); ++j) {
                    Entity otherE = otherMobPool->entities[j];
                    if (otherE == current) continue;
                    const Mob& other = otherMobPool->components[j];
                    if (other.hp <= 0.0f) continue;
                    
                    Transform* otherTransform = registry.getComponent<Transform>(otherE);
                    if (!otherTransform) continue;
                    
                    float distSq = (pos.x - otherTransform->position.x)*(pos.x - otherTransform->position.x) + 
                                    (pos.z - otherTransform->position.z)*(pos.z - otherTransform->position.z);
                    if (distSq < 0.7f && std::abs(pos.y - otherTransform->position.y) < 1.2f) return true;
                }
                return false;
            };

            bool onGround = world.isSolid((int)std::floor(position.x), (int)std::floor(position.y - 0.1f), (int)std::floor(position.z));
            if (!isAquatic(mob.type)) {
                if (!inWater && !onGround) vel.y -= 28.0f * (float)dt;
                if (inWater) vel.y += 6.0f * (float)dt;
                if (onGround) {
                    vel.y = std::max(0.0f, vel.y);
                    Vec3 front = position + Vec3{std::sin(yawRad) * 0.7f, 0.0f, std::cos(yawRad) * 0.7f};
                    if (world.isSolid((int)std::floor(front.x), (int)std::floor(front.y), (int)std::floor(front.z)) ||
                        world.isSolid((int)std::floor(front.x), (int)std::floor(front.y + 1.0f), (int)std::floor(front.z))) {
                        vel.y = 9.0f;
                    }
                    else if (rand() % 800 == 0 && !inWater) vel.y = 7.0f;
                }
            } else {
                vel.y = wish.y;
            }

            Vec3 newPos = position;
            Vec3 tryPos = newPos;
            tryPos.x += vel.x * (float)dt;
            if (!Physics::collideAABB(world, tryPos, half) && !collideWithOtherMobs(tryPos, e)) newPos.x = tryPos.x;
            else mob.wanderYawDeg += 180.0f;

            tryPos = newPos;
            tryPos.z += vel.z * (float)dt;
            if (!Physics::collideAABB(world, tryPos, half) && !collideWithOtherMobs(tryPos, e)) newPos.z = tryPos.z;
            else mob.wanderYawDeg += 180.0f;

            tryPos = newPos;
            tryPos.y += vel.y * (float)dt;
            if (!Physics::collideAABB(world, tryPos, half)) {
                newPos.y = tryPos.y;
            } else {
                vel.y = 0.0f;
            }

            position = newPos;
            if (position.y < 0.0f) position.y = 100.0f;
        }
    }

    void render(Registry& registry, const Shader& shader, GLMesh* mobMeshes) {
        auto mobPool = registry.getPool<Mob>();
        if (!mobPool) return;

        for (size_t i = 0; i < mobPool->entities.size(); ++i) {
            Entity e = mobPool->entities[i];
            const Mob& mob = mobPool->components[i];
            if (mob.hp <= 0.0f) continue;
            
            Transform* transform = registry.getComponent<Transform>(e);
            if (!transform) continue;

            Vec3 half = getHalfExtents(mob.type);
            Mat4 model = translate(transform->position) * rotateY(mob.yawDeg * 0.01745329252f) * scale({half.x * 2.0f, half.y * 2.0f, half.z * 2.0f});
            shader.setMat4("uModel", model);
            mobMeshes[(int)mob.type].draw();
        }
    }
}
