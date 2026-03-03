#include "StructureGenerator.hpp"
#include "Chunk.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Internal helper — returns y of the first air block above solid ground at (x,z).
// Returns 1 if the column is all air (degenerate), or SizeY-1 at most.
// ---------------------------------------------------------------------------
static int findGround(Chunk* chunk, int x, int z) {
    if (x < 0 || x >= Chunk::SizeX || z < 0 || z >= Chunk::SizeZ) return 1;
    int y = Chunk::SizeY - 1;
    while (y > 0 && chunk->get(x, y, z) == BLOCK_AIR) --y;
    return y + 1; // first air block above the surface
}

// ---------------------------------------------------------------------------
// Trees
// ---------------------------------------------------------------------------

void StructureGenerator::generateTree(Chunk* chunk, int x, int y, int z, uint8_t woodType, uint8_t leafType) {
    int height = 5 + rand() % 3; // 5-7 blocks tall (taller trunk)
    for (int i = 0; i < height; ++i) {
        chunk->set(x, y + i, z, woodType);
    }

    // Full, lush spheroid canopy — NO gaps.
    // Shape: wider in XZ (r=3), shorter in Y (r=2.5), centred above trunk top.
    // Equation: (lx^2 + lz^2)/9 + ly^2/6.25 <= 1  (filled ellipsoid)
    // This guarantees a dense, Minecraft-like rounded treetop with no missing corners.
    for (int ly = -2; ly <= 3; ++ly) {
        for (int lx = -3; lx <= 3; ++lx) {
            for (int lz = -3; lz <= 3; ++lz) {
                // Normalised ellipsoid distance: < 1.0 is inside
                float dist = (float)(lx * lx + lz * lz) / 9.0f
                           + (float)(ly * ly) / 6.25f;
                if (dist <= 1.0f) {
                    int bx = x + lx;
                    int by = y + height + ly;
                    int bz = z + lz;
                    // Only overwrite air so trunk and ground are never replaced
                    if (chunk->get(bx, by, bz) == BLOCK_AIR)
                        chunk->set(bx, by, bz, leafType);
                }
            }
        }
    }
    // Crown tuft: fill a 1-block radius disc one block above the ellipsoid top
    const int tipY = y + height + 3;
    for (int lx = -1; lx <= 1; ++lx) {
        for (int lz = -1; lz <= 1; ++lz) {
            if (chunk->get(x + lx, tipY, z + lz) == BLOCK_AIR)
                chunk->set(x + lx, tipY, z + lz, leafType);
        }
    }
}

// Conifer / spruce-style pine tree — ideal for Mountains and Snowy biomes.
void StructureGenerator::generatePineTree(Chunk* chunk, int x, int y, int z) {
    const int height = 10 + rand() % 4; // 10-13 blocks tall — taller for drama

    // Trunk straight up
    for (int i = 0; i < height; ++i)
        chunk->set(x, y + i, z, BLOCK_WOOD);

    // Canopy: dense layered tiers — NO gaps, fills all interior voxels per layer.
    // Each tier uses a solid square (r² check) rather than circle for maximum density.
    // The widest tiers are at the base (skirt), tapering linearly to a point.
    struct Layer { int yOff; int r; };
    const Layer layers[] = {
        { height - 6, 4 },   // very wide skirt
        { height - 5, 4 },   // wide skirt
        { height - 4, 3 },   // lower crown
        { height - 3, 3 },   // lower crown continued
        { height - 2, 2 },   // mid crown
        { height - 1, 2 },   // mid crown continued
        { height,     2 },   // upper crown
        { height + 1, 1 },   // near-top
        { height + 2, 1 },   // near-top continued
        { height + 3, 0 },   // apex
    };

    for (const auto& l : layers) {
        const int cy = y + l.yOff;
        if (cy < 1 || cy >= Chunk::SizeY) continue;
        for (int lx = -l.r; lx <= l.r; ++lx) {
            for (int lz = -l.r; lz <= l.r; ++lz) {
                // Use r²+1 to always fill the ring corners — no gaps
                if (lx * lx + lz * lz <= l.r * l.r + l.r) {
                    if (chunk->get(x + lx, cy, z + lz) == BLOCK_AIR)
                        chunk->set(x + lx, cy, z + lz, BLOCK_LEAVES);
                }
            }
        }
    }
    // Apex leaf cap
    const int tipY = y + height + 3;
    if (tipY < Chunk::SizeY && chunk->get(x, tipY, z) == BLOCK_AIR)
        chunk->set(x, tipY, z, BLOCK_LEAVES);
}

// Jungle mega-tree — very tall 2×2 trunk with large spherical canopy
// and diagonal root buttresses extending from the base.
void StructureGenerator::generateMegaTree(Chunk* chunk, int x, int y, int z) {
    const int height = 14 + rand() % 5; // 14-18 blocks
    const int trunkWide = (height * 2) / 3; // lower 2/3 is 2×2 trunk

    // 2×2 trunk at base tapering to 1×1 near top
    for (int i = 0; i < height; ++i) {
        if (i < trunkWide) {
            chunk->set(x,     y + i, z,     BLOCK_WOOD);
            chunk->set(x + 1, y + i, z,     BLOCK_WOOD);
            chunk->set(x,     y + i, z + 1, BLOCK_WOOD);
            chunk->set(x + 1, y + i, z + 1, BLOCK_WOOD);
        } else {
            chunk->set(x, y + i, z, BLOCK_WOOD);
        }
    }

    // Root buttresses
    const int btDir[4][2] = { {-1, 0}, {2, 0}, {0, -1}, {0, 2} };
    for (const auto& d : btDir) {
        for (int step = 1; step <= 3; ++step) {
            const int bx = x + d[0] * step;
            const int bz = z + d[1] * step;
            const int by = y + step - 1;
            if (by < Chunk::SizeY && chunk->get(bx, by, bz) == BLOCK_AIR)
                chunk->set(bx, by, bz, BLOCK_WOOD);
        }
    }

    // Large spherical canopy centred above the 2×2 trunk
    const int canopyR = 5; // slightly larger canopy
    const int ccy     = y + height;
    for (int ly = -canopyR; ly <= canopyR + 1; ++ly) {
        for (int lx = -canopyR; lx <= canopyR; ++lx) {
            for (int lz = -canopyR; lz <= canopyR; ++lz) {
                if (lx * lx + ly * ly + lz * lz <= canopyR * canopyR + canopyR) {
                    const int tx = x + lx, ty = ccy + ly, tz = z + lz;
                    if (ty >= 1 && ty < Chunk::SizeY && chunk->get(tx, ty, tz) == BLOCK_AIR)
                        chunk->set(tx, ty, tz, BLOCK_LEAVES);
                }
            }
        }
    }
}

void StructureGenerator::generateCactus(Chunk* chunk, int x, int y, int z) {
    int height = 2 + rand() % 2;
    for (int i = 0; i < height; ++i) {
        chunk->set(x, y + i, z, BLOCK_TALL_GRASS); // placeholder — no BLOCK_CACTUS in palette yet
    }
}

// ---------------------------------------------------------------------------
// Buildings
// ---------------------------------------------------------------------------

// Small peasant house — 5×4×5 with cobblestone walls, plank floor/ceiling,
// glass windows, and a 2-tall door gap at the front centre.
void StructureGenerator::generateSmallHouse(Chunk* chunk, int x, int y, int z) {
    const int w = 5, h = 4, d = 5;
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            for (int dz = 0; dz < d; ++dz) {
                const bool wall = (dx == 0 || dx == w - 1 || dz == 0 || dz == d - 1);
                if (dy == 0) {
                    // Floor
                    chunk->set(x + dx, y + dy, z + dz, BLOCK_OAK_PLANKS);
                } else if (dy == h - 1) {
                    // Ceiling
                    chunk->set(x + dx, y + dy, z + dz, BLOCK_OAK_PLANKS);
                } else if (wall) {
                    // Door gap — 2-tall centred on the front face (dz == 0)
                    if (dz == 0 && dx == w / 2 && dy < 3) {
                        // leave air
                    } else {
                        // Windows — single glass block centred on each side wall
                        const bool sideWindow =
                            (dz == 0 && dx == w / 2 && dy == 2) ||  // above door
                            (dz == d - 1 && dx == w / 2) ||          // back wall
                            (dx == 0 && dz == d / 2) ||              // left wall
                            (dx == w - 1 && dz == d / 2);            // right wall
                        chunk->set(x + dx, y + dy, z + dz,
                                   sideWindow ? BLOCK_GLASS : BLOCK_COBBLESTONE);
                    }
                }
            }
        }
    }
}

// Larger house — 7×5×7 with cobblestone walls, plank floor/ceiling,
// glass windows on all sides, and a 2-tall door on the front face.
void StructureGenerator::generateLargeHouse(Chunk* chunk, int x, int y, int z) {
    const int w = 7, h = 5, d = 7;
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            for (int dz = 0; dz < d; ++dz) {
                const bool wall = (dx == 0 || dx == w - 1 || dz == 0 || dz == d - 1);
                if (dy == 0) {
                    chunk->set(x + dx, y + dy, z + dz, BLOCK_OAK_PLANKS);
                } else if (dy == h - 1) {
                    chunk->set(x + dx, y + dy, z + dz, BLOCK_OAK_PLANKS);
                } else if (wall) {
                    // Door gap on front (dz == 0), centre column
                    if (dz == 0 && dx == w / 2 && dy < 3) {
                        // leave air
                    } else {
                        // Windows: two per wall side at dy=1 and dy=2
                        const bool win =
                            (dz == 0    && (dx == 2 || dx == 4) && dy >= 1 && dy <= 2) ||
                            (dz == d-1  && (dx == 2 || dx == 4) && dy >= 1 && dy <= 2) ||
                            (dx == 0    && (dz == 2 || dz == 4) && dy >= 1 && dy <= 2) ||
                            (dx == w-1  && (dz == 2 || dz == 4) && dy >= 1 && dy <= 2);
                        chunk->set(x + dx, y + dy, z + dz,
                                   win ? BLOCK_GLASS : BLOCK_COBBLESTONE);
                    }
                }
            }
        }
    }
}

// Village well — 3×4×3 cobblestone structure, hollow interior, water at base.
void StructureGenerator::generateVillageWell(Chunk* chunk, int x, int y, int z) {
    // Mossy stone base ring at ground level
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue;
            chunk->set(x + dx, y - 1, z + dz, BLOCK_MOSSY_STONE);
        }
    }

    // Cobblestone walls 3 blocks high (hollow centre)
    for (int dy = 0; dy < 3; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dz == 0) continue; // hollow interior
                chunk->set(x + dx, y + dy, z + dz, BLOCK_COBBLESTONE);
            }
        }
    }

    // Water at the bottom of the well (inside the hollow)
    chunk->set(x, y, z, BLOCK_WATER);

    // Simple arch on top — two corner pillars plus a cap
    chunk->set(x - 1, y + 3, z, BLOCK_COBBLESTONE);
    chunk->set(x + 1, y + 3, z, BLOCK_COBBLESTONE);
    chunk->set(x,     y + 3, z, BLOCK_OAK_PLANKS); // wooden beam / cap
}

// Farm plot — flat dirt patch (w × d), cleared to the ground.
void StructureGenerator::generateFarmPlot(Chunk* chunk, int x, int y, int z, int w, int d) {
    for (int dx = 0; dx < w; ++dx) {
        for (int dz = 0; dz < d; ++dz) {
            const int px = x + dx, pz = z + dz;
            if (px < 0 || px >= Chunk::SizeX || pz < 0 || pz >= Chunk::SizeZ) continue;
            // Find and replace the surface block with dirt
            int py = findGround(chunk, px, pz) - 1;
            if (py > 0 && py < Chunk::SizeY)
                chunk->set(px, py, pz, BLOCK_DIRT);
            // Scatter some tall grass over the farm
            if (rand() % 3 == 0 && py + 1 < Chunk::SizeY && chunk->get(px, py + 1, pz) == BLOCK_AIR)
                chunk->set(px, py + 1, pz, BLOCK_TALL_GRASS);
        }
    }
}

// L-shaped gravel path from (x1,z1) to (x2,z2).
// Walks in x first, then in z (Manhattan path), following the terrain surface.
void StructureGenerator::generatePath(Chunk* chunk, int x1, int z1, int x2, int z2) {
    const int dx = (x2 > x1) ? 1 : (x2 < x1 ? -1 : 0);
    const int dz = (z2 > z1) ? 1 : (z2 < z1 ? -1 : 0);

    // Walk x segment (guarded — skip if already aligned on x)
    if (dx != 0) {
        for (int px = x1; px != x2; px += dx) {
            if (px < 0 || px >= Chunk::SizeX || z1 < 0 || z1 >= Chunk::SizeZ) continue;
            int py = findGround(chunk, px, z1) - 1;
            if (py > 0 && py < Chunk::SizeY)
                chunk->set(px, py, z1, BLOCK_GRAVEL);
        }
    }

    // Walk z segment from (x2, z1) → (x2, z2) (guarded — skip if already aligned on z)
    if (dz != 0) {
        for (int pz = z1; pz != z2; pz += dz) {
            if (pz < 0 || pz >= Chunk::SizeZ || x2 < 0 || x2 >= Chunk::SizeX) continue;
            int py = findGround(chunk, x2, pz) - 1;
            if (py > 0 && py < Chunk::SizeY)
                chunk->set(x2, py, pz, BLOCK_GRAVEL);
        }
    }

    // Stamp the corner and endpoint
    if (x2 >= 0 && x2 < Chunk::SizeX && z2 >= 0 && z2 < Chunk::SizeZ) {
        int py = findGround(chunk, x2, z2) - 1;
        if (py > 0 && py < Chunk::SizeY)
            chunk->set(x2, py, z2, BLOCK_GRAVEL);
    }
}

// ---------------------------------------------------------------------------
// Village — Minecraft-style: central well, connected houses, paths, farms
// ---------------------------------------------------------------------------
void StructureGenerator::generateVillage(Chunk* chunk, int x, int y, int z) {
    // Clamp origin so all structures fit within the chunk
    const int cx = std::clamp(x, 6, Chunk::SizeX - 7);
    const int cz = std::clamp(z, 6, Chunk::SizeZ - 7);
    const int cy = findGround(chunk, cx, cz);
    if (cy <= 0 || cy >= Chunk::SizeY - 8) return;

    // 1. Central well
    generateVillageWell(chunk, cx, cy, cz);

    // 2. Houses — 8 candidate positions at cardinal + diagonal offsets
    const int dirs[8][2] = {
        {-11,  0}, { 11,  0}, {  0, -11}, {  0, 11},
        { -8, -8}, {  8, -8}, { -8,  8},  {  8,  8}
    };

    const int numHouses = 4 + rand() % 5; // 4-8 houses
    for (int i = 0; i < numHouses && i < 8; ++i) {
        // Large houses need 7-block clearance; small need 5
        const bool large = (i % 2 == 1);
        const int  margin = large ? 8 : 6;

        int hx = std::clamp(cx + dirs[i][0], 1, Chunk::SizeX - margin);
        int hz = std::clamp(cz + dirs[i][1], 1, Chunk::SizeZ - margin);
        int hy = findGround(chunk, hx, hz);
        if (hy <= 0 || hy >= Chunk::SizeY - 7) continue;

        if (large)
            generateLargeHouse(chunk, hx, hy, hz);
        else
            generateSmallHouse(chunk, hx, hy, hz);

        // Gravel path from house front to village centre
        generatePath(chunk, hx + (large ? 3 : 2), hz, cx, cz);
    }

    // 3. Farm plots — 1-2 near the centre
    const int farmOff[2][2] = { {-4, 5}, {5, -3} };
    const int nFarms = 1 + rand() % 2;
    for (int i = 0; i < nFarms; ++i) {
        int fx = std::clamp(cx + farmOff[i][0], 0, Chunk::SizeX - 7);
        int fz = std::clamp(cz + farmOff[i][1], 0, Chunk::SizeZ - 7);
        generateFarmPlot(chunk, fx, cy, fz, 5, 4);
    }

    // 4. Scatter 4 oak trees around the village perimeter
    const int treeOff[4][2] = { {-6, 3}, { 7, -4}, { 3,  7}, {-5, -6} };
    for (const auto& t : treeOff) {
        int tx = std::clamp(cx + t[0], 0, Chunk::SizeX - 1);
        int tz = std::clamp(cz + t[1], 0, Chunk::SizeZ - 1);
        int ty = findGround(chunk, tx, tz);
        if (ty > 0 && ty < Chunk::SizeY - 8)
            generateTree(chunk, tx, ty, tz, BLOCK_WOOD, BLOCK_LEAVES);
    }
}

// ---------------------------------------------------------------------------
// Other structures (unchanged)
// ---------------------------------------------------------------------------

void StructureGenerator::generateRuins(Chunk* chunk, int x, int y, int z) {
    for (int i = 0; i < 15; ++i) {
        int rx = rand() % 5;
        int ry = rand() % 3;
        int rz = rand() % 5;
        chunk->set(x + rx, y + ry, z + rz, (rand() % 2 == 0) ? BLOCK_COBBLESTONE : BLOCK_MOSSY_STONE);
    }
}

void StructureGenerator::generateVolcanoVent(Chunk* chunk, int x, int y, int z) {
    // Lava column with BASALT rim
    for (int i = 0; i < 4; ++i)
        chunk->set(x, y - i, z, BLOCK_LAVA);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            if (dx || dz)
                chunk->set(x + dx, y, z + dz, BLOCK_BASALT);
}

void StructureGenerator::generateObsidianSpire(Chunk* chunk, int x, int y, int z) {
    const int height = 6 + rand() % 7;
    for (int i = 0; i < height; ++i)
        chunk->set(x, y + i, z, BLOCK_OBSIDIAN);
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dz = -2; dz <= 2; ++dz) {
            if (dx == 0 && dz == 0) continue;
            int dist2 = dx * dx + dz * dz;
            if (dist2 <= 4) {
                if (chunk->get(x + dx, y, z + dz) == BLOCK_AIR)
                    chunk->set(x + dx, y, z + dz, BLOCK_BASALT);
                if (dist2 == 4 && chunk->get(x + dx, y + 1, z + dz) == BLOCK_AIR)
                    chunk->set(x + dx, y + 1, z + dz, BLOCK_ASH);
            }
        }
    }
    const int leanH = height / 2;
    const int ldx = (rand() % 3) - 1, ldz = (rand() % 3) - 1;
    if ((ldx || ldz) && y + leanH < Chunk::SizeY)
        chunk->set(x + ldx, y + leanH, z + ldz, BLOCK_OBSIDIAN);
}
