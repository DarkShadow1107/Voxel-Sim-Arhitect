#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Math.hpp"
#include <cmath>

using namespace Catch::Matchers;
static constexpr float kPi = 3.14159265358979323846f;

TEST_CASE("Math Vector Operations", "[math]") {
    SECTION("Vector Addition") {
        Vec3 a = {1.0f, 2.0f, 3.0f};
        Vec3 b = {4.0f, 5.0f, 6.0f};
        Vec3 c = a + b;
        REQUIRE_THAT(c.x, WithinAbs(5.0f, 0.001f));
        REQUIRE_THAT(c.y, WithinAbs(7.0f, 0.001f));
        REQUIRE_THAT(c.z, WithinAbs(9.0f, 0.001f));
    }

    SECTION("Vector Length and Normalization") {
        Vec3 a = {3.0f, 0.0f, 4.0f};
        float len = length(a);
        REQUIRE_THAT(len, WithinAbs(5.0f, 0.001f));

        Vec3 n = normalize(a);
        float n_len = length(n);
        REQUIRE_THAT(n.x, WithinAbs(0.6f, 0.001f));
        REQUIRE_THAT(n.z, WithinAbs(0.8f, 0.001f));
        REQUIRE_THAT(n_len, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Dot Product") {
        Vec3 a = {1.0f, 0.0f, 0.0f};
        Vec3 b = {0.0f, 1.0f, 0.0f};
        REQUIRE_THAT(dot(a, b), WithinAbs(0.0f, 0.001f));
        
        Vec3 c = {2.0f, 3.0f, 4.0f};
        Vec3 d = {1.0f, 2.0f, 3.0f};
        REQUIRE_THAT(dot(c, d), WithinAbs(20.0f, 0.001f));
    }

    SECTION("Cross Product") {
        Vec3 a = {1.0f, 0.0f, 0.0f};
        Vec3 b = {0.0f, 1.0f, 0.0f};
        Vec3 c = cross(a, b);
        REQUIRE_THAT(c.x, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(c.y, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(c.z, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Vector Unary and Scalar Multiplication") {
        Vec3 a = {1.0f, -2.0f, 3.0f};
        Vec3 n_a = -a;
        REQUIRE_THAT(n_a.x, WithinAbs(-1.0f, 0.001f));
        REQUIRE_THAT(n_a.y, WithinAbs(2.0f, 0.001f));
        REQUIRE_THAT(n_a.z, WithinAbs(-3.0f, 0.001f));

        Vec3 s_a = a * 2.5f;
        REQUIRE_THAT(s_a.x, WithinAbs(2.5f, 0.001f));
        REQUIRE_THAT(s_a.y, WithinAbs(-5.0f, 0.001f));
        REQUIRE_THAT(s_a.z, WithinAbs(7.5f, 0.001f));
    }

    SECTION("Vec2 and Vec4 Constructors") {
        Vec2 v2(1.0f, 2.0f);
        REQUIRE_THAT(v2.x, WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(v2.y, WithinAbs(2.0f, 0.001f));

        Vec4 v4(1.0f, 2.0f, 3.0f, 4.0f);
        REQUIRE_THAT(v4.x, WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(v4.w, WithinAbs(4.0f, 0.001f));
    }
}

TEST_CASE("Math Matrix Operations", "[math]") {
    SECTION("Identity Matrix") {
        Mat4 id = Mat4::identity();
        REQUIRE_THAT(id.m[0], WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(id.m[1], WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(id.m[5], WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(id.m[15], WithinAbs(1.0f, 0.001f));
    }

    SECTION("Matrix Multiplication") {
        Mat4 a = Mat4::identity();
        a.m[12] = 5.0f; // Translate X

        Mat4 b = Mat4::identity();
        b.m[13] = 3.0f; // Translate Y

        Mat4 c = a * b; // Translation composition
        REQUIRE_THAT(c.m[12], WithinAbs(5.0f, 0.001f));
        REQUIRE_THAT(c.m[13], WithinAbs(3.0f, 0.001f));
        REQUIRE_THAT(c.m[0], WithinAbs(1.0f, 0.001f));
    }

    SECTION("Matrix-Vector Multiplication") {
        Mat4 trans = translate(Vec3(10.0f, -5.0f, 2.0f));
        Vec4 point(0.0f, 0.0f, 0.0f, 1.0f);
        
        Vec4 result = trans * point;
        REQUIRE_THAT(result.x, WithinAbs(10.0f, 0.001f));
        REQUIRE_THAT(result.y, WithinAbs(-5.0f, 0.001f));
        REQUIRE_THAT(result.z, WithinAbs(2.0f, 0.001f));
        REQUIRE_THAT(result.w, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Scale Matrix") {
        Mat4 s = scale(Vec3(2.0f, 3.0f, 4.0f));
        Vec4 point(1.0f, 1.0f, 1.0f, 1.0f);

        Vec4 result = s * point;
        REQUIRE_THAT(result.x, WithinAbs(2.0f, 0.001f));
        REQUIRE_THAT(result.y, WithinAbs(3.0f, 0.001f));
        REQUIRE_THAT(result.z, WithinAbs(4.0f, 0.001f));
    }

    SECTION("LookAt Matrix") {
        Vec3 eye(0.0f, 0.0f, 5.0f);
        Vec3 center(0.0f, 0.0f, 0.0f);
        Vec3 up(0.0f, 1.0f, 0.0f);

        Mat4 view = lookAt(eye, center, up);
        Vec4 point(0.0f, 0.0f, 0.0f, 1.0f);
        Vec4 result = view * point;

        // Position should be transformed to (0, 0, -5) since camera is at Z=5 looking down -Z
        REQUIRE_THAT(result.x, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(result.z, WithinAbs(-5.0f, 0.001f));
    }

    SECTION("Perspective Matrix — element validation") {
        Mat4 proj = perspective(radians(90.0f), 1.0f, 0.1f, 100.0f);
        REQUIRE_THAT(proj.m[0],  WithinAbs(1.0f, 0.001f));  // f / aspect = 1
        REQUIRE_THAT(proj.m[5],  WithinAbs(1.0f, 0.001f));  // f = 1
        REQUIRE_THAT(proj.m[11], WithinAbs(-1.0f, 0.001f)); // perspective divide row
        REQUIRE_THAT(proj.m[3],  WithinAbs(0.0f,  0.001f));
        REQUIRE_THAT(proj.m[7],  WithinAbs(0.0f,  0.001f));
        REQUIRE_THAT(proj.m[15], WithinAbs(0.0f,  0.001f));
    }

    SECTION("RotateX — 90 degrees transforms Y -> Z") {
        Mat4 rx = rotateX(radians(90.0f));
        Vec4 yAxis(0.0f, 1.0f, 0.0f, 1.0f);
        Vec4 r = rx * yAxis;
        REQUIRE_THAT(r.x, WithinAbs(0.0f,  0.001f));
        REQUIRE_THAT(r.y, WithinAbs(0.0f,  0.001f));
        REQUIRE_THAT(r.z, WithinAbs(1.0f,  0.001f));
    }

    SECTION("RotateY — 90 degrees transforms Z -> X") {
        Mat4 ry = rotateY(radians(90.0f));
        Vec4 zAxis(0.0f, 0.0f, 1.0f, 1.0f);
        Vec4 r = ry * zAxis;
        REQUIRE_THAT(r.x, WithinAbs(1.0f,  0.001f));
        REQUIRE_THAT(r.y, WithinAbs(0.0f,  0.001f));
        REQUIRE_THAT(r.z, WithinAbs(0.0f,  0.001f));
    }

    SECTION("RotateZ — 90 degrees transforms X -> Y") {
        Mat4 rz = rotateZ(radians(90.0f));
        Vec4 xAxis(1.0f, 0.0f, 0.0f, 1.0f);
        Vec4 r = rz * xAxis;
        REQUIRE_THAT(r.x, WithinAbs(0.0f,  0.001f));
        REQUIRE_THAT(r.y, WithinAbs(1.0f,  0.001f));
        REQUIRE_THAT(r.z, WithinAbs(0.0f,  0.001f));
    }

    SECTION("TRS composition: translate * rotate * scale") {
        // Build a TRS: scale(2) * rotateY(90) * translate(5,0,0)
        // Then transform origin: should yield the translation scaled.
        Mat4 T = translate(Vec3(5.0f, 0.0f, 0.0f));
        Mat4 R = rotateY(radians(90.0f));
        Mat4 S = scale(Vec3(2.0f, 2.0f, 2.0f));
        Mat4 TRS = T * R * S;
        // Point at (1, 0, 0) in model space
        Vec4 p(1.0f, 0.0f, 0.0f, 1.0f);
        Vec4 result = TRS * p;
        // Scale: (2,0,0) -> RotY90: (0,0,-2) -> Translate: (5,0,-2)
        REQUIRE_THAT(result.x, WithinAbs(5.0f, 0.01f));
        REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(result.z, WithinAbs(-2.0f, 0.01f));
    }

    SECTION("Matrix identity is a no-op on any vector") {
        Mat4 id = Mat4::identity();
        Vec4 v(7.3f, -1.4f, 0.5f, 1.0f);
        Vec4 r = id * v;
        REQUIRE_THAT(r.x, WithinAbs(v.x, 0.001f));
        REQUIRE_THAT(r.y, WithinAbs(v.y, 0.001f));
        REQUIRE_THAT(r.z, WithinAbs(v.z, 0.001f));
        REQUIRE_THAT(r.w, WithinAbs(v.w, 0.001f));
    }

    SECTION("Mat4 * identity = original, identity * Mat4 = original") {
        Mat4 A = translate(Vec3(3.0f, 7.0f, -2.0f));
        Mat4 id = Mat4::identity();
        Mat4 AI = A * id;
        Mat4 IA = id * A;
        for (int i = 0; i < 16; ++i) {
            REQUIRE_THAT(AI.m[i], WithinAbs(A.m[i], 0.001f));
            REQUIRE_THAT(IA.m[i], WithinAbs(A.m[i], 0.001f));
        }
    }
}

// =============================================================================
//  Scalar / Utility
// =============================================================================

TEST_CASE("Math: radians() and degrees round-trip", "[math][scalar]") {
    REQUIRE_THAT(radians(0.0f),   WithinAbs(0.0f,      0.00001f));
    REQUIRE_THAT(radians(90.0f),  WithinAbs(kPi*0.5f,  0.00001f));
    REQUIRE_THAT(radians(180.0f), WithinAbs(kPi,        0.00001f));
    REQUIRE_THAT(radians(360.0f), WithinAbs(2.0f * kPi, 0.00001f));
    REQUIRE_THAT(radians(-90.0f), WithinAbs(-kPi*0.5f,  0.00001f));
}

TEST_CASE("Math: normalize of zero vector returns zero (no NaN/crash)", "[math][safety]") {
    Vec3 zero{0.0f, 0.0f, 0.0f};
    Vec3 n = normalize(zero);
    // Must not contain NaN
    REQUIRE(n.x == n.x); // NaN != NaN
    REQUIRE(n.y == n.y);
    REQUIRE(n.z == n.z);
    // Length of result should be 0 (the fallback Vec3{})
    REQUIRE_THAT(length(n), WithinAbs(0.0f, 0.001f));
}

TEST_CASE("Math: Vec3 arithmetic operators", "[math][vector]") {
    Vec3 a{6.0f, 4.0f, 2.0f};
    Vec3 b{1.0f, 2.0f, 3.0f};

    SECTION("Subtraction") {
        Vec3 r = a - b;
        REQUIRE_THAT(r.x, WithinAbs(5.0f,  0.001f));
        REQUIRE_THAT(r.y, WithinAbs(2.0f,  0.001f));
        REQUIRE_THAT(r.z, WithinAbs(-1.0f, 0.001f));
    }
    SECTION("Division by scalar") {
        Vec3 r = a / 2.0f;
        REQUIRE_THAT(r.x, WithinAbs(3.0f, 0.001f));
        REQUIRE_THAT(r.y, WithinAbs(2.0f, 0.001f));
        REQUIRE_THAT(r.z, WithinAbs(1.0f, 0.001f));
    }
    SECTION("Component-wise multiply") {
        Vec3 r = a * b;
        REQUIRE_THAT(r.x, WithinAbs(6.0f, 0.001f));
        REQUIRE_THAT(r.y, WithinAbs(8.0f, 0.001f));
        REQUIRE_THAT(r.z, WithinAbs(6.0f, 0.001f));
    }
    SECTION("Unary negation") {
        Vec3 n = -a;
        REQUIRE_THAT(n.x, WithinAbs(-6.0f, 0.001f));
        REQUIRE_THAT(n.y, WithinAbs(-4.0f, 0.001f));
        REQUIRE_THAT(n.z, WithinAbs(-2.0f, 0.001f));
    }
}

TEST_CASE("Math: Dot product — orthogonality and self-dot", "[math][vector]") {
    SECTION("Orthogonal vectors have dot = 0") {
        Vec3 x{1,0,0}, y{0,1,0}, z{0,0,1};
        REQUIRE_THAT(dot(x,y), WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(dot(y,z), WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(dot(x,z), WithinAbs(0.0f, 0.001f));
    }
    SECTION("Self dot = squared length") {
        Vec3 v{3.0f, 4.0f, 0.0f};
        REQUIRE_THAT(dot(v,v), WithinAbs(25.0f, 0.001f)); // 3^2+4^2=25
    }
    SECTION("Parallel vectors have maximum dot") {
        Vec3 a = normalize(Vec3{1.0f, 2.0f, 3.0f});
        REQUIRE_THAT(dot(a,a), WithinAbs(1.0f, 0.001f));
    }
}

TEST_CASE("Math: Cross product — right-hand rule and anti-commutativity", "[math][vector]") {
    Vec3 x{1,0,0}, y{0,1,0}, z{0,0,1};
    SECTION("x cross y = z") {
        Vec3 r = cross(x, y);
        REQUIRE_THAT(r.z, WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(r.x, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(r.y, WithinAbs(0.0f, 0.001f));
    }
    SECTION("y cross x = -z (anti-commutativity)") {
        Vec3 r = cross(y, x);
        REQUIRE_THAT(r.z, WithinAbs(-1.0f, 0.001f));
    }
    SECTION("Parallel vectors cross = zero") {
        Vec3 a{1,2,3};
        Vec3 r = cross(a, a);
        REQUIRE_THAT(length(r), WithinAbs(0.0f, 0.001f));
    }
    SECTION("Cross result is perpendicular to both operands") {
        Vec3 a{1,2,3}, b{4,5,6};
        Vec3 c = cross(a,b);
        REQUIRE_THAT(dot(c,a), WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(dot(c,b), WithinAbs(0.0f, 0.001f));
    }
}
