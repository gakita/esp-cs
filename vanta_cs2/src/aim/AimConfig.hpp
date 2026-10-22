#pragma once
#include "game/Entity.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace vanta::aim {

enum class SelectionPriority : std::uint8_t {
    ClosestToCrosshair = 0,
    ClosestInWorld     = 1,
    LowestHealth       = 2,
};

enum class ActivationMode : std::uint8_t {
    Hold = 0,
    Toggle = 1,
};

struct AimConfig {
    bool enabled{false};
    ActivationMode mode{ActivationMode::Hold};
    std::uint32_t hotkey_vk{0x02 /* VK_RBUTTON */};

    float fov_deg{5.0f};
    bool  draw_fov_circle{true};

    // smoothing: 1 = snap, >1 = fractional step per frame.
    // smoothing works in frame-rate-independent fashion: effective fraction =
    // 1 - exp(-dt / tau), where tau = max(0.001, 1/smooth_rate_hz).
    float smooth_rate_hz{14.0f};

    game::Bone target_bone{game::Bone::Head};
    SelectionPriority priority{SelectionPriority::ClosestToCrosshair};
    bool only_enemies{true};
    bool require_visible{false};         // uses pawn.spotted (NOT a real trace)
    bool require_firing{false};
    bool hold_last_target{true};
    float target_switch_delay_ms{80.0f};

    float max_distance{300.0f * 12.0f};

    // per-weapon profile key: filled by UI; aim honors it when non-empty.
    std::string active_weapon_profile;
};

struct TriggerConfig {
    bool enabled{false};
    std::uint32_t hotkey_vk{0x12 /* VK_MENU */};
    float delay_ms{30.0f};
    float release_ms{10.0f};
    bool only_enemies{true};
    bool require_scoped{false};
};

struct RecoilConfig {
    bool enabled{false};
    float strength_x{0.6f};
    float strength_y{0.65f};
    bool only_while_firing{true};
    bool only_full_auto{true};
};

struct AimStats {
    std::uint64_t ticks{0};
    std::uint64_t targets_selected{0};
    std::uint64_t targets_lost{0};
    std::uint64_t mouse_events_sent{0};
};

} // namespace vanta::aim
