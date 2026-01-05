#include "Modules/FluidSystem.hpp"
#include <cmath>
#include <cstdlib>

namespace FluidSystem {
    void update(World& world, const Vec3& playerPos, float dt) {
        static float fluidTimer = 0.0f;
        fluidTimer += dt;
        if (fluidTimer < 0.15f) return;
        fluidTimer = 0.0f;

        int px = (int)std::floor(playerPos.x / Chunk::SizeX);
        int pz = (int)std::floor(playerPos.z / Chunk::SizeZ);
        
        auto tryFlow = [&](int x, int y, int z, uint8_t type) {
            if (y <= 1) return;
            // 1. Flow Down
            if (world.getBlock(x, y - 1, z) == 0) {
                world.setBlock(x, y - 1, z, type);
                return;
            }
            // 2. Flow Horizontally
            int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
            for (int i = 0; i < 4; ++i) {
                if (world.getBlock(x + dirs[i][0], y, z + dirs[i][1]) == 0) {
                    uint8_t below = world.getBlock(x + dirs[i][0], y - 1, z + dirs[i][1]);
                    if (below != 0 || y < 5) {
                        world.setBlock(x + dirs[i][0], y, z + dirs[i][1], type);
                    }
                }
            }
        };

        for (int i = 0; i < 6; ++i) {
            int rx = px + (rand() % 7 - 3);
            int rz = pz + (rand() % 7 - 3);
            for (int j = 0; j < 150; ++j) {
                int vx = rx * Chunk::SizeX + (rand() % Chunk::SizeX);
                int vz = rz * Chunk::SizeZ + (rand() % Chunk::SizeZ);
                int vy = rand() % (Chunk::SizeY - 2) + 1;
                uint8_t b = world.getBlock(vx, vy, vz);
                if (b == BLOCK_WATER || b == BLOCK_LAVA) {
                    tryFlow(vx, vy, vz, b);
                }
            }
        }
    }
}
