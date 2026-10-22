#pragma once
#include "game/GameState.hpp"

namespace vanta::extras {

struct BunnyHopConfig {
    bool enabled{false};
    std::uint32_t hotkey_vk{0x20 /* VK_SPACE */};
    bool only_while_moving{true};
};

struct BunnyHopOutput {
    bool send_jump{false};
    bool release_jump{false};
};

class BunnyHop {
public:
    BunnyHopOutput tick(const game::Snapshot& snap, const BunnyHopConfig& cfg,
                        bool hotkey_down, bool player_on_ground);

    void reset() { m_prev_pressed = false; m_held_jump = false; }

private:
    bool m_prev_pressed{false};
    bool m_held_jump{false};
};

} // namespace vanta::extras
