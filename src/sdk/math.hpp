#pragma once
#include <cmath>
#include <cstdint>

namespace sdk {

struct Vec3 {
    float x{}, y{}, z{};
    constexpr Vec3() = default;
    constexpr Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
    Vec3 operator-(const Vec3& r) const { return {x-r.x, y-r.y, z-r.z}; }
    Vec3 operator*(float s)      const { return {x*s, y*s, z*s}; }
    float dot(const Vec3& r) const { return x*r.x + y*r.y + z*r.z; }
    float length() const { return std::sqrt(dot(*this)); }
    float length_2d() const { return std::sqrt(x*x + y*y); }
    Vec3 normalized() const { float l = length(); return l > 1e-6f ? *this * (1.f/l) : Vec3{}; }
};

struct Vec2 { float x{}, y{}; Vec2 operator-(const Vec2& r) const { return {x-r.x, y-r.y}; } float length() const { return std::sqrt(x*x + y*y); } };
struct QAngle { float pitch{}, yaw{}, roll{}; };
struct ViewMatrix { float m[4][4]{}; };

inline bool world_to_screen(const ViewMatrix& vm, const Vec3& world, int sw, int sh, Vec2& out) {
    const float w = vm.m[3][0]*world.x + vm.m[3][1]*world.y + vm.m[3][2]*world.z + vm.m[3][3];
    if (w < 0.01f) return false;
    const float sx = vm.m[0][0]*world.x + vm.m[0][1]*world.y + vm.m[0][2]*world.z + vm.m[0][3];
    const float sy = vm.m[1][0]*world.x + vm.m[1][1]*world.y + vm.m[1][2]*world.z + vm.m[1][3];
    out.x = (sw * 0.5f) + (sx * (sw * 0.5f) / w);
    out.y = (sh * 0.5f) - (sy * (sh * 0.5f) / w);
    return true;
}

inline float wrap_angle(float a) { while (a > 180.f) a -= 360.f; while (a < -180.f) a += 360.f; return a; }

inline QAngle calc_angles(const Vec3& from, const Vec3& to) {
    const Vec3 d = to - from;
    const float len2d = d.length_2d();
    QAngle a;
    a.pitch = -std::atan2(d.z, len2d) * (180.f / 3.14159265358979f);
    a.yaw   =  std::atan2(d.y, d.x)   * (180.f / 3.14159265358979f);
    a.roll  = 0.f;
    return a;
}

} // namespace sdk