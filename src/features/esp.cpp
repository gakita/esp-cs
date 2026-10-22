#include "esp.hpp"
#include "imgui.h"
#include <cstdio>
#include <algorithm>

namespace features {
static constexpr float UNITS_PER_METER = 39.37f;

static ImU32 team_color(std::uint8_t team) {
    switch (team) {
        case 2: return IM_COL32(255, 170, 60, 255);
        case 3: return IM_COL32( 90, 180, 255, 255);
        default: return IM_COL32(255, 255, 60, 255);
    }
}
static ImU32 health_color(int hp, int max) {
    const float t = std::clamp(float(hp) / float(max ? max : 100), 0.f, 1.f);
    return IM_COL32(int((1.f-t)*255.f), int(t*220.f+35.f), 32, 255);
}

void draw_esp(ImDrawList* dl, const Snapshot& snap, const EspConfig& cfg) {
    if (!cfg.enabled || !dl) return;
    const auto& view = snap.view;
    const int sw = snap.screen_w, sh = snap.screen_h;

    for (const auto& p : snap.players) {
        // paranoia: double-check mesmo passando pelo cache
        if (!p.valid || !p.alive || p.dormant) continue;
        if (p.team != 2 && p.team != 3) continue;
        if (p.health <= 0 || p.health > 100) continue;
        if (cfg.team_check && snap.local_team && p.team == snap.local_team) continue;

        const float dist_u = (p.origin - snap.local_eye).length();
        if (dist_u > cfg.max_distance * UNITS_PER_METER) continue;

        // usa bone_head (já world-space pelo cache) pra topo da box
        sdk::Vec2 feet{}, head{};
        if (!sdk::world_to_screen(view, p.origin,    sw, sh, feet)) continue;
        if (!sdk::world_to_screen(view, p.bone_head, sw, sh, head)) continue;

        const float h = std::abs(feet.y - head.y);
        if (h < 4.f) continue;
        const float w = h * 0.45f;
        const ImVec2 tl{head.x - w*0.5f, head.y};
        const ImVec2 br{head.x + w*0.5f, feet.y};
        const ImU32 col = team_color(p.team);

        if (cfg.box) {
            dl->AddRect(tl, br, IM_COL32(0,0,0,180), 0.f, 0, 3.f);
            dl->AddRect(tl, br, col,                 0.f, 0, 1.5f);
        }
        if (cfg.health) {
            const float bar_h = (br.y - tl.y);
            const float bar_x = tl.x - 7.f;
            const float frac  = std::clamp(float(p.health)/float(p.max_health?p.max_health:100), 0.f, 1.f);
            dl->AddRectFilled({bar_x-1, tl.y-1}, {bar_x+4, br.y+1}, IM_COL32(0,0,0,220));
            dl->AddRectFilled({bar_x, br.y - bar_h*frac}, {bar_x+3, br.y}, health_color(p.health, p.max_health));
        }
        if (cfg.distance) {
            char buf[32]; std::snprintf(buf, sizeof(buf), "%.0fm", dist_u / UNITS_PER_METER);
            dl->AddText({br.x + 4, tl.y}, col, buf);
        }
        if (cfg.snapline) {
            dl->AddLine({sw*0.5f, float(sh)}, {head.x, tl.y}, IM_COL32(255,255,255,120), 1.f);
        }
    }
}
}