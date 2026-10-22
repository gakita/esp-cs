#pragma once
#include "esp/ESPConfig.hpp"
#include "game/GameState.hpp"

struct ImDrawList;

namespace vanta::esp {

// Pure-function renderer. Takes the current snapshot and config, pushes
// shapes onto an ImDrawList. No reads from the process in here.
class ESP {
public:
    void render(const game::Snapshot& snap,
                const ESPConfig& cfg,
                ImDrawList* dl);

    struct DrawStats {
        std::uint32_t boxes{0};
        std::uint32_t skeletons{0};
        std::uint32_t snaplines{0};
        std::uint32_t offscreen{0};
    };
    DrawStats last_stats() const { return m_stats; }

private:
    DrawStats m_stats{};
};

} // namespace vanta::esp
