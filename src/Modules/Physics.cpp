#include "Modules/Physics.hpp"
#include <cmath>
#include <algorithm>

namespace Physics {
    bool collideAABB(const World& world, const Vec3& pos, const Vec3& halfExt) {
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                for (int sy = -1; sy <= 1; ++sy) {
                    float px = pos.x + halfExt.x * (float)sx;
                    float pz = pos.z + halfExt.z * (float)sz;
                    float py = pos.y + halfExt.y * (float)sy;
                    if (world.isSolid((int)std::floor(px), (int)std::floor(py), (int)std::floor(pz))) return true;
                }
            }
        }
        return false;
    }

    float findGroundY(const World& world, float wx, float wz, int startY) {
        int ix = (int)std::floor(wx);
        int iz = (int)std::floor(wz);
        for (int y = std::min(startY, Chunk::SizeY - 2); y >= 1; --y) {
            if (world.isSolid(ix, y, iz)) {
                return (float)(y + 1);
            }
        }
        return 80.0f;
    }
}
