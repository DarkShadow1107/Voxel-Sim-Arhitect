#include "Modules/PlayerSystem.hpp"
#include "Chunk.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace PlayerSystem {
    void init(PlayerState& state) {
        state.inventory.counts[BLOCK_DIRT] = 64;
        state.inventory.counts[BLOCK_GRASS] = 64;
        state.inventory.counts[BLOCK_STONE] = 64;
        state.inventory.counts[BLOCK_WOOD] = 64;
        state.inventory.counts[BLOCK_LEAVES] = 64;
        state.inventory.counts[BLOCK_SAND] = 64;
        state.inventory.counts[BLOCK_GLASS] = 64;

        state.advancements = {
            {"Stone Age", false, 10, BLOCK_STONE},
            {"Lumberjack", false, 5, BLOCK_WOOD},
            {"Gardener", false, 20, BLOCK_GRASS},
            {"Mountaineer", false, 1, BLOCK_SNOW}
        };
    }

    static bool isWater(uint8_t b) { return b == BLOCK_WATER; }
    static bool isLava(uint8_t b) { return b == BLOCK_LAVA; }

    void update(PlayerState& state, Camera& camera, World& world, GLFWwindow* window, float dt, float totalTime, bool menuMode) {
        if (menuMode) return;

        // Inventory Toggle
        static bool eWasDown = false;
        bool eDown = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
        if (eDown && !eWasDown) {
            state.inventoryOpen = !state.inventoryOpen;
        }
        eWasDown = eDown;

        // Mouse Look
        static double lastMouseX = 0.0, lastMouseY = 0.0;
        static bool hadMouse = false;
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        if (!hadMouse) {
            lastMouseX = mx; lastMouseY = my;
            hadMouse = true;
        }
        float dx = (float)(mx - lastMouseX);
        float dy = (float)(my - lastMouseY);
        lastMouseX = mx; lastMouseY = my;
        camera.addYawPitch(dx * 0.15f, dy * 0.15f);

        // Block Selection
        for (int i = 0; i < 9; ++i) {
            if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS) state.selectedBlock = i + 1;
        }

        // Sprint Detection
        bool wDown = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
        if (wDown && !state.wWasDown) {
            if (totalTime - state.lastWPressTime < 0.25f) state.isSprinting = true;
            state.lastWPressTime = totalTime;
        }
        if (!wDown) state.isSprinting = false;
        state.wWasDown = wDown;

        // Fly Toggle
        static bool fPressed = false;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            if (!fPressed) {
                state.isFlying = !state.isFlying;
                fPressed = true;
            }
        } else fPressed = false;

        // Movement Speed
        float speedMultiplier = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ? 2.5f : 1.0f;
        if (state.isSprinting) speedMultiplier *= 1.8f;
        if (state.isFlying) speedMultiplier *= 2.5f;

        const Vec3 camPos = camera.position();
        uint8_t bFeet = world.getBlock((int)std::floor(camPos.x), (int)std::floor(camPos.y - 1.4f), (int)std::floor(camPos.z));
        uint8_t bBody = world.getBlock((int)std::floor(camPos.x), (int)std::floor(camPos.y - 0.2f), (int)std::floor(camPos.z));
        bool inWater = isWater(bFeet) || isWater(bBody);
        bool inLava = isLava(bFeet) || isLava(bBody);
        
        if (inWater) speedMultiplier *= 0.65f;
        if (inLava) speedMultiplier *= 0.25f;
        if (inLava) state.onFireSeconds = 2.0f;
        if (state.onFireSeconds > 0) state.onFireSeconds -= dt;

        float speed = 7.2f * speedMultiplier;
        float forward = 0.0f, right = 0.0f, up = 0.0f;

        if (wDown) forward += speed * dt;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) forward -= speed * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) right += speed * dt;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) right -= speed * dt;
        
        if (state.isFlying) {
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) up += speed * dt;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) up -= speed * dt;
        } else if (inWater) {
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) up = speed * dt * 0.8f;
            else up = speed * dt * 0.15f; // Buoyancy
        } else {
            bool onGround = world.isSolid((int)std::floor(camPos.x), (int)std::floor(camPos.y - 1.6f), (int)std::floor(camPos.z));
            if (!onGround) state.verticalVelocity -= 28.0f * dt;
            else {
                if (state.verticalVelocity < 0) state.verticalVelocity = 0;
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) state.verticalVelocity = 10.0f;
            }
            up += state.verticalVelocity * dt;
        }

        // Collision & Movement
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

        Vec3 oldPos = camera.position();
        Vec3 nextPos = oldPos;

        // X Movement
        Vec3 tryX = nextPos;
        tryX.x += camera.right().x * right + camera.forward().x * forward;
        if (!checkColl(tryX)) nextPos.x = tryX.x;
        else {
            Vec3 stepX = tryX; stepX.y += 1.1f;
            if (!checkColl(stepX)) nextPos = stepX;
        }

        // Z Movement
        Vec3 tryZ = nextPos;
        tryZ.z += camera.right().z * right + camera.forward().z * forward;
        if (!checkColl(tryZ)) nextPos.z = tryZ.z;
        else {
            Vec3 stepZ = tryZ; stepZ.y += 1.1f;
            if (!checkColl(stepZ)) nextPos = stepZ;
        }

        // Y Movement
        Vec3 tryY = nextPos;
        tryY.y += up;
        if (!checkColl(tryY)) nextPos.y = tryY.y;
        else {
            state.verticalVelocity = (up < 0) ? 0.0f : -2.0f;
        }

        camera.setPosition(nextPos);

        // Interaction (LMB/RMB)
        static bool rmbPressed = false;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            auto res = world.raycast(camera.position(), camera.forward(), 10.0f);
            if (res.hit) {
                uint8_t type = world.getBlock(res.x, res.y, res.z);
                if (type != BLOCK_WATER && type != BLOCK_LAVA && type != BLOCK_BEDROCK) {
                    if (!state.breaking || res.x != state.breakX || res.y != state.breakY || res.z != state.breakZ) {
                        state.breaking = true;
                        state.breakX = res.x; state.breakY = res.y; state.breakZ = res.z;
                        state.breakProgress = 0.0f;
                    }

                    float secs = 0.5f;
                    if (type == BLOCK_STONE) secs = 1.5f;
                    if (type == BLOCK_WOOD) secs = 1.0f;
                    
                    state.breakProgress += dt / secs;
                    if (state.breakProgress >= 1.0f) {
                        state.inventory.counts[type]++;
                        world.setBlock(res.x, res.y, res.z, 0);
                        state.breaking = false;
                    }
                }
            } else state.breaking = false;
        } else state.breaking = false;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            if (!rmbPressed) {
                if (state.inventory.counts[state.selectedBlock] > 0) {
                    auto res = world.raycast(camera.position(), camera.forward(), 10.0f);
                    if (res.hit) {
                        world.setBlock(res.x + res.nx, res.y + res.ny, res.z + res.nz, state.selectedBlock);
                        state.inventory.counts[state.selectedBlock]--;
                    }
                }
                rmbPressed = true;
            }
        } else rmbPressed = false;
    }
}
