#include "Shader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include <glad/gl.h>

Shader::~Shader() {
    destroy();
}

std::string Shader::readTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int Shader::compile(unsigned int type, const std::string& src) {
    const char* csrc = src.c_str();
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(shader, (GLsizei)sizeof(log), nullptr, log);
        std::cerr << "Shader compile failed: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    destroy();

    const std::string vs = readTextFile(vertexPath);
    const std::string fs = readTextFile(fragmentPath);
    if (vs.empty() || fs.empty()) {
        std::cerr << "Failed to read shader files: " << vertexPath << ", " << fragmentPath << std::endl;
        return false;
    }

    const unsigned int v = compile(GL_VERTEX_SHADER, vs);
    const unsigned int f = compile(GL_FRAGMENT_SHADER, fs);
    if (v == 0 || f == 0) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, v);
    glAttachShader(m_program, f);
    glLinkProgram(m_program);

    glDeleteShader(v);
    glDeleteShader(f);

    int ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetProgramInfoLog(m_program, (GLsizei)sizeof(log), nullptr, log);
        std::cerr << "Shader link failed: " << log << std::endl;
        destroy();
        return false;
    }

    return true;
}

void Shader::destroy() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::setMat4(const char* name, const Mat4& m) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, m.m);
    }
}

void Shader::setVec3(const char* name, const Vec3& v) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) {
        glUniform3f(loc, v.x, v.y, v.z);
    }
}

void Shader::setVec2(const char* name, const Vec2& v) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) {
        glUniform2f(loc, v.x, v.y);
    }
}

void Shader::setFloat(const char* name, float v) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) {
        glUniform1f(loc, v);
    }
}

void Shader::setInt(const char* name, int v) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) {
        glUniform1i(loc, v);
    }
}
