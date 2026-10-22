// math/WorldToScreen.hpp
#pragma once

#include "math/Mat4.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"

namespace vanta::math {

// Projects a world-space point through a Source 2 row-major view-projection
// matrix onto screen space.
//
//   vp : the 4x4 view-projection matrix read from client.dll + dwViewMatrix
//        (Source 2 stores it row-major, so result = vp * [x y z 1]^T)
//   p  : world-space point
//   sw : SCREEN WIDTH in pixels — MUST be the client-area width of
//        cs2.exe's window (GetClientRect on the game HWND). Passing the
//        desktop width or the overlay window's size produces lateral/
//        vertical offsets in windowed-fullscreen and multi-monitor setups.
//   sh : SCREEN HEIGHT in pixels — same constraint as sw.
//   out: filled with the on-screen pixel position (origin top-left).
//
// Returns false when the point is behind the near plane (w < 0.01).
inline bool world_to_screen(const Mat4& vp, Vec3 p, float sw, float sh, Vec2& out) {
    const float x = vp(0,0)*p.x + vp(0,1)*p.y + vp(0,2)*p.z + vp(0,3);
    const float y = vp(1,0)*p.x + vp(1,1)*p.y + vp(1,2)*p.z + vp(1,3);
    const float w = vp(3,0)*p.x + vp(3,1)*p.y + vp(3,2)*p.z + vp(3,3);

    if (w < 0.01f) return false; // behind near plane

    const float inv   = 1.0f / w;
    const float ndc_x = x * inv;      // -1..+1 right
    const float ndc_y = y * inv;      // -1..+1 up (Source 2 convention)

    out.x = (sw * 0.5f) + (sw * 0.5f) * ndc_x;
    out.y = (sh * 0.5f) - (sh * 0.5f) * ndc_y;
    return true;
}

} // namespace vanta::math