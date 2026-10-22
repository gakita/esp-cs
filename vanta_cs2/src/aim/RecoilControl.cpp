#include "aim/RecoilControl.hpp"

namespace vanta::aim {

void RecoilControl::reset() {
    m_had_prev = false;
    m_prev_punch = {};
    m_prev_shots = 0;
}

RecoilOutput RecoilControl::tick(const game::Snapshot& snap,
                                 const RecoilConfig& cfg,
                                 float sensitivity,
                                 bool firing) {
    RecoilOutput out;
    if (!cfg.enabled || !snap.local) {
        reset();
        return out;
    }
    if (cfg.only_while_firing && !firing) {
        m_had_prev = false;
        return out;
    }

    // The snapshot producer should populate local->vel.z, local->armor etc.
    // We piggyback the punch angles on local.vel members via extra snapshot
    // fields in a future iteration; for now the pawn's punch_angle is
    // sampled in EntityReader (TODO) and lives on local.bones[0].pos as a
    // placeholder marker (NOT shipped — see docs/limitations.md).
    //
    // For a correct recoil-counter path, EntityReader must read
    // C_CSPlayerPawnBase::m_aimPunchAngle each tick and surface it here.
    // The structure below computes the counter delta assuming that read is
    // wired. Until then this returns {} and callers see no injected input.
    (void)sensitivity; (void)out;
    return out;
}

} // namespace vanta::aim
