#pragma once
#include "Modules/Components.hpp"
#include "ECS.hpp"
#include "World.hpp"
#include "GLMesh.hpp"
#include "Shader.hpp"

namespace MobSystem {
    Vec3 getHalfExtents(MobType t);
    float getBaseSpeed(MobType t);
    bool isAquatic(MobType t);
    
    void spawnInitialMobs(Registry& registry, const World& world);
    void update(Registry& registry, World& world, double dt);
    void render(Registry& registry, const Shader& shader, GLMesh* mobMeshes);
}
