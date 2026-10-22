#pragma once
#include "aim/AimConfig.hpp"
#include "game/GameState.hpp"
#include "math/Angles.hpp"

#include <chrono>
#include <optional>

namespace vanta::aim {

// Target selection + smoothing. Pure w.r.t. the OS — the caller takes the
// returned mouse delta (if any) and feeds SendInput.
struct AimOutput {
    bool active{false};              // engine was active this tick
    math::MouseDelta mouse{};        // raw mouse counts to inject
    std::optional<std::uint32_t> target_index;
    float distance_to_target{0.0f};
};

class AimAssist {
public:
    AimOutput tick(const game::Snapshot& snap,
                   const AimConfig& cfg,
                   const math::Angles& view_angles,
                   float player_sensitivity,
                   std::chrono::steady_clock::time_point now,
                   float dt_seconds,
                   bool hotkey_down,
                   bool firing);

    void reset();

    AimStats stats() const { return m_stats; }

private:
    AimStats m_stats{};
    bool m_toggle_state{false};
    bool m_prev_hotkey{false};
    std::optional<std::uint32_t> m_locked_target;
    std::chrono::steady_clock::time_point m_last_switch{};

    // Backing state for frame-independent smoothing residue.
    float m_resid_pitch{0.0f};
    float m_resid_yaw{0.0f};
};

} // namespace vanta::aim
