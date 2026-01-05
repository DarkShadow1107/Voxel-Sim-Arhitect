#pragma once
#include "Modules/GameState.hpp"
#include "Modules/RenderSystem.hpp"
#include "Framebuffer.hpp"
#include "GUIManager.hpp"
#include "Renderer.hpp"

namespace UISystem {
    void render(GameState& state, RenderSystem::RenderResources& res, Framebuffer& viewportBuffer, Vec2& viewportSize, GUIManager& gui, Renderer& renderer, float dt);
}
