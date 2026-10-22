#pragma once
#include "math/Vec.hpp"

namespace vanta::math {

constexpr float kPi      = 3.14159265358979323846f;
constexpr float kDegRad  = kPi / 180.0f;
constexpr float kRadDeg  = 180.0f / kPi;

// Source engine Euler: pitch (x, -89..89), yaw (y, -180..180), roll (z).
struct Angles {
    float pitch{0}, yaw{0}, roll{0};
};

// Returns the view angles (pitch, yaw) that point from `from` to `to`.
Angles calc_angle(Vec3 from, Vec3 to);

// Normalizes a single angle component into [-180, 180].
float normalize_angle(float deg);

// Clamps pitch to [-89, 89] and normalizes yaw.
Angles normalize(Angles a);

// Shortest-arc delta from `src` to `dst`, each component normalized.
Angles delta(Angles src, Angles dst);

// Angular distance between two view vectors in degrees.
float angle_between(Angles a, Angles b);

// Converts a mouse-delta in degrees into a raw mouse count given the game's
// sensitivity and the player's m_yaw/m_pitch cvars (typical defaults 0.022).
// Returns integer deltas suitable for SendInput MOUSEEVENTF_MOVE.
struct MouseDelta { long dx{0}, dy{0}; };
MouseDelta degrees_to_mouse(float d_pitch_deg, float d_yaw_deg,
                            float sensitivity,
                            float m_yaw = 0.022f, float m_pitch = 0.022f);

} // namespace vanta::math
