#pragma once

#include <string>

#include "Math.hpp"

class Shader {
public:
    ~Shader();

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
    void destroy();

    void use() const;

    void setMat4(const char* name, const Mat4& m) const;
    void setVec3(const char* name, const Vec3& v) const;
    void setVec2(const char* name, const Vec2& v) const;
    void setFloat(const char* name, float v) const;
    void setInt(const char* name, int v) const;

private:
    unsigned int m_program = 0;

    static std::string readTextFile(const std::string& path);
    static unsigned int compile(unsigned int type, const std::string& src);
};
