#pragma once

#include "Math.hpp"

class Camera {
public:
    void setAspect(float aspect) { m_aspect = aspect; }

    void setPosition(const Vec3& p) { m_position = p; }
    const Vec3& position() const { return m_position; }

    void addYawPitch(float deltaYawDeg, float deltaPitchDeg);
    void moveLocal(float forward, float right, float up);

    Mat4 viewMatrix() const;
    Mat4 projectionMatrix() const;

    Vec3 forward() const;
    Vec3 right() const;

    float yawDegrees() const { return m_yawDeg; }
    float pitchDegrees() const { return m_pitchDeg; }

    float fovDegrees() const { return m_fovDeg; }
    void setFovDegrees(float fov) { m_fovDeg = fov; }

private:
    Vec3 m_position{0.0f, 10.0f, 20.0f};
    float m_yawDeg = -90.0f;   // looking toward -Z
    float m_pitchDeg = -15.0f;

    float m_fovDeg = 70.0f;
    float m_aspect = 16.0f / 9.0f;
    float m_nearZ = 0.05f;
    float m_farZ = 500.0f;
};
