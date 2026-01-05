#pragma once
#include "Camera.hpp"
#include "World.hpp"
#include "Modules/Components.hpp"

struct GLFWwindow;

namespace PlayerSystem {
    struct PlayerState {
        bool isFlying = false;
        bool isSprinting = false;
        float verticalVelocity = 0.0f;
        float lastWPressTime = 0.0f;
        bool wWasDown = false;
        float onFireSeconds = 0.0f;
        int selectedBlock = 1;
        
        struct Inventory {
            int counts[256] = {0};
        } inventory;

        struct Advancement {
            std::string title;
            bool achieved = false;
            int requiredCount = 0;
            uint8_t blockType = 0;
        };
        std::vector<Advancement> advancements;

        bool inventoryOpen = false;
        bool breaking = false;
        int breakX = 0, breakY = 0, breakZ = 0;
        float breakProgress = 0.0f;
        uint8_t breakType = 0;
    };

    void init(PlayerState& state);
    void update(PlayerState& state, Camera& camera, World& world, GLFWwindow* window, float dt, float totalTime, bool menuMode);
}
