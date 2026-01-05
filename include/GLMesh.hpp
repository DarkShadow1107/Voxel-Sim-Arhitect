#pragma once

#include <cstddef>
#include <vector>

#include "MeshBuilder.hpp"

class GLMesh {
public:
    GLMesh() = default;
    ~GLMesh();

    // Prevent copying
    GLMesh(const GLMesh&) = delete;
    GLMesh& operator=(const GLMesh&) = delete;

    // Allow moving
    GLMesh(GLMesh&& other) noexcept;
    GLMesh& operator=(GLMesh&& other) noexcept;

    bool upload(const std::vector<Vertex>& vertices);
    void draw() const;
    void destroy();

    size_t vertexCount() const { return m_vertexCount; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    size_t m_vertexCount = 0;
};
