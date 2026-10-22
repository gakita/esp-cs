#include <algorithm>
#include "extras/Radar.hpp"
#include "math/Angles.hpp"

#include <imgui.h>
#include <cmath>

namespace vanta::extras {

void Radar::render(const game::Snapshot& snap, const RadarConfig& cfg,
                   const math::Angles& view, ImDrawList* dl) {
    m_stats = {};
    if (!cfg.enabled || !dl || !snap.local) return;

    const float cx = cfg.offset_x_px + cfg.size_px * 0.5f;
    const float cy = cfg.offset_y_px + cfg.size_px * 0.5f;
    const float r  = cfg.size_px * 0.5f;

    const ImU32 bg   = IM_COL32(10, 12, 18, (int)(220 * cfg.opacity));
    const ImU32 edge = IM_COL32(39, 49, 66, (int)(255 * cfg.opacity));
    dl->AddCircleFilled({cx, cy}, r, bg, 48);
    dl->AddCircle({cx, cy}, r, edge, 48, 1.5f);
    dl->AddLine({cx - r, cy}, {cx + r, cy}, edge, 1.0f);
    dl->AddLine({cx, cy - r}, {cx, cy + r}, edge, 1.0f);

    const float yaw_rad = view.yaw * math::kDegRad;
    const float cos_y = std::cos(yaw_rad);
    const float sin_y = std::sin(yaw_rad);

    const game::Entity* local = &*snap.local;
    const auto project = [&](const math::Vec3& wp) -> std::pair<float, float> {
        float dx = wp.x - local->origin.x;
        float dy = wp.y - local->origin.y;
        float rx = dx, ry = dy;
        if (cfg.rotate_with_view) {
            rx =  dx * sin_y - dy * cos_y;
            ry = -dx * cos_y - dy * sin_y;
        }
        const float scale = r / std::max(1.0f, cfg.range_units);
        return {cx + rx * scale, cy + ry * scale};
    };

    for (const auto& e : snap.players) {
        if (!e.alive) continue;
        if (cfg.only_enemies && e.team == local->team) continue;
        auto [px, py] = project(e.origin);
        if (px < cfg.offset_x_px || px > cfg.offset_x_px + cfg.size_px) continue;
        if (py < cfg.offset_y_px || py > cfg.offset_y_px + cfg.size_px) continue;
        const bool ally = (e.team == local->team);
        const ImU32 c = ally ? IM_COL32(102, 204, 255, 230)
                             : IM_COL32(255, 90, 100, 240);
        dl->AddCircleFilled({px, py}, 3.5f, c, 12);
        m_stats.dots++;
    }

    if (cfg.draw_bomb && snap.bomb && snap.bomb->planted) {
        auto [bx, by] = project(snap.bomb->origin);
        dl->AddCircleFilled({bx, by}, 4.0f, IM_COL32(255, 190, 40, 240), 12);
        dl->AddCircle({bx, by}, 7.0f, IM_COL32(255, 190, 40, 180), 16, 1.2f);
    }
}

} // namespace vanta::extras
