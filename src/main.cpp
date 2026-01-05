#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "Renderer.hpp"
#include "GUIManager.hpp"
#include "Modules/GameState.hpp"
#include "Modules/RenderSystem.hpp"
#include "Modules/PlayerSystem.hpp"
#include "Modules/MobSystem.hpp"
#include "Modules/WeatherSystem.hpp"
#include "Modules/FluidSystem.hpp"
#include "Modules/ChatSystem.hpp"
#include "Modules/UISystem.hpp"
#include <chrono>
#include "imgui.h"

int main() {
    Renderer renderer;
    if (!renderer.init(1280, 720, "Voxel-Sim Architect")) return -1;

    GUIManager gui;
    gui.init(renderer.getWindow());
    GameState state;
    RenderSystem::RenderResources res;

    if (!RenderSystem::init(res)) return -1;

    // Initial Setup
    state.noise.SetSeed(state.worldSeed);
    state.noise.SetFrequency(state.worldFrequency);
    state.world.setRenderDistance(state.renderDistance);
    state.camera.setPosition({0.0f, 40.0f, 0.0f});
    state.playerEntity = state.registry.createEntity();
    state.registry.addComponent<Transform>(state.playerEntity, {state.camera.position()});
    MobSystem::spawnInitialMobs(state.registry, state.world);

    Framebuffer viewportBuffer;
    viewportBuffer.init(1280, 720);
    Vec2 viewportSize = {1280, 720};
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!renderer.shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;

        glfwPollEvents();
        gui.beginFrame();

        // Input & State Toggles
        if (ImGui::IsKeyPressed(ImGuiKey_F10)) state.menuMode = !state.menuMode;
        if (ImGui::IsKeyPressed(ImGuiKey_E) && !state.chatOpen) state.playerState.inventoryOpen = !state.playerState.inventoryOpen;
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) state.chatOpen = !state.chatOpen;

        // Systems Update
        if (!state.menuMode && !state.chatOpen && !state.playerState.inventoryOpen) {
            PlayerSystem::update(state.playerState, state.camera, state.world, renderer.getWindow(), dt, (float)glfwGetTime(), state.menuMode);
        }

        MobSystem::update(state.registry, state.world, dt);
        FluidSystem::update(state.world, state.camera.position(), dt);
        WeatherSystem::update(state.snowParticles, state.camera, dt, state.currentWeather);
        ChatSystem::update(state.chatHistory, state.chatInput, state.chatOpen, state.currentWeather, state.worldTime, state.world, state.worldSeed);
        state.world.update(state.camera.position(), state.noise, state.worldSeed, state.worldFrequency, state.worldBaseHeight, &state.scheduler);

        state.worldTime += dt * 13.33f;
        if (state.worldTime > 24000.0f) state.worldTime = 0.0f;

        // Sync Player Entity
        if (auto* t = state.registry.getComponent<Transform>(state.playerEntity)) t->position = state.camera.position();

        // Rendering
        RenderSystem::render(state, res, viewportBuffer, viewportSize);
        UISystem::render(state, res, viewportBuffer, viewportSize);

        gui.endFrame();
        renderer.swapBuffers();
    }

    gui.shutdown();
    renderer.shutdown();
    return 0;
}
