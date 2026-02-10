#include "MeshBuilder.hpp"
#include "Chunk.hpp"
#include "Registry.hpp"
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

    const auto& def = GameRegistry::getInstance().getBlock(type);
    float r = def.color.x, g = def.color.y, b = def.color.z;
    int tx = def.texX;
    int ty = def.texY;

    if (def.usePerFace && face >= 0 && face < 6) {
        r = def.faces[face].color.x;
        g = def.faces[face].color.y;
        b = def.faces[face].color.z;
        tx = def.faces[face].texX;
        ty = def.faces[face].texY;
    } else {
        // Specific overrides for grass top (fallback if not using per-face)
        if (type == BLOCK_GRASS && face == 2) {
            // green
        } else if (type == BLOCK_GRASS && face != 3) {
            r = 0.55f; g = 0.40f; b = 0.25f;
            tx = 0; ty = 0; // Dirt texture
        }
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

        static const float uvCoords[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };
        float u = ((float)tx + uvCoords[i][0]) / 16.0f;
        float v = ((float)ty + uvCoords[i][1]) / 16.0f;

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
