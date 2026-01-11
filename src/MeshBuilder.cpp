#include "MeshBuilder.hpp"
#include "Chunk.hpp"
#include <algorithm>

static float getAO(const Chunk& chunk, int x, int y, int z, int face, int corner) {
    // Very simple AO: check 3 neighbors around the corner
    // face order: +X, -X, +Y, -Y, +Z, -Z
    // This is a simplified version.
    return 1.0f; // Placeholder for now, will implement full AO if requested
}

static void addVoxelFace(std::vector<Vertex>& out, const Chunk& chunk, int x, int y, int z, int face, uint8_t type) {
    static const float normals[6][3] = {
        { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 },
    };

    float r = 1.0f, g = 1.0f, b = 1.0f;
    switch(type) {
        case 1: r = 0.55f; g = 0.40f; b = 0.25f; break; // Dirt
        case 2: // Grass
            if (face == 2) { r = 0.30f; g = 0.75f; b = 0.30f; } 
            else { r = 0.55f; g = 0.40f; b = 0.25f; } 
            break;
        case 3: r = 0.60f; g = 0.60f; b = 0.65f; break; // Stone
        case 4: r = 0.20f; g = 0.40f; b = 0.90f; break; // Water
        case 5: r = 1.00f; g = 0.30f; b = 0.00f; break; // Lava
        case 6: r = 0.45f; g = 0.30f; b = 0.15f; break; // Wood
        case 7: r = 0.20f; g = 0.60f; b = 0.20f; break; // Leaves
        case 8: r = 0.95f; g = 0.90f; b = 0.60f; break; // Sand
        case 9: r = 1.00f; g = 1.00f; b = 1.00f; break; // Snow
        case 10: r = 0.20f; g = 0.20f; b = 0.20f; break; // Bedrock
        case 11: r = 1.00f; g = 0.20f; b = 0.20f; break; // Red Flower
        case 12: r = 0.20f; g = 0.40f; b = 1.00f; break; // Blue Flower
        case 13: r = 0.10f; g = 0.50f; b = 0.10f; break; // Tall Grass
        case 19: r = 0.90f; g = 0.90f; b = 0.85f; break; // Birch Wood
        case 20: r = 0.30f; g = 0.70f; b = 0.30f; break; // Birch Leaves
        case 21: r = 1.00f; g = 0.95f; b = 0.95f; break; // Cherry Wood
        case 22: r = 1.00f; g = 0.60f; b = 0.80f; break; // Cherry Leaves
        case 23: r = 0.50f; g = 0.50f; b = 0.50f; break; // Cobblestone
        case 24: r = 0.40f; g = 0.55f; b = 0.40f; break; // Mossy Stone
        case 25: r = 0.70f; g = 0.55f; b = 0.35f; break; // Oak Planks
        case 26: r = 0.70f; g = 0.40f; b = 0.35f; break; // Bricks
        case 27: r = 0.80f; g = 0.90f; b = 1.00f; break; // Ice
        default: r = 1.0f; g = 0.0f; b = 1.0f; break; // Error Pink
    }

    static const float corners[6][4][3] = {
        { {1,0,0},{1,1,0},{1,1,1},{1,0,1} }, // +X
        { {0,0,1},{0,1,1},{0,1,0},{0,0,0} }, // -X
        { {0,1,1},{1,1,1},{1,1,0},{0,1,0} }, // +Y
        { {0,0,0},{1,0,0},{1,0,1},{0,0,1} }, // -Y
        { {1,0,1},{1,1,1},{0,1,1},{0,0,1} }, // +Z
        { {0,0,0},{0,1,0},{1,1,0},{1,0,0} }, // -Z
    };

    const float nx = normals[face][0];
    const float ny = normals[face][1];
    const float nz = normals[face][2];

    // Improved AO calculation for all faces
    float aoValues[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    auto checkAO = [&](int dx, int dy, int dz) {
        return chunk.get(x + dx, y + dy, z + dz) != 0 ? 0.15f : 0.0f;
    };

    if (face == 0) { // +X
        aoValues[0] = 1.0f - (checkAO(1, -1, 0) + checkAO(1, 0, -1) + checkAO(1, -1, -1));
        aoValues[1] = 1.0f - (checkAO(1, 1, 0) + checkAO(1, 0, -1) + checkAO(1, 1, -1));
        aoValues[2] = 1.0f - (checkAO(1, 1, 0) + checkAO(1, 0, 1) + checkAO(1, 1, 1));
        aoValues[3] = 1.0f - (checkAO(1, -1, 0) + checkAO(1, 0, 1) + checkAO(1, -1, 1));
    } else if (face == 1) { // -X
        aoValues[0] = 1.0f - (checkAO(-1, -1, 0) + checkAO(-1, 0, 1) + checkAO(-1, -1, 1));
        aoValues[1] = 1.0f - (checkAO(-1, 1, 0) + checkAO(-1, 0, 1) + checkAO(-1, 1, 1));
        aoValues[2] = 1.0f - (checkAO(-1, 1, 0) + checkAO(-1, 0, -1) + checkAO(-1, 1, -1));
        aoValues[3] = 1.0f - (checkAO(-1, -1, 0) + checkAO(-1, 0, -1) + checkAO(-1, -1, -1));
    } else if (face == 2) { // +Y
        aoValues[0] = 1.0f - (checkAO(-1, 1, 0) + checkAO(0, 1, 1) + checkAO(-1, 1, 1));
        aoValues[1] = 1.0f - (checkAO(1, 1, 0) + checkAO(0, 1, 1) + checkAO(1, 1, 1));
        aoValues[2] = 1.0f - (checkAO(1, 1, 0) + checkAO(0, 1, -1) + checkAO(1, 1, -1));
        aoValues[3] = 1.0f - (checkAO(-1, 1, 0) + checkAO(0, 1, -1) + checkAO(-1, 1, -1));
    } else if (face == 3) { // -Y
        aoValues[0] = 1.0f - (checkAO(-1, -1, 0) + checkAO(0, -1, -1) + checkAO(-1, -1, -1));
        aoValues[1] = 1.0f - (checkAO(1, -1, 0) + checkAO(0, -1, -1) + checkAO(1, -1, -1));
        aoValues[2] = 1.0f - (checkAO(1, -1, 0) + checkAO(0, -1, 1) + checkAO(1, -1, 1));
        aoValues[3] = 1.0f - (checkAO(-1, -1, 0) + checkAO(0, -1, 1) + checkAO(-1, -1, 1));
    } else if (face == 4) { // +Z
        aoValues[0] = 1.0f - (checkAO(0, -1, 1) + checkAO(1, 0, 1) + checkAO(1, -1, 1));
        aoValues[1] = 1.0f - (checkAO(0, 1, 1) + checkAO(1, 0, 1) + checkAO(1, 1, 1));
        aoValues[2] = 1.0f - (checkAO(0, 1, 1) + checkAO(-1, 0, 1) + checkAO(-1, 1, 1));
        aoValues[3] = 1.0f - (checkAO(0, -1, 1) + checkAO(-1, 0, 1) + checkAO(-1, -1, 1));
    } else if (face == 5) { // -Z
        aoValues[0] = 1.0f - (checkAO(0, -1, -1) + checkAO(-1, 0, -1) + checkAO(-1, -1, -1));
        aoValues[1] = 1.0f - (checkAO(0, 1, -1) + checkAO(-1, 0, -1) + checkAO(-1, 1, -1));
        aoValues[2] = 1.0f - (checkAO(0, 1, -1) + checkAO(1, 0, -1) + checkAO(1, 1, -1));
        aoValues[3] = 1.0f - (checkAO(0, -1, -1) + checkAO(1, 0, -1) + checkAO(1, -1, -1));
    }

    auto emit = [&](int i) {
        const float px = (float)x + corners[face][i][0];
        const float py = (float)y + corners[face][i][1];
        const float pz = (float)z + corners[face][i][2];
        float s = std::clamp(aoValues[i], 0.5f, 1.0f);

        // UV calculation
        float tx = 0, ty = 0;
        switch(type) {
            case 1: tx = 0; ty = 0; break; // Dirt
            case 2: // Grass
                if (face == 2) { tx = 1; ty = 0; } // Top
                else if (face == 3) { tx = 0; ty = 0; } // Bottom (Dirt)
                else { tx = 2; ty = 0; } // Side
                break;
            case 3: tx = 3; ty = 0; break; // Stone
            case 4: tx = 4; ty = 0; break; // Water
            case 5: tx = 5; ty = 0; break; // Lava
            case 6: tx = 6; ty = 0; break; // Wood
            case 7: tx = 7; ty = 0; break; // Leaves
            case 8: tx = 8; ty = 0; break; // Sand
            case 9: tx = 9; ty = 0; break; // Snow
            case 10: tx = 10; ty = 0; break; // Bedrock
            case 11: tx = 11; ty = 0; break; // Red Flower
            case 12: tx = 12; ty = 0; break; // Blue Flower
            case 13: tx = 13; ty = 0; break; // Tall Grass
            case 14: tx = 14; ty = 0; break; // Glass
            case 15: tx = 0; ty = 1; break; // Coal
            case 16: tx = 1; ty = 1; break; // Iron
            case 17: tx = 2; ty = 1; break; // Gold
            case 18: tx = 3; ty = 1; break; // Diamond
            case 19: tx = 4; ty = 1; break; // Birch Wood
            case 20: tx = 5; ty = 1; break; // Birch Leaves
            case 21: tx = 6; ty = 1; break; // Cherry Wood
            case 22: tx = 7; ty = 1; break; // Cherry Leaves
            case 23: tx = 8; ty = 1; break; // Cobblestone
            case 24: tx = 9; ty = 1; break; // Mossy Stone
            case 25: tx = 10; ty = 1; break; // Oak Planks
            case 26: tx = 11; ty = 1; break; // Bricks
            case 27: tx = 12; ty = 1; break; // Ice
        }

        static const float uvCoords[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };
        float u = (tx + uvCoords[i][0]) / 16.0f;
        float v = (ty + uvCoords[i][1]) / 16.0f;

        out.push_back(Vertex{px, py, pz, nx, ny, nz, r * s, g * s, b * s, u, v});
    };

    emit(0); emit(1); emit(2);
    emit(0); emit(2); emit(3);
}

std::vector<Vertex> MeshBuilder::buildGreedyMesh(const Chunk& chunk) {
    std::vector<Vertex> verts;
    verts.reserve(Chunk::SizeX * Chunk::SizeY * Chunk::SizeZ);

    for (int z = 0; z < Chunk::SizeZ; ++z) {
        for (int y = 0; y < Chunk::SizeY; ++y) {
            for (int x = 0; x < Chunk::SizeX; ++x) {
                uint8_t type = chunk.get(x, y, z);
                if (type == 0) continue;

                if (chunk.get(x + 1, y, z) == 0) addVoxelFace(verts, chunk, x, y, z, 0, type);
                if (chunk.get(x - 1, y, z) == 0) addVoxelFace(verts, chunk, x, y, z, 1, type);
                if (chunk.get(x, y + 1, z) == 0) addVoxelFace(verts, chunk, x, y, z, 2, type);
                if (chunk.get(x, y - 1, z) == 0) addVoxelFace(verts, chunk, x, y, z, 3, type);
                if (chunk.get(x, y, z + 1) == 0) addVoxelFace(verts, chunk, x, y, z, 4, type);
                if (chunk.get(x, y, z - 1) == 0) addVoxelFace(verts, chunk, x, y, z, 5, type);
            }
        }
    }
    return verts;
}

void MeshBuilder::addFace(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 normal, float u1, float v1, float u2, float v2, float r, float g, float b) {
    float light = 0.7f + std::abs(normal.y) * 0.3f;

    m_vertices.push_back(Vertex{p1.x, p1.y, p1.z, normal.x, normal.y, normal.z, r * light, g * light, b * light, u1, v1});
    m_vertices.push_back(Vertex{p2.x, p2.y, p2.z, normal.x, normal.y, normal.z, r * light, g * light, b * light, u2, v1});
    m_vertices.push_back(Vertex{p3.x, p3.y, p3.z, normal.x, normal.y, normal.z, r * light, g * light, b * light, u2, v2});

    m_vertices.push_back(Vertex{p1.x, p1.y, p1.z, normal.x, normal.y, normal.z, r * light, g * light, b * light, u1, v1});
    m_vertices.push_back(Vertex{p3.x, p3.y, p3.z, normal.x, normal.y, normal.z, r * light, g * light, b * light, u2, v2});
    m_vertices.push_back(Vertex{p4.x, p4.y, p4.z, normal.x, normal.y, normal.z, r * light, g * light, b * light, u1, v2});
}
