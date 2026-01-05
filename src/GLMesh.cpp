#include "GLMesh.hpp"

#include <glad/gl.h>

GLMesh::~GLMesh() {
    destroy();
}

GLMesh::GLMesh(GLMesh&& other) noexcept 
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_vertexCount(other.m_vertexCount) 
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_vertexCount = 0;
}

GLMesh& GLMesh::operator=(GLMesh&& other) noexcept {
    if (this != &other) {
        destroy();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_vertexCount = other.m_vertexCount;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_vertexCount = 0;
    }
    return *this;
}

bool GLMesh::upload(const std::vector<Vertex>& vertices) {
    destroy();

    if (vertices.empty()) {
        m_vertexCount = 0;
        return true;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    // location 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // location 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

    // location 2: color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));

    // location 3: uv
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(9 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_vertexCount = vertices.size();
    return true;
}

void GLMesh::draw() const {
    if (m_vao == 0 || m_vertexCount == 0) {
        return;
    }

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}

void GLMesh::destroy() {
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_vertexCount = 0;
}
