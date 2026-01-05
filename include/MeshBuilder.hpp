#pragma once
#include <vector>
#include <cstdint>
#include "Math.hpp"

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};

class MeshBuilder {
public:
    static std::vector<Vertex> buildGreedyMesh(const class Chunk& chunk);

    void addFace(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n, float u1, float v1, float u2, float v2, float ao);
    const std::vector<Vertex>& getVertices() const { return m_vertices; }

private:
    std::vector<Vertex> m_vertices;
};
