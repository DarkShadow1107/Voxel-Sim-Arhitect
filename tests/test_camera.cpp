#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Camera.hpp"
#include "Math.hpp"
#include <cmath>
#include <algorithm>

using namespace Catch::Matchers;
static constexpr float kEps = 0.002f;
static constexpr float kPi  = 3.14159265358979323846f;

// =============================================================================
//  DEFAULT STATE
// =============================================================================

TEST_CASE("Camera: Default position is (0, 10, 20)", "[camera][init]") {
    Camera cam;
    REQUIRE_THAT(cam.position().x, WithinAbs(0.0f,  kEps));
    REQUIRE_THAT(cam.position().y, WithinAbs(10.0f, kEps));
    REQUIRE_THAT(cam.position().z, WithinAbs(20.0f, kEps));
}

TEST_CASE("Camera: Default FOV is 70 degrees", "[camera][init]") {
    Camera cam;
    REQUIRE_THAT(cam.fovDegrees(), WithinAbs(70.0f, kEps));
}

TEST_CASE("Camera: Default forward vector has unit length", "[camera][init]") {
    Camera cam;
    Vec3 f = cam.forward();
    float len = length(f);
    INFO("Default forward length: " << len);
    REQUIRE_THAT(len, WithinAbs(1.0f, kEps));
}

TEST_CASE("Camera: Default right vector has unit length", "[camera][init]") {
    Camera cam;
    Vec3 r = cam.right();
    REQUIRE_THAT(length(r), WithinAbs(1.0f, kEps));
}

TEST_CASE("Camera: Forward and right are perpendicular", "[camera][init]") {
    Camera cam;
    float d = dot(cam.forward(), cam.right());
    INFO("dot(forward, right) = " << d);
    REQUIRE_THAT(d, WithinAbs(0.0f, kEps));
}

// =============================================================================
//  PITCH CLAMPING
// =============================================================================

TEST_CASE("Camera: Pitch clamps to +89 degrees at the top", "[camera][pitch]") {
    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);
    // Add a large upward rotation
    cam.addYawPitch(0.0f, 200.0f);
    INFO("Clamped pitch: " << cam.pitchDegrees());
    REQUIRE(cam.pitchDegrees() <= 89.0f + kEps);
}

TEST_CASE("Camera: Pitch clamps to -89 degrees at the bottom", "[camera][pitch]") {
    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);
    cam.addYawPitch(0.0f, -200.0f);
    INFO("Clamped pitch: " << cam.pitchDegrees());
    REQUIRE(cam.pitchDegrees() >= -89.0f - kEps);
}

TEST_CASE("Camera: Pitch accumulates correctly within safe range", "[camera][pitch]") {
    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);
    cam.addYawPitch(0.0f, 30.0f);
    REQUIRE_THAT(cam.pitchDegrees(), WithinAbs(30.0f, kEps));
    cam.addYawPitch(0.0f, 20.0f);
    REQUIRE_THAT(cam.pitchDegrees(), WithinAbs(50.0f, kEps));
}

TEST_CASE("Camera: Pitch clamping keeps forward Y component in valid range", "[camera][pitch]") {
    Camera cam;
    cam.setYawPitch(0.0f, 89.0f);
    float fy = cam.forward().y;
    INFO("forward.y at pitch=89: " << fy);
    REQUIRE(fy >= -1.0f);
    REQUIRE(fy <=  1.0f);
    REQUIRE_THAT(length(cam.forward()), WithinAbs(1.0f, kEps));

    cam.setYawPitch(0.0f, -89.0f);
    REQUIRE_THAT(length(cam.forward()), WithinAbs(1.0f, kEps));
}

// =============================================================================
//  YAW / DIRECTION ACCURACY
// =============================================================================

TEST_CASE("Camera: Yaw 0 degrees points along +X axis (pitch=0)", "[camera][yaw]") {
    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);
    Vec3 f = cam.forward();
    INFO("forward at yaw=0: (" << f.x << ", " << f.y << ", " << f.z << ")");
    REQUIRE_THAT(f.x, WithinAbs(1.0f, kEps)); // cos(0)*cos(0) = 1
    REQUIRE_THAT(f.y, WithinAbs(0.0f, kEps)); // sin(0) = 0
    REQUIRE_THAT(f.z, WithinAbs(0.0f, kEps)); // sin(0)*cos(0) = 0
}

TEST_CASE("Camera: Yaw 90 degrees points along +Z axis (pitch=0)", "[camera][yaw]") {
    Camera cam;
    cam.setYawPitch(90.0f, 0.0f);
    Vec3 f = cam.forward();
    INFO("forward at yaw=90: (" << f.x << ", " << f.y << ", " << f.z << ")");
    REQUIRE_THAT(f.x, WithinAbs(0.0f,  kEps));
    REQUIRE_THAT(f.y, WithinAbs(0.0f,  kEps));
    REQUIRE_THAT(f.z, WithinAbs(1.0f,  kEps));
}

TEST_CASE("Camera: Yaw 180 degrees points along -X axis (pitch=0)", "[camera][yaw]") {
    Camera cam;
    cam.setYawPitch(180.0f, 0.0f);
    Vec3 f = cam.forward();
    REQUIRE_THAT(f.x, WithinAbs(-1.0f, kEps));
    REQUIRE_THAT(f.y, WithinAbs( 0.0f, kEps));
    REQUIRE_THAT(f.z, WithinAbs( 0.0f, kEps));
}

TEST_CASE("Camera: Pitch 90 degrees points straight up", "[camera][yaw]") {
    Camera cam;
    cam.setYawPitch(0.0f, 89.0f); // clamped to 89
    Vec3 f = cam.forward();
    INFO("forward.y at pitch=89: " << f.y);
    // sin(89deg) ≈ 0.9998 — very close to 1
    REQUIRE(f.y > 0.99f);
}

TEST_CASE("Camera: Yaw accumulates addYawPitch", "[camera][yaw]") {
    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);
    cam.addYawPitch(45.0f, 0.0f);
    REQUIRE_THAT(cam.yawDegrees(), WithinAbs(45.0f, kEps));
    cam.addYawPitch(45.0f, 0.0f);
    REQUIRE_THAT(cam.yawDegrees(), WithinAbs(90.0f, kEps));
}

// =============================================================================
//  MOVEMENT  (moveLocal)
// =============================================================================

TEST_CASE("Camera: moveLocal forward moves along forward vector", "[camera][movement]") {
    Camera cam;
    cam.setPosition({0, 0, 0});
    cam.setYawPitch(0.0f, 0.0f); // looks along +X
    cam.moveLocal(5.0f, 0.0f, 0.0f);

    INFO("Position after forward 5: (" << cam.position().x << ", " << cam.position().y << ", " << cam.position().z << ")");
    REQUIRE_THAT(cam.position().x, WithinAbs(5.0f, kEps));
    REQUIRE_THAT(cam.position().y, WithinAbs(0.0f, kEps));
    REQUIRE_THAT(cam.position().z, WithinAbs(0.0f, kEps));
}

TEST_CASE("Camera: moveLocal right-strafe is perpendicular to forward", "[camera][movement]") {
    Camera cam;
    cam.setPosition({0, 0, 0});
    cam.setYawPitch(0.0f, 0.0f); // forward = +X, right = +Z
    cam.moveLocal(0.0f, 3.0f, 0.0f);

    // right of yaw=0 (forward +X) is cross(+X, world_up) → +Z
    INFO("Position after right 3: " << cam.position().x << ", " << cam.position().y << ", " << cam.position().z);
    REQUIRE_THAT(cam.position().y, WithinAbs(0.0f, kEps));
    // x should be ~0  (no forward move)
    REQUIRE_THAT(cam.position().x, WithinAbs(0.0f, kEps));
}

TEST_CASE("Camera: moveLocal up moves along world Y", "[camera][movement]") {
    Camera cam;
    cam.setPosition({0, 0, 0});
    cam.setYawPitch(0.0f, 0.0f);
    cam.moveLocal(0.0f, 0.0f, 10.0f);
    REQUIRE_THAT(cam.position().y, WithinAbs(10.0f, kEps));
}

TEST_CASE("Camera: moveLocal combination produces expected position", "[camera][movement]") {
    Camera cam;
    cam.setPosition({10.0f, 5.0f, 3.0f});
    cam.setYawPitch(0.0f, 0.0f); // forward = +X
    cam.moveLocal(2.0f, 0.0f, -1.0f); // forward 2, down 1

    INFO("Position: " << cam.position().x << ", " << cam.position().y << ", " << cam.position().z);
    REQUIRE_THAT(cam.position().x, WithinAbs(12.0f, kEps));
    REQUIRE_THAT(cam.position().y, WithinAbs(4.0f,  kEps));
}

// =============================================================================
//  VIEW MATRIX
// =============================================================================

TEST_CASE("Camera: viewMatrix is deterministic for same state", "[camera][matrix]") {
    Camera cam;
    cam.setPosition({5, 2, 3});
    cam.setYawPitch(45.0f, -15.0f);

    Mat4 v1 = cam.viewMatrix();
    Mat4 v2 = cam.viewMatrix();
    for (int i = 0; i < 16; ++i)
        REQUIRE_THAT(v1.m[i], WithinAbs(v2.m[i], 0.0001f));
}

TEST_CASE("Camera: viewMatrix changes when position changes", "[camera][matrix]") {
    Camera cam;
    cam.setPosition({0, 0, 0});
    cam.setYawPitch(0.0f, 0.0f);

    Mat4 before = cam.viewMatrix();
    cam.setPosition({100, 50, -30});
    Mat4 after = cam.viewMatrix();

    bool changed = false;
    for (int i = 0; i < 16; ++i)
        if (std::abs(before.m[i] - after.m[i]) > 0.001f) { changed = true; break; }
    REQUIRE(changed);
}

TEST_CASE("Camera: viewMatrix changes when orientation changes", "[camera][matrix]") {
    Camera cam;
    cam.setPosition({0, 0, 0});
    cam.setYawPitch(0.0f, 0.0f);
    Mat4 before = cam.viewMatrix();

    cam.addYawPitch(90.0f, 0.0f);
    Mat4 after = cam.viewMatrix();

    bool changed = false;
    for (int i = 0; i < 16; ++i)
        if (std::abs(before.m[i] - after.m[i]) > 0.001f) { changed = true; break; }
    REQUIRE(changed);
}

// =============================================================================
//  PROJECTION MATRIX
// =============================================================================

TEST_CASE("Camera: projectionMatrix changes when FOV changes", "[camera][matrix]") {
    Camera cam;
    cam.setAspect(16.0f / 9.0f);
    cam.setFovDegrees(70.0f);
    Mat4 p70 = cam.projectionMatrix();

    cam.setFovDegrees(45.0f);
    Mat4 p45 = cam.projectionMatrix();

    bool changed = false;
    for (int i = 0; i < 16; ++i)
        if (std::abs(p70.m[i] - p45.m[i]) > 0.001f) { changed = true; break; }
    REQUIRE(changed);
}

TEST_CASE("Camera: projectionMatrix has correct perspective divide element", "[camera][matrix]") {
    Camera cam;
    cam.setAspect(1.0f);
    cam.setFovDegrees(90.0f);
    Mat4 p = cam.projectionMatrix();
    // m[11] must be -1 for standard OpenGL perspective
    REQUIRE_THAT(p.m[11], WithinAbs(-1.0f, kEps));
    REQUIRE_THAT(p.m[15], WithinAbs( 0.0f, kEps));
}

TEST_CASE("Camera: projectionMatrix aspect ratio scales X element", "[camera][matrix]") {
    Camera cam;
    cam.setFovDegrees(90.0f);

    cam.setAspect(1.0f);
    float m0_aspect1 = cam.projectionMatrix().m[0];

    cam.setAspect(2.0f);
    float m0_aspect2 = cam.projectionMatrix().m[0];

    // Wider aspect → smaller x scaling factor
    INFO("m[0] at aspect 1: " << m0_aspect1 << "  at aspect 2: " << m0_aspect2);
    REQUIRE(m0_aspect2 < m0_aspect1);
}

TEST_CASE("Camera: setFovDegrees round-trips correctly", "[camera][matrix]") {
    Camera cam;
    cam.setFovDegrees(55.0f);
    REQUIRE_THAT(cam.fovDegrees(), WithinAbs(55.0f, kEps));
    cam.setFovDegrees(110.0f);
    REQUIRE_THAT(cam.fovDegrees(), WithinAbs(110.0f, kEps));
}
