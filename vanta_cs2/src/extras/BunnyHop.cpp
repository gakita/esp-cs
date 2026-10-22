#include "extras/BunnyHop.hpp"

#include <cmath>

namespace vanta::extras {

BunnyHopOutput BunnyHop::tick(const game::Snapshot& snap,
                              const BunnyHopConfig& cfg,
                              bool hotkey_down, bool on_ground) {
    BunnyHopOutput out;

    if (!cfg.enabled || !snap.local || !snap.game_focused || !hotkey_down) {
        if (m_held_jump) { out.release_jump = true; m_held_jump = false; }
        m_prev_pressed = false;
        return out;
    }

    if (cfg.only_while_moving) {
        const auto& v = snap.local->vel;
        float speed = std::sqrt(v.x * v.x + v.y * v.y);
        if (speed < 50.0f) {
            if (m_held_jump) { out.release_jump = true; m_held_jump = false; }
            return out;
        }
    }

    // Fire a jump edge whenever we're on the ground AND the hotkey is held.
    // Release the previous press first to generate a real edge.
    if (on_ground) {
        if (m_held_jump) { out.release_jump = true; m_held_jump = false; }
        else { out.send_jump = true; m_held_jump = true; }
    }

    m_prev_pressed = true;
    return out;
}

} // namespace vanta::extras
