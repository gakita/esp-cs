#include "aim/AimAssist.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace vanta::aim {

namespace {

bool want_target(const AimConfig& cfg, const game::Entity& e, const game::Entity* local) {
    if (!e.alive) return false;
    if (cfg.only_enemies && local && e.team == local->team) return false;
    if (cfg.require_visible && !e.spotted) return false;
    return true;
}

math::Vec3 bone_pos(const game::Entity& e, game::Bone which) {
    const auto idx = (std::size_t)which;
    (void)idx; // bones in CS2 come in model-local coords; skip them, use fallback.
    // Fallback approximations in Source-2 units.
    math::Vec3 p = e.origin;
    switch (which) {
        case game::Bone::Head:   p.z += 72.0f; break;
        case game::Bone::Neck:   p.z += 64.0f; break;
        case game::Bone::Chest:  p.z += 54.0f; break;
        case game::Bone::Pelvis: p.z += 36.0f; break;
        default: break;
    }
    return p;
}

} // anonymous

void AimAssist::reset() {
    m_locked_target.reset();
    m_toggle_state = false;
    m_prev_hotkey  = false;
    m_resid_pitch  = 0.0f;
    m_resid_yaw    = 0.0f;
}

AimOutput AimAssist::tick(const game::Snapshot& snap,
                          const AimConfig& cfg,
                          const math::Angles& view,
                          float sensitivity,
                          std::chrono::steady_clock::time_point now,
                          float dt_seconds,
                          bool hotkey_down,
                          bool firing) {
    AimOutput out;
    m_stats.ticks++;

    if (!cfg.enabled || !snap.game_focused || !snap.local) {
        reset(); // never leak state between game sessions / unfocused windows
        return out;
    }

    // Activation.
    bool active = false;
    if (cfg.mode == ActivationMode::Hold) {
        active = hotkey_down;
    } else {
        if (hotkey_down && !m_prev_hotkey) m_toggle_state = !m_toggle_state;
        active = m_toggle_state;
    }
    m_prev_hotkey = hotkey_down;

    if (cfg.require_firing && !firing) active = false;

    if (!active) {
        // Clear residues and lock so a released hotkey never produces a stray
        // frame of motion.
        m_resid_pitch = 0.0f;
        m_resid_yaw   = 0.0f;
        if (!cfg.hold_last_target) m_locked_target.reset();
        return out;
    }
    out.active = true;

    // Selection: find the best candidate.
    const game::Entity* local = &*snap.local;
    const game::Entity* best  = nullptr;
    float best_score = std::numeric_limits<float>::infinity();
    for (const auto& e : snap.players) {
        if (!want_target(cfg, e, local)) continue;
        math::Vec3 target_pt = bone_pos(e, cfg.target_bone);
        float dist = math::Vec3::distance(local->origin, target_pt);
        if (cfg.max_distance > 0.0f && dist > cfg.max_distance) continue;

        math::Angles desired = math::calc_angle(local->eye_pos, target_pt);
        float ang_dist = math::angle_between(view, desired);
        if (ang_dist > cfg.fov_deg) continue;

        float score;
        switch (cfg.priority) {
            case SelectionPriority::ClosestInWorld:     score = dist;        break;
            case SelectionPriority::LowestHealth:       score = (float)e.health;  break;
            case SelectionPriority::ClosestToCrosshair:
            default:                                     score = ang_dist;    break;
        }
        if (score < best_score) { best_score = score; best = &e; }
    }

    // Target switch delay / hold.
    if (m_locked_target) {
        bool still_valid = false;
        for (const auto& e : snap.players) {
            if (e.index == *m_locked_target && want_target(cfg, e, local)) {
                math::Vec3 tp = bone_pos(e, cfg.target_bone);
                math::Angles d = math::calc_angle(local->eye_pos, tp);
                if (math::angle_between(view, d) <= cfg.fov_deg) {
                    still_valid = true;
                    best = &e;
                }
                break;
            }
        }
        if (!still_valid) {
            auto since = std::chrono::duration<float, std::milli>(now - m_last_switch).count();
            if (since < cfg.target_switch_delay_ms) {
                return out; // debounce
            }
            m_locked_target.reset();
            m_stats.targets_lost++;
        }
    }

    if (!best) return out;

    if (!m_locked_target || *m_locked_target != best->index) {
        m_locked_target = best->index;
        m_last_switch   = now;
        m_stats.targets_selected++;
    }

    // Compute desired angle delta.
    math::Vec3 target_pt = bone_pos(*best, cfg.target_bone);
    math::Angles desired = math::calc_angle(local->eye_pos, target_pt);
    math::Angles d       = math::delta(view, desired);

    // Frame-independent smoothing: effective fraction grows with dt but
    // asymptotes at 1 so very long frames don't overshoot.
    const float tau = 1.0f / std::max(0.001f, cfg.smooth_rate_hz);
    const float alpha = 1.0f - std::exp(-dt_seconds / tau);

    const float step_pitch = d.pitch * alpha + m_resid_pitch;
    const float step_yaw   = d.yaw   * alpha + m_resid_yaw;

    // Convert to raw mouse counts; keep the rounding residue for next frame
    // so repeated sub-count moves don't vanish.
    auto raw = math::degrees_to_mouse(step_pitch, step_yaw, sensitivity);
    const float applied_pitch = (float)raw.dy * sensitivity * 0.022f;
    const float applied_yaw   = -(float)raw.dx * sensitivity * 0.022f;
    m_resid_pitch = step_pitch - applied_pitch;
    m_resid_yaw   = step_yaw   - applied_yaw;

    // Clamp per-frame motion to a sane cap to kill oscillations if dt spikes.
    constexpr long kMaxCountsPerFrame = 400;
    raw.dx = std::clamp(raw.dx, -kMaxCountsPerFrame, kMaxCountsPerFrame);
    raw.dy = std::clamp(raw.dy, -kMaxCountsPerFrame, kMaxCountsPerFrame);

    out.mouse = raw;
    out.target_index      = best->index;
    out.distance_to_target = math::Vec3::distance(local->origin, target_pt);
    if (raw.dx != 0 || raw.dy != 0) m_stats.mouse_events_sent++;
    return out;
}

} // namespace vanta::aim
