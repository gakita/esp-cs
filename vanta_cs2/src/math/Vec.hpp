#pragma once
#include <cmath>
#include <cstddef>

namespace vanta::math {

struct Vec2 {
    float x{0}, y{0};
    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    float length() const { return std::sqrt(x * x + y * y); }
};

struct Vec3 {
    float x{0}, y{0}, z{0};
    constexpr Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    static float distance(Vec3 a, Vec3 b) { return (a - b).length(); }
};

// Row-major 4x4 in the layout used by Source 2's VMatrix.
struct Mat4 {
    float m[4][4] = {};
    float& operator()(std::size_t r, std::size_t c) { return m[r][c]; }
    float  operator()(std::size_t r, std::size_t c) const { return m[r][c]; }
};

} // namespace vanta::math
