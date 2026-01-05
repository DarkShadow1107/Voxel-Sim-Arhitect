#pragma once
#include "Modules/GameState.hpp"
#include "GLMesh.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include <vector>

namespace RenderSystem {
    struct RenderResources {
        Shader voxelShader;
        Texture atlas;
        std::vector<GLMesh> mobMeshes;
        std::vector<GLMesh> crackMeshes;
        GLMesh sunMesh;
        GLMesh moonMesh;
        GLMesh cloudMesh;
        GLMesh starMesh;
    };

    bool init(RenderResources& res);
    void render(GameState& state, RenderResources& res, Framebuffer& viewportBuffer, const Vec2& viewportSize);
}
