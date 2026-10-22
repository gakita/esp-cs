#pragma once
#include "aim/AimConfig.hpp"
#include "game/GameState.hpp"
#include "math/Angles.hpp"

namespace vanta::aim {

struct RecoilOutput {
    math::MouseDelta mouse{};
    bool active{false};
};

// Reads the pawn's current punch angle delta vs. the previous frame and
// generates a mouse delta that counters the change. Works only when the
// offset bundle exposes `C_CSPlayerPawnBase::m_aimPunchAngle` and the pawn
// is firing (shots_fired delta > 0) when `only_while_firing` is set.
class RecoilControl {
public:
    RecoilOutput tick(const game::Snapshot& snap,
                      const RecoilConfig& cfg,
                      float sensitivity,
                      bool firing);

    void reset();

private:
    math::Angles m_prev_punch{};
    int m_prev_shots{0};
    bool m_had_prev{false};
};

} // namespace vanta::aim
