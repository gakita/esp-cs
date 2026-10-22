#include "esp/ESP.hpp"

#include "math/WorldToScreen.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace vanta::esp {

static ImU32 col32(const Color& c, float alpha_mul = 1.0f) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, c.a * alpha_mul));
}

static bool is_ally(const game::Entity& e, const game::Entity& local) {
    return e.team != game::Team::Unknown && e.team == local.team;
}

static const Color& pick_color(const ESPConfig& cfg, const game::Entity& e, const game::Entity* local) {
    if (!local) return cfg.enemy_color;
    return is_ally(e, *local) ? cfg.ally_color : cfg.enemy_color;
}

// Builds a tight 2D bbox from the per-bone screen projections of the entity.
// Falls back to a vertical capsule around origin if no bones projected.
struct Box { float x0, y0, x1, y1; bool valid; };

static Box compute_box(const game::Entity& e,
                       const math::Mat4& vp, float sw, float sh) {
    Box b{0, 0, 0, 0, false};
    bool first = true;
    // Bones in CS2 come in model-local coords and corrupt the box; skip them.
    for (auto& bone : e.bones) {
        if (true) continue; // disabled
        if (!bone.valid) continue;
        math::Vec2 s;
        if (!math::world_to_screen(vp, bone.pos, sw, sh, s)) continue;
        if (first) { b.x0 = b.x1 = s.x; b.y0 = b.y1 = s.y; first = false; }
        else {
            b.x0 = std::min(b.x0, s.x);
            b.y0 = std::min(b.y0, s.y);
            b.x1 = std::max(b.x1, s.x);
            b.y1 = std::max(b.y1, s.y);
        }
    }
    if (!first) { b.valid = true; return b; }

    // Fallback: project head + feet.
    math::Vec3 head = e.origin; head.z += 72.0f;
    math::Vec3 feet = e.origin;
    math::Vec2 s_head, s_feet;
    if (!math::world_to_screen(vp, head, sw, sh, s_head)) return b;
    if (!math::world_to_screen(vp, feet, sw, sh, s_feet)) return b;
    const float half_w = std::max(6.0f, std::abs(s_feet.y - s_head.y) * 0.18f);
    b.x0 = s_head.x - half_w;
    b.x1 = s_head.x + half_w;
    b.y0 = s_head.y;
    b.y1 = s_feet.y;
    b.valid = true;
    return b;
}

static void draw_corner_box(ImDrawList* dl, const Box& b, ImU32 c, float t) {
    const float w = b.x1 - b.x0;
    const float h = b.y1 - b.y0;
    const float cw = std::max(4.0f, w * 0.25f);
    const float ch = std::max(4.0f, h * 0.15f);

    // top-left
    dl->AddLine({b.x0, b.y0}, {b.x0 + cw, b.y0}, c, t);
    dl->AddLine({b.x0, b.y0}, {b.x0, b.y0 + ch}, c, t);
    // top-right
    dl->AddLine({b.x1, b.y0}, {b.x1 - cw, b.y0}, c, t);
    dl->AddLine({b.x1, b.y0}, {b.x1, b.y0 + ch}, c, t);
    // bottom-left
    dl->AddLine({b.x0, b.y1}, {b.x0 + cw, b.y1}, c, t);
    dl->AddLine({b.x0, b.y1}, {b.x0, b.y1 - ch}, c, t);
    // bottom-right
    dl->AddLine({b.x1, b.y1}, {b.x1 - cw, b.y1}, c, t);
    dl->AddLine({b.x1, b.y1}, {b.x1, b.y1 - ch}, c, t);
}

// Skeleton connection table — pairs of bone indices.
static constexpr std::array<std::pair<int,int>, 3> kSkeletonPairs = {{
    {(int)game::Bone::Head, (int)game::Bone::Neck},
    {(int)game::Bone::Neck, (int)game::Bone::Chest},
    {(int)game::Bone::Chest, (int)game::Bone::Pelvis},
}};

static void draw_skeleton(ImDrawList* /*dl*/, const game::Entity& /*e*/,
                          const math::Mat4& vp, float sw, float sh,
                          ImU32 c, float t) {
    if (true) return; for (auto& [a, b] : kSkeletonPairs) {
        if (!e.bones[a].valid || !e.bones[b].valid) continue;
        math::Vec2 pa, pb;
        if (!math::world_to_screen(vp, e.bones[a].pos, sw, sh, pa)) continue;
        if (!math::world_to_screen(vp, e.bones[b].pos, sw, sh, pb)) continue;
        dl->AddLine({pa.x, pa.y}, {pb.x, pb.y}, c, t);
    }
}

static void draw_health_bar(ImDrawList* dl, const Box& b, int hp, int armor, ImU32 team_col) {
    hp = std::clamp(hp, 0, 100);
    armor = std::clamp(armor, 0, 100);
    const float bar_x = b.x0 - 5.0f;
    const float bar_h = b.y1 - b.y0;
    const float ratio = hp / 100.0f;
    dl->AddRectFilled({bar_x - 1, b.y0 - 1}, {bar_x + 3, b.y1 + 1}, IM_COL32(0,0,0,200));
    dl->AddRectFilled({bar_x, b.y1 - bar_h * ratio},
                      {bar_x + 2, b.y1},
                      IM_COL32((int)((1 - ratio) * 240), (int)(ratio * 220), 60, 230));
    if (armor > 0) {
        const float ax = b.x1 + 2.0f;
        const float ar = armor / 100.0f;
        dl->AddRectFilled({ax - 1, b.y0 - 1}, {ax + 3, b.y1 + 1}, IM_COL32(0,0,0,200));
        dl->AddRectFilled({ax, b.y1 - bar_h * ar}, {ax + 2, b.y1}, team_col);
    }
}

void ESP::render(const game::Snapshot& snap, const ESPConfig& cfg, ImDrawList* dl) {
    m_stats = {};
    if (!cfg.enabled || !dl) return;
    if (snap.client_w <= 1.0f || snap.client_h <= 1.0f) return;

    const game::Entity* local = snap.local ? &*snap.local : nullptr;
    const auto center = ImVec2(snap.client_w * 0.5f, snap.client_h * 0.5f);

    for (const auto& e : snap.players) {
        if (e.is_local) continue; // never draw box on self
        if (local && e.pawn_address == local->pawn_address) continue;
        if (cfg.skip_dead && !e.alive) continue;

        const bool ally = local && is_ally(e, *local);
        if (ally  && !cfg.show_allies)  continue;
        if (!ally && !cfg.show_enemies) continue;

        const float dist = local ? math::Vec3::distance(local->origin, e.origin) : 0.0f;
        if (cfg.max_distance > 0.0f && dist > cfg.max_distance) continue;

        const Color& col = pick_color(cfg, e, local);
        const ImU32 c = col32(col, cfg.opacity);

        Box bb = compute_box(e, snap.view_projection, snap.client_w, snap.client_h);

        if (bb.valid) {
            const float bw = bb.x1 - bb.x0;
            const float bh = bb.y1 - bb.y0;
            if (bw > snap.client_w * 0.8f || bh > snap.client_h * 0.8f) continue;
            if (bw < 2.0f || bh < 2.0f) continue;
            if (cfg.box_cornered) {
                draw_corner_box(dl, bb, c, cfg.line_thickness);
            } else if (cfg.box_2d) {
                dl->AddRect({bb.x0, bb.y0}, {bb.x1, bb.y1}, c, 0.0f,
                            ImDrawFlags_None, cfg.line_thickness);
            }
            m_stats.boxes++;

            if (cfg.health || cfg.armor) draw_health_bar(dl, bb, e.health, e.armor, c);

            float text_y = bb.y1 + 2.0f;
            auto push_text = [&](const std::string& s, ImU32 tc) {
                ImVec2 sz = ImGui::CalcTextSize(s.c_str());
                dl->AddText(nullptr, cfg.text_size,
                            {bb.x0 + (bb.x1 - bb.x0 - sz.x) * 0.5f, text_y}, tc, s.c_str());
                text_y += cfg.text_size + 1.0f;
            };
            const ImU32 tc = col32(cfg.text_color, cfg.opacity);
            if (cfg.name && !e.name.empty())         push_text(e.name, tc);
            if (cfg.weapon && !e.active_weapon.empty()) push_text(e.active_weapon, tc);
            if (cfg.distance && local) {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.0fm", dist / 39.37f); // units->meters approx
                push_text(buf, tc);
            }
            if (cfg.team_id) push_text(game::team_label(e.team), tc);
            if (cfg.skeleton) {
                draw_skeleton(dl, e, snap.view_projection, snap.client_w, snap.client_h,
                              c, cfg.line_thickness);
                m_stats.skeletons++;
            }

            if (cfg.snapline) {
                dl->AddLine({center.x, snap.client_h},
                            {(bb.x0 + bb.x1) * 0.5f, bb.y1},
                            c, cfg.line_thickness);
                m_stats.snaplines++;
            }
        } else if (cfg.offscreen_arrows) {
            // Project head and clamp to screen edge.
            math::Vec3 head = e.origin; head.z += 72.0f;
            math::Vec2 s;
            if (math::world_to_screen(snap.view_projection, head,
                                      snap.client_w, snap.client_h, s)) {
                const float cx = snap.client_w * 0.5f;
                const float cy = snap.client_h * 0.5f;
                const float dx = s.x - cx, dy = s.y - cy;
                const float ang = std::atan2(dy, dx);
                const float r = std::min(snap.client_w, snap.client_h) * 0.42f;
                ImVec2 p{cx + std::cos(ang) * r, cy + std::sin(ang) * r};
                dl->AddCircleFilled(p, 5.0f, c, 12);
                m_stats.offscreen++;
            }
        }
    }

    // Bomb HUD.
    if (cfg.bomb_info && snap.bomb && snap.bomb->planted) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "BOMB %.1fs", snap.bomb->time_until_detonate);
        const ImU32 bc = col32(cfg.bomb_color, cfg.opacity);
        dl->AddText(nullptr, cfg.text_size + 2.0f,
                    {snap.client_w * 0.5f - 40.0f, 24.0f}, bc, buf);
        if (snap.bomb->defusing) {
            std::snprintf(buf, sizeof(buf), "DEFUSE %.1fs", snap.bomb->time_until_defused);
            dl->AddText(nullptr, cfg.text_size,
                        {snap.client_w * 0.5f - 40.0f, 42.0f}, bc, buf);
        }
    }
}

} // namespace vanta::esp
