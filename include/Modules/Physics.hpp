#pragma once
#include "Math.hpp"
#include "World.hpp"

namespace Physics {
    bool collideAABB(const World& world, const Vec3& pos, const Vec3& halfExt);
    float findGroundY(const World& world, float wx, float wz, int startY);
}
