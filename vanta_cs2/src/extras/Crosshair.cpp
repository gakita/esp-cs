#include "extras/Crosshair.hpp"

#include <imgui.h>

namespace vanta::extras {

void Crosshair::render(ImDrawList* dl, float sw, float sh,
                       const CrosshairConfig& cfg,
                       float rdx, float rdy) {
    if (!cfg.enabled || !dl) return;
    const float cx = sw * 0.5f;
    const float cy = sh * 0.5f;
    const ImU32 c = ImGui::ColorConvertFloat4ToU32({cfg.r, cfg.g, cfg.b, cfg.a});
    const ImU32 outline = IM_COL32(0, 0, 0, (int)(cfg.a * 200));

    auto hline = [&](float a, float b, float y) {
        if (cfg.outline) dl->AddLine({a, y - 0.5f}, {b, y - 0.5f}, outline, cfg.thickness + 1.0f);
        dl->AddLine({a, y}, {b, y}, c, cfg.thickness);
    };
    auto vline = [&](float x, float a, float b) {
        if (cfg.outline) dl->AddLine({x - 0.5f, a}, {x - 0.5f, b}, outline, cfg.thickness + 1.0f);
        dl->AddLine({x, a}, {x, b}, c, cfg.thickness);
    };

    hline(cx - cfg.gap - cfg.length, cx - cfg.gap, cy);
    hline(cx + cfg.gap, cx + cfg.gap + cfg.length, cy);
    vline(cx, cy - cfg.gap - cfg.length, cy - cfg.gap);
    vline(cx, cy + cfg.gap, cy + cfg.gap + cfg.length);

    if (cfg.dot) {
        dl->AddRectFilled({cx - 0.5f, cy - 0.5f}, {cx + 0.5f, cy + 0.5f}, c);
    }

    if (cfg.show_recoil_marker && (rdx != 0.0f || rdy != 0.0f)) {
        dl->AddCircleFilled({cx + rdx, cy + rdy}, 2.0f, IM_COL32(255, 60, 90, 230), 12);
    }
}

} // namespace vanta::extras
