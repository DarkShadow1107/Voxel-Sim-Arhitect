#pragma once

#include <cmath>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
    Vec4() = default;
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(const Vec3& v) { return {-v.x, -v.y, -v.z}; }
inline Vec3 operator*(const Vec3& v, float s) { return {v.x * s, v.y * s, v.z * s}; }
inline Vec3 operator/(const Vec3& v, float s) { return {v.x / s, v.y / s, v.z / s}; }

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}
inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }
inline Vec3 normalize(const Vec3& v) {
    const float len = length(v);
    return (len > 0.0f) ? (v / len) : Vec3{};
}
inline float radians(float degrees) { return degrees * 0.01745329251994329577f; }

// Column-major 4x4 (OpenGL-friendly)
struct Mat4 {
    float m[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    static Mat4 identity() { return Mat4{}; }
};

inline Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            r.m[c * 4 + row] =
                a.m[0 * 4 + row] * b.m[c * 4 + 0] +
                a.m[1 * 4 + row] * b.m[c * 4 + 1] +
                a.m[2 * 4 + row] * b.m[c * 4 + 2] +
                a.m[3 * 4 + row] * b.m[c * 4 + 3];
        }
    }
    return r;
}

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    return multiply(a, b);
}

inline Vec4 operator*(const Mat4& m, const Vec4& v) {
    return {
        m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w,
        m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w,
        m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w,
        m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w
    };
}

inline Mat4 translate(const Vec3& t) {
    Mat4 r = Mat4::identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

inline Mat4 rotateX(float angleRadians) {
    Mat4 r = Mat4::identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

inline Mat4 rotateY(float angleRadians) {
    Mat4 r = Mat4::identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

inline Mat4 rotateZ(float angleRadians) {
    Mat4 r = Mat4::identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}

inline Mat4 scale(const Vec3& s) {
    Mat4 r = Mat4::identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}

inline Mat4 perspective(float fovyRadians, float aspect, float zNear, float zFar) {
    Mat4 r{};
    const float f = 1.0f / std::tan(fovyRadians * 0.5f);

    r.m[0] = f / aspect;
    r.m[1] = 0.0f;
    r.m[2] = 0.0f;
    r.m[3] = 0.0f;

    r.m[4] = 0.0f;
    r.m[5] = f;
    r.m[6] = 0.0f;
    r.m[7] = 0.0f;

    r.m[8] = 0.0f;
    r.m[9] = 0.0f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;

    r.m[12] = 0.0f;
    r.m[13] = 0.0f;
    r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    r.m[15] = 0.0f;

    return r;
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 f = normalize(center - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 r = Mat4::identity();
    // Row 0 = s
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;

    // Row 1 = u
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;

    // Row 2 = -f
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;

    r.m[12] = -dot(s, eye);
    r.m[13] = -dot(u, eye);
    r.m[14] = dot(f, eye);

    return r;
}

struct Transform {
    Vec3 position;
};
