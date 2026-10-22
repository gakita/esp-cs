#include "math/Angles.hpp"

#include <cmath>

namespace vanta::math {

float normalize_angle(float deg) {
    deg = std::fmod(deg + 180.0f, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg - 180.0f;
}

Angles normalize(Angles a) {
    a.yaw   = normalize_angle(a.yaw);
    a.roll  = normalize_angle(a.roll);
    if (a.pitch >  89.0f) a.pitch =  89.0f;
    if (a.pitch < -89.0f) a.pitch = -89.0f;
    return a;
}

Angles calc_angle(Vec3 from, Vec3 to) {
    Vec3 d = to - from;
    float hyp = std::sqrt(d.x * d.x + d.y * d.y);
    Angles a;
    a.pitch = -std::atan2(d.z, hyp) * kRadDeg;
    a.yaw   =  std::atan2(d.y, d.x) * kRadDeg;
    a.roll  = 0.0f;
    return normalize(a);
}

Angles delta(Angles src, Angles dst) {
    Angles d{dst.pitch - src.pitch,
             dst.yaw   - src.yaw,
             dst.roll  - src.roll};
    d.pitch = normalize_angle(d.pitch);
    d.yaw   = normalize_angle(d.yaw);
    d.roll  = normalize_angle(d.roll);
    return d;
}

float angle_between(Angles a, Angles b) {
    Angles d = delta(a, b);
    return std::sqrt(d.pitch * d.pitch + d.yaw * d.yaw);
}

MouseDelta degrees_to_mouse(float d_pitch_deg, float d_yaw_deg,
                            float sensitivity, float m_yaw, float m_pitch) {
    // In Source-family engines: 1 raw count = sensitivity * m_yaw deg of yaw.
    // Reverse: counts = deg / (sens * m_*). Yaw-left in Source is positive,
    // but mouse X right is positive -> invert the sign on yaw.
    const float denom_yaw   = sensitivity * m_yaw;
    const float denom_pitch = sensitivity * m_pitch;
    MouseDelta out{};
    if (denom_yaw   != 0.0f) out.dx = static_cast<long>(std::lround(-d_yaw_deg   / denom_yaw));
    if (denom_pitch != 0.0f) out.dy = static_cast<long>(std::lround( d_pitch_deg / denom_pitch));
    return out;
}

} // namespace vanta::math
