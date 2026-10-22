#include "aim/Trigger.hpp"
#include "math/Angles.hpp"

#include <cmath>

namespace vanta::aim {

void Trigger::reset() {
    m_firing = false;
    m_hovered_index = 0;
}

TriggerOutput Trigger::tick(const game::Snapshot& snap,
                            const TriggerConfig& cfg,
                            const math::Angles& view,
                            std::chrono::steady_clock::time_point now,
                            bool hotkey_down) {
    TriggerOutput out;

    if (!cfg.enabled || !snap.game_focused || !snap.local || !hotkey_down) {
        if (m_firing) { out.release = true; m_firing = false; }
        m_hovered_index = 0;
        return out;
    }

    const auto* local = &*snap.local;

    // Determine whether the crosshair is on a valid target. Use the angle
    // between view and head bone as the "on target" test rather than a
    // trace — we don't have a trace.
    const game::Entity* hovered = nullptr;
    float best_ang = 1.5f; // deg
    for (const auto& e : snap.players) {
        if (!e.alive) continue;
        if (cfg.only_enemies && e.team == local->team) continue;
        math::Vec3 bp = e.bones[(std::size_t)game::Bone::Chest].valid
                            ? e.bones[(std::size_t)game::Bone::Chest].pos
                            : e.origin;
        bp.z += 48.0f;
        auto desired = math::calc_angle(local->eye_pos, bp);
        float a = math::angle_between(view, desired);
        if (a < best_ang) { best_ang = a; hovered = &e; }
    }

    if (!hovered) {
        if (m_firing) { out.release = true; m_firing = false; }
        m_hovered_index = 0;
        return out;
    }

    // Target hover timing — enforce delay_ms before firing.
    if (m_hovered_index != hovered->index) {
        m_hovered_index = hovered->index;
        m_hover_since   = now;
    }
    const auto hovered_ms =
        std::chrono::duration<float, std::milli>(now - m_hover_since).count();

    if (!m_firing) {
        if (hovered_ms >= cfg.delay_ms) {
            out.fire = true;
            m_firing = true;
            m_fire_began = now;
        }
    } else {
        const auto held_ms =
            std::chrono::duration<float, std::milli>(now - m_fire_began).count();
        if (held_ms >= cfg.release_ms) {
            out.release = true;
            m_firing = false;
        }
    }

    return out;
}

} // namespace vanta::aim
