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

struct Plane {
    float a, b, c, d;
    
    void normalize() {
        float mag = std::sqrt(a * a + b * b + c * c);
        a /= mag; b /= mag; c /= mag; d /= mag;
    }
    
    float distance(const Vec3& p) const {
        return a * p.x + b * p.y + c * p.z + d;
    }
};

struct AABB {
    Vec3 min;
    Vec3 max;

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
    
    bool contains(const Vec3& p) const {
        return (p.x >= min.x && p.x <= max.x) &&
               (p.y >= min.y && p.y <= max.y) &&
               (p.z >= min.z && p.z <= max.z);
    }

    Vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    Vec3 getExtents() const {
        return (max - min) * 0.5f;
    }
};

struct Frustum {
    Plane planes[6];
    
    void update(const Mat4& vp) {
        // Left
        planes[0].a = vp.m[3] + vp.m[0];
        planes[0].b = vp.m[7] + vp.m[4];
        planes[0].c = vp.m[11] + vp.m[8];
        planes[0].d = vp.m[15] + vp.m[12];
        
        // Right
        planes[1].a = vp.m[3] - vp.m[0];
        planes[1].b = vp.m[7] - vp.m[4];
        planes[1].c = vp.m[11] - vp.m[8];
        planes[1].d = vp.m[15] - vp.m[12];
        
        // Bottom
        planes[2].a = vp.m[3] + vp.m[1];
        planes[2].b = vp.m[7] + vp.m[5];
        planes[2].c = vp.m[11] + vp.m[9];
        planes[2].d = vp.m[15] + vp.m[13];
        
        // Top
        planes[3].a = vp.m[3] - vp.m[1];
        planes[3].b = vp.m[7] - vp.m[5];
        planes[3].c = vp.m[11] - vp.m[9];
        planes[3].d = vp.m[15] - vp.m[13];
        
        // Near
        planes[4].a = vp.m[3] + vp.m[2];
        planes[4].b = vp.m[7] + vp.m[6];
        planes[4].c = vp.m[11] + vp.m[10];
        planes[4].d = vp.m[15] + vp.m[14];
        
        // Far
        planes[5].a = vp.m[3] - vp.m[2];
        planes[5].b = vp.m[7] - vp.m[6];
        planes[5].c = vp.m[11] - vp.m[10];
        planes[5].d = vp.m[15] - vp.m[14];
        
        for(int i=0; i<6; ++i) planes[i].normalize();
    }
    
    bool testPoint(const Vec3& p) const {
        for(int i=0; i<6; ++i) {
            if(planes[i].distance(p) < 0) return false;
        }
        return true;
    }
    
    bool testSphere(const Vec3& center, float radius) const {
        for(int i=0; i<6; ++i) {
            if(planes[i].distance(center) < -radius) return false;
        }
        return true;
    }

    bool testAABB(const AABB& box) const {
        for (int i = 0; i < 6; i++) {
            Vec3 p = box.min;
            if (planes[i].a >= 0) p.x = box.max.x;
            if (planes[i].b >= 0) p.y = box.max.y;
            if (planes[i].c >= 0) p.z = box.max.z;

            if (planes[i].distance(p) < 0) {
                return false;
            }
        }
        return true;
    }
};
