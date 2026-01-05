#pragma once
#include "Modules/GameState.hpp"
#include "Modules/RenderSystem.hpp"
#include "Framebuffer.hpp"

namespace UISystem {
    void render(GameState& state, RenderSystem::RenderResources& res, Framebuffer& viewportBuffer, Vec2& viewportSize);
}
