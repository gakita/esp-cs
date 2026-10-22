#pragma once
#include "aim/AimConfig.hpp"
#include "game/GameState.hpp"
#include "math/Angles.hpp"
#include "math/Vec.hpp"

#include <chrono>

namespace vanta::aim {

struct TriggerOutput {
    bool fire{false};   // mouse1 down
    bool release{false};// mouse1 up
};

class Trigger {
public:
    TriggerOutput tick(const game::Snapshot& snap,
                       const TriggerConfig& cfg,
                       const math::Angles& view,
                       std::chrono::steady_clock::time_point now,
                       bool hotkey_down);

    void reset();

private:
    bool m_firing{false};
    std::chrono::steady_clock::time_point m_hover_since{};
    std::chrono::steady_clock::time_point m_fire_began{};
    std::uint32_t m_hovered_index{0};
};

} // namespace vanta::aim
