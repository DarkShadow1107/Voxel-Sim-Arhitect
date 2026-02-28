#include "MobAI.hpp"
#include "Registry.hpp"
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

    // Decay combat timers
    if (mob.attackCooldown > 0.0f)  mob.attackCooldown  = std::max(0.0f, mob.attackCooldown  - (float)dt);
    if (mob.hurtFlashTimer > 0.0f)  mob.hurtFlashTimer  = std::max(0.0f, mob.hurtFlashTimer  - (float)dt);

    // Decay knockback (exponential drag each frame)
    const float knockDecay = std::pow(0.05f, (float)dt); // ~decays to 5% per second
    mob.knockbackVel.x *= knockDecay;
    mob.knockbackVel.y *= knockDecay;
    mob.knockbackVel.z *= knockDecay;

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
            case MOB_DOG: typeStr = "dog"; break;
            case MOB_CAT: typeStr = "cat"; break;
            default: break;
        }
        if (!typeStr.empty()) {
            AudioManager::getInstance().playAmbientMobSound(typeStr, transform.position, viewerPos);
        }
        mob.ambientSoundTimer = 10.0f + (float)(std::rand() % 20000) / 1000.0f; // 10-30 seconds
    }

    // AI State Machine - use graph if available, otherwise fallback to hardcoded
    float distToPlayer = length(transform.position - viewerPos);

    const auto& mobDef = GameRegistry::getInstance().getMob(mob.type);
    bool useGraph = !mobDef.aiGraph.nodes.empty();

    if (useGraph) {
        // --- Graph-based AI evaluation ---
        // Initialize to start node if needed
        if (mob.currentAINode < 0 && mobDef.aiGraph.startNodeId >= 0) {
            mob.currentAINode = mobDef.aiGraph.startNodeId;
            mob.aiNodeTimer = 0.0f;
        }

        const AINode* currentNode = mobDef.aiGraph.findNode(mob.currentAINode);
        if (currentNode) {
            mob.aiNodeTimer += (float)dt;

            // Evaluate outgoing connections for transitions
            for (const auto& conn : mobDef.aiGraph.connections) {
                if (conn.fromNodeId != mob.currentAINode) continue;

                bool condMet = false;
                float lhs = 0.0f;

                if (conn.conditionType == "distance_to_player") {
                    lhs = distToPlayer;
                } else if (conn.conditionType == "hp_below") {
                    lhs = mob.hp;
                } else if (conn.conditionType == "hp_above") {
                    lhs = mob.hp;
                } else if (conn.conditionType == "is_day") {
                    lhs = 1.0f; // Assume day for now (TODO: pass time of day)
                } else if (conn.conditionType == "is_night") {
                    lhs = 0.0f; // Assume not night
                } else if (conn.conditionType == "in_water") {
                    int mx = (int)std::floor(transform.position.x);
                    int my = (int)std::floor(transform.position.y);
                    int mz = (int)std::floor(transform.position.z);
                    lhs = (world.getBlock(mx, my, mz) == 4) ? 1.0f : 0.0f;
                } else if (conn.conditionType == "on_fire") {
                    lhs = mob.onFireSeconds > 0.0f ? 1.0f : 0.0f;
                } else if (conn.conditionType == "timer_expired") {
                    lhs = mob.aiNodeTimer;
                } else if (conn.conditionType == "random_chance") {
                    lhs = (float)(std::rand() % 1000) / 1000.0f;
                } else if (conn.conditionType == "was_attacked") {
                    lhs = 0.0f; // TODO: track damage events
                }

                float rhs = conn.conditionValue;
                if (conn.comparison == "<")       condMet = lhs < rhs;
                else if (conn.comparison == ">")  condMet = lhs > rhs;
                else if (conn.comparison == "<=") condMet = lhs <= rhs;
                else if (conn.comparison == ">=") condMet = lhs >= rhs;
                else if (conn.comparison == "==") condMet = std::abs(lhs - rhs) < 0.001f;

                if (condMet) {
                    mob.currentAINode = conn.toNodeId;
                    mob.aiNodeTimer = 0.0f;
                    currentNode = mobDef.aiGraph.findNode(mob.currentAINode);
                    break;
                }
            }

            // Execute current node behavior
            if (currentNode) {
                const std::string& action = currentNode->name;
                float spd = currentNode->moveSpeed;

                if (action == "Idle" || action == "Sleep") {
                    mob.isMoving = false;
                    mob.state = Mob::IDLE;
                } else if (action == "Wander" || action == "wander_random") {
                    mob.state = Mob::WANDER;
                    handleWander(mob, dt);
                } else if (action == "Follow" || action == "follow_player") {
                    mob.state = Mob::FOLLOW;
                    if (distToPlayer > 2.0f) {
                        mob.isMoving = true;
                        Vec3 dir = normalize(viewerPos - transform.position);
                        float targetYaw = std::atan2(dir.x, dir.z) * 57.2957795f;
                        float yawDelta = targetYaw - mob.yawDeg;
                        while (yawDelta > 180.0f) yawDelta -= 360.0f;
                        while (yawDelta < -180.0f) yawDelta += 360.0f;
                        mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 5.0f);
                    } else {
                        mob.isMoving = false;
                    }
                } else if (action == "Flee" || action == "flee_from_player") {
                    mob.state = Mob::FLEE;
                    mob.isMoving = true;
                    Vec3 dir = normalize(transform.position - viewerPos);
                    float targetYaw = std::atan2(dir.x, dir.z) * 57.2957795f;
                    float yawDelta = targetYaw - mob.yawDeg;
                    while (yawDelta > 180.0f) yawDelta -= 360.0f;
                    while (yawDelta < -180.0f) yawDelta += 360.0f;
                    mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 6.0f);
                } else if (action == "Attack" || action == "attack_player") {
                    if (distToPlayer > 1.5f) {
                        mob.isMoving = true;
                        Vec3 dir = normalize(viewerPos - transform.position);
                        float targetYaw = std::atan2(dir.x, dir.z) * 57.2957795f;
                        float yawDelta = targetYaw - mob.yawDeg;
                        while (yawDelta > 180.0f) yawDelta -= 360.0f;
                        while (yawDelta < -180.0f) yawDelta += 360.0f;
                        mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 5.0f);
                    } else {
                        mob.isMoving = false;
                    }
                } else if (action == "Patrol") {
                    mob.state = Mob::WANDER;
                    handleWander(mob, dt);
                } else if (action == "swim_wander") {
                    mob.state = Mob::WANDER;
                    handleWander(mob, dt);
                } else if (action == "fly_wander") {
                    mob.state = Mob::WANDER;
                    handleWander(mob, dt);
                    mob.velocity.y = std::sin(mob.animTime) * 2.0f;
                } else if (action == "jump") {
                    mob.velocity.y = 6.0f;
                } else {
                    // Unknown action, idle
                    mob.isMoving = false;
                }
            }
        }
    } else {
    // --- Fallback: original hardcoded AI ---

    auto isHostile = [](MobType t) {
        return t == MOB_ZOMBIE || t == MOB_SKELETON || t == MOB_CREEPER;
    };

    // Simple reaction to player
    if (mob.state != Mob::FLEE && mob.state != Mob::ATTACK) {
        if (distToPlayer < 5.0f && (mob.type == MOB_DOG || mob.type == MOB_CAT)) {
            mob.state = Mob::FOLLOW;
        } else if (distToPlayer < 14.0f && isHostile(mob.type)) {
            mob.state = Mob::FOLLOW; // Hostile mobs follow to attack
        } else if (distToPlayer < 3.0f && !isAquatic(mob.type) && mob.type != MOB_BIRD) {
            // Skittish animals flee
            if (mob.type == MOB_RABBIT || mob.type == MOB_CHICKEN || mob.type == MOB_BIRD) {
                mob.state = Mob::FLEE;
                mob.targetPos = transform.position + (transform.position - viewerPos) * 2.0f;
                mob.stateTimer = 3.0f;
            }
        }
    }

    switch (mob.state) {
        case Mob::IDLE:
        case Mob::WANDER:
            handleWander(mob, dt);
            break;
        case Mob::FOLLOW:
            if (distToPlayer > 18.0f) {
                mob.state = Mob::WANDER;
            } else if (isHostile(mob.type) && distToPlayer < 2.0f) {
                mob.state = Mob::ATTACK;
            } else if (distToPlayer > 1.5f) {
                mob.isMoving = true;
                Vec3 dir = normalize(viewerPos - transform.position);
                float targetYaw = std::atan2(dir.x, dir.z) * 57.2957795f;
                float yawDelta = targetYaw - mob.yawDeg;
                while (yawDelta > 180.0f) yawDelta -= 360.0f;
                while (yawDelta < -180.0f) yawDelta += 360.0f;
                mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 5.0f);
            } else {
                mob.isMoving = false;
            }
            break;
        case Mob::ATTACK:
            if (distToPlayer > 3.5f) {
                // Lost melee range – chase again
                mob.state = Mob::FOLLOW;
            } else {
                // Face and lunge at player
                mob.isMoving = true;
                Vec3 dir = normalize(viewerPos - transform.position);
                float targetYaw = std::atan2(dir.x, dir.z) * 57.2957795f;
                float yawDelta = targetYaw - mob.yawDeg;
                while (yawDelta > 180.0f) yawDelta -= 360.0f;
                while (yawDelta < -180.0f) yawDelta += 360.0f;
                mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 8.0f);
            }
            break;
        case Mob::FLEE:
            mob.stateTimer -= (float)dt;
            if (mob.stateTimer <= 0.0f) {
                mob.state = Mob::WANDER;
            } else {
                mob.isMoving = true;
                Vec3 dir = normalize(transform.position - viewerPos); // Run away
                float targetYaw = std::atan2(dir.x, dir.z) * 57.2957795f;
                float yawDelta = targetYaw - mob.yawDeg;
                while (yawDelta > 180.0f) yawDelta -= 360.0f;
                while (yawDelta < -180.0f) yawDelta += 360.0f;
                mob.yawDeg += yawDelta * std::min(1.0f, (float)dt * 8.0f);
            }
            break;
    }

    } // end else (fallback hardcoded AI)

    // 3. Environmental checks (water, lava, falling)
    handleEnvironment(mob, transform, world, dt);

    // 4. Movement Logic
    handleMovement(mob, transform, world, dt);

    // 5. Animation progress
    if (mob.isMoving || isAquatic(mob.type) || mob.type == MOB_BIRD) {
        float animSpeed = isAquatic(mob.type) ? 3.0f : 5.0f;
        if (mob.type == MOB_BIRD) animSpeed = 8.0f;
        if (mob.state == Mob::FLEE || mob.state == Mob::ATTACK) animSpeed *= 1.5f;
        mob.animTime += (float)dt * animSpeed;
    } else {
        // Advance slowly so idle breathing animation plays
        mob.animTime += (float)dt * 0.8f;
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

    // Organic micro-steering jitter while walking
    if (mob.isMoving) {
        float jitter = ((float)(std::rand() % 201) - 100.0f) * 0.003f; // ±0.3 deg/s
        mob.wanderYawDeg += jitter * (float)dt * 60.0f;
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
    bool inLava  = (feetBlock == 5 || headBlock == 5); // BLOCK_LAVA
    bool inFire  = (feetBlock == 28 || headBlock == 28); // BLOCK_FIRE

    // Lava: heavy damage and sets on fire
    if (inLava) {
        mob.hp -= (float)dt * 4.0f; // 4 HP/sec in lava (like Minecraft)
        mob.onFireSeconds = 15.0f;  // Lava sets on fire for 15 seconds
        // Panic: force flee state away from lava
        mob.state = Mob::FLEE;
        mob.stateTimer = 3.0f;
        mob.wanderYawDeg += 180.0f + (float)(std::rand() % 90 - 45);
        mob.isMoving = true;
    }

    // Fire block contact: damage and continue burning
    if (inFire && !inLava) {
        mob.hp -= (float)dt * 1.0f;
        mob.onFireSeconds = std::max(mob.onFireSeconds, 8.0f);
        mob.state = Mob::FLEE;
        mob.stateTimer = 2.0f;
        mob.isMoving = true;
    }

    // On fire: flee toward water if nearby, otherwise random panic
    if (mob.onFireSeconds > 0.0f && !inLava && !inFire) {
        if (mob.state != Mob::ATTACK) { // Don't interrupt attack
            mob.state = Mob::FLEE;
            if (mob.stateTimer <= 0.0f) mob.stateTimer = 2.0f;
            mob.isMoving = true;

            // Search for water nearby to extinguish fire
            bool foundWater = false;
            for (int dx = -5; dx <= 5 && !foundWater; ++dx) {
                for (int dz = -5; dz <= 5 && !foundWater; ++dz) {
                    for (int dy = -2; dy <= 2 && !foundWater; ++dy) {
                        if (world.getBlock(mx + dx, my + dy, mz + dz) == 4) { // BLOCK_WATER
                            // Run toward water
                            float tx = (float)(mx + dx);
                            float tz = (float)(mz + dz);
                            float diffX = tx - transform.position.x;
                            float diffZ = tz - transform.position.z;
                            mob.wanderYawDeg = std::atan2(diffX, diffZ) * 57.2957795f;
                            foundWater = true;
                        }
                    }
                }
            }
            if (!foundWater) {
                // Panic: random direction changes
                if ((std::rand() % 5) == 0) mob.wanderYawDeg += (float)(std::rand() % 180 - 90);
            }
        }

        // Water extinguishes fire
        if (inWater) mob.onFireSeconds = 0.0f;
    }

    if (isAquatic(mob.type) && !inWater) {
        mob.hp -= (float)dt * 2.0f;
        mob.velocity.y = -4.0f;
        mob.isMoving = true;
    }

    // Obstacle / Cliff Avoidance
    float yawRad = mob.yawDeg * 0.01745329252f;
    Vec3 forward = {std::sin(yawRad), 0.0f, std::cos(yawRad)};
    Vec3 frontCheck = transform.position + forward * 1.5f;

    uint8_t bFront = world.getBlock((int)std::floor(frontCheck.x), (int)std::floor(frontCheck.y), (int)std::floor(frontCheck.z));
    uint8_t bBelow = world.getBlock((int)std::floor(frontCheck.x), (int)std::floor(frontCheck.y - 1.0f), (int)std::floor(frontCheck.z));

    // Avoid lava in front (5 = BLOCK_LAVA)
    if (bFront == 5 || bBelow == 5 || (bBelow == 0 && !isAquatic(mob.type) && mob.type != MOB_BIRD)) {
        mob.wanderYawDeg += 90.0f + (float)(std::rand() % 180);
        if (mob.type == MOB_BIRD) mob.velocity.y = 5.0f;
    }
}

void MobAI::handleMovement(Mob& mob, Transform& transform, World& world, double dt) {
    float yawRad = mob.yawDeg * 0.01745329252f;
    Vec3 forward = {std::sin(yawRad), 0.0f, std::cos(yawRad)};
    
    float speed = mob.isMoving ? getBaseSpeed(mob.type) : 0.0f;

    // On fire: mob sprints at 1.5x speed in panic (Minecraft-like)
    if (mob.onFireSeconds > 0.0f && mob.state == Mob::FLEE) speed *= 1.5f;

    int mx = (int)std::floor(transform.position.x);
    int my = (int)std::floor(transform.position.y);
    int mz = (int)std::floor(transform.position.z);
    bool inWater = (world.getBlock(mx, my, mz) == 4);
    bool inLava  = (world.getBlock(mx, my, mz) == 5);

    if (inWater) speed *= 0.6f;
    if (inLava)  speed *= 0.3f; // Very slow in lava

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
            } else if (world.isSolid((int)std::floor(jumpCheckPos.x), (int)std::floor(jumpCheckPos.y - 1.0f), (int)std::floor(jumpCheckPos.z)) == false) {
                // Avoid falling off cliffs
                if (mob.state != Mob::FLEE && mob.state != Mob::FOLLOW) {
                    mob.wanderYawDeg += 180.0f;
                    mob.isMoving = false;
                }
            }
        }
    }

    mob.velocity.x = wishVel.x;
    mob.velocity.z = wishVel.z;
    if (isAquatic(mob.type) || mob.type == MOB_BIRD) mob.velocity.y = wishVel.y;

    float dvx = (mob.velocity.x + mob.knockbackVel.x) * (float)dt;
    float dvy = (mob.velocity.y + mob.knockbackVel.y) * (float)dt;
    float dvz = (mob.velocity.z + mob.knockbackVel.z) * (float)dt;

    if (!isAquatic(mob.type) && mob.type != MOB_BIRD) {
        // Per-axis sweep: prevents seizures when walking into a wall
        auto solidAt = [&](float px, float py, float pz) -> bool {
            return world.isSolid((int)std::floor(px), (int)std::floor(py),       (int)std::floor(pz))
                || world.isSolid((int)std::floor(px), (int)std::floor(py + 0.8f), (int)std::floor(pz));
        };

        // X axis
        if (!solidAt(transform.position.x + dvx, transform.position.y, transform.position.z)) {
            transform.position.x += dvx;
        } else {
            mob.velocity.x      = 0.0f;
            mob.knockbackVel.x  = 0.0f;
            // Deflect mob so it walks around the obstacle instead of freezing
            if (mob.isMoving) mob.wanderYawDeg += 90.0f + (float)(rand() % 91);
        }

        // Z axis
        if (!solidAt(transform.position.x, transform.position.y, transform.position.z + dvz)) {
            transform.position.z += dvz;
        } else {
            mob.velocity.z      = 0.0f;
            mob.knockbackVel.z  = 0.0f;
            if (mob.isMoving) mob.wanderYawDeg += 90.0f + (float)(rand() % 91);
        }

        // Y axis — gravity / ground snap (no ceil pop-up that caused oscillation)
        float newY = transform.position.y + dvy;
        if (world.isSolid((int)std::floor(transform.position.x), (int)std::floor(newY), (int)std::floor(transform.position.z))) {
            if (dvy < 0.0f) {
                // Land cleanly on top of the block
                transform.position.y = std::floor(newY + 1.0f);
                mob.velocity.y      = 0.0f;
                mob.knockbackVel.y  = 0.0f;
            } else {
                // Head-bonk: start falling
                mob.velocity.y = -2.0f;
            }
        } else {
            transform.position.y = newY;
        }
    } else {
        // Aquatic / bird: free movement, no collision
        transform.position.x += dvx;
        transform.position.y += dvy;
        transform.position.z += dvz;
    }
}

float MobAI::getBaseSpeed(MobType type) {
    return GameRegistry::getInstance().getMob(type).speed;
}

Vec3 MobAI::getHalfExtents(MobType type) {
    switch (type) {
        case MOB_CHICKEN: return {0.25f, 0.35f, 0.25f};
        case MOB_RABBIT: return {0.2f, 0.25f, 0.2f};
        case MOB_FISH:
        case MOB_SALMON: return {0.4f, 0.2f, 0.2f};
        case MOB_OCTOPUS: return {0.4f, 0.4f, 0.4f};
        case MOB_BIRD: return {0.2f, 0.2f, 0.2f};
        case MOB_ZOMBIE:
        case MOB_SKELETON:
        case MOB_CREEPER: return {0.3f, 0.9f, 0.3f};
        default: return {0.45f, 0.65f, 0.45f};
    }
}

bool MobAI::isAquatic(MobType type) {
    return GameRegistry::getInstance().getMob(type).isAquatic;
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

void MobAI::spawnMobsInChunk(Registry& registry, Chunk* chunk, int chunkX, int chunkZ, FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise, FastNoiseLite& mountainNoise) {
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

    BiomeType biome = Chunk::getBiomeAt(biomeNoise, continentalNoise, mountainNoise, wx, wz);

    // Determine Mob Type based on Biome
    std::vector<MobType> possibleMobs;
    if (biome == BIOME_POLAR || biome == BIOME_SNOWY) {
        possibleMobs = {MOB_SHEEP, MOB_BIRD, MOB_RABBIT, MOB_SKELETON};
    } else if (biome == BIOME_JUNGLE) {
        possibleMobs = {MOB_CAT, MOB_CHICKEN, MOB_BIRD, MOB_PIG, MOB_ZOMBIE, MOB_CREEPER};
    } else if (biome == BIOME_OCEAN) {
        possibleMobs = {MOB_SALMON, MOB_OCTOPUS};
    } else if (biome == BIOME_DESERT) {
        possibleMobs = {MOB_RABBIT, MOB_SKELETON, MOB_CREEPER};
    } else if (biome == BIOME_SAVANNA) { // Plains/Savanna
        possibleMobs = {MOB_COW, MOB_SHEEP, MOB_PIG, MOB_DOG, MOB_RABBIT, MOB_ZOMBIE};
    } else if (biome == BIOME_PLAINS) {
        possibleMobs = {MOB_COW, MOB_SHEEP, MOB_PIG, MOB_CHICKEN, MOB_DOG, MOB_RABBIT, MOB_ZOMBIE, MOB_SKELETON, MOB_CREEPER, MOB_VILLAGER};
    } else if (biome == BIOME_ASHWORLD) {
        possibleMobs = {MOB_ZOMBIE, MOB_SKELETON, MOB_CREEPER};
    } else if (biome == BIOME_MOUNTAINS) {
        possibleMobs = {MOB_SHEEP, MOB_RABBIT, MOB_BIRD, MOB_SKELETON};
    } else { // Default
         possibleMobs = {MOB_COW, MOB_SHEEP, MOB_PIG, MOB_CHICKEN, MOB_DOG, MOB_RABBIT, MOB_ZOMBIE, MOB_SKELETON, MOB_CREEPER};
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
            m.idleAnimOffset = (float)(rand() % 1000) / 1000.0f * 6.28318f; // random phase 0..2π
            
            registry.addComponent(ent, m);
            registry.addComponent(ent, t);
        }
    }
}
