#include "Camera.hpp"

#include <algorithm>

void Camera::addYawPitch(float deltaYawDeg, float deltaPitchDeg) {
    m_yawDeg += deltaYawDeg;
    m_pitchDeg += deltaPitchDeg;
    m_pitchDeg = std::clamp(m_pitchDeg, -89.0f, 89.0f);
}

Vec3 Camera::forward() const {
    const float yaw = radians(m_yawDeg);
    const float pitch = radians(m_pitchDeg);

    Vec3 f;
    f.x = std::cos(yaw) * std::cos(pitch);
    f.y = std::sin(pitch);
    f.z = std::sin(yaw) * std::cos(pitch);
    return normalize(f);
}

Vec3 Camera::right() const {
    return normalize(cross(forward(), {0.0f, 1.0f, 0.0f}));
}

void Camera::moveLocal(float forwardMove, float rightMove, float upMove) {
    const Vec3 f = forward();
    const Vec3 r = right();
    const Vec3 u{0.0f, 1.0f, 0.0f};

    m_position = m_position + f * forwardMove + r * rightMove + u * upMove;
}

Mat4 Camera::viewMatrix() const {
    return lookAt(m_position, m_position + forward(), {0.0f, 1.0f, 0.0f});
}

Mat4 Camera::projectionMatrix() const {
    return perspective(radians(m_fovDeg), m_aspect, m_nearZ, m_farZ);
}
