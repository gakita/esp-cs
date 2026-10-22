#pragma once
#include "math/Vec.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace vanta::game {

enum class Team : std::uint8_t {
    Unknown     = 0,
    Spectator   = 1,
    Terrorist   = 2,
    CounterTerr = 3,
};

inline const char* team_label(Team t) {
    switch (t) {
        case Team::Terrorist:   return "T";
        case Team::CounterTerr: return "CT";
        case Team::Spectator:   return "SPEC";
        default: return "?";
    }
}

// Bone indices used for aiming / skeleton rendering. The actual Source 2
// bone index per model is NOT guessed here — this enum is the semantic name
// the aim subsystem asks for; the EntityReader maps enum->index via the
// offset bundle (`bone.head`, `bone.neck`, ...).
enum class Bone : std::uint8_t {
    Head  = 0,
    Neck  = 1,
    Chest = 2,
    Pelvis = 3,
    Count
};

struct BoneTransform {
    math::Vec3 pos;
    bool valid{false};
};

struct Entity {
    std::uint32_t index{0};
    std::uintptr_t pawn_address{0};
    Team team{Team::Unknown};
    std::string name;

    int health{0};
    int armor{0};
    bool alive{false};
    bool is_local{false};
    bool has_bomb{false};

    math::Vec3 origin;          // feet
    math::Vec3 eye_pos;         // view origin for the player (approx)
    math::Vec3 vel;
    std::string active_weapon;  // free-form name string if resolved
    std::array<BoneTransform, (std::size_t)Bone::Count> bones{};

    // Game-reported "spotted by local" bit. True in CS2 only when the game
    // itself believes we can see the player, i.e. subject to anti-cheat
    // visibility rules. Not a trace.
    bool spotted{false};
};

struct BombState {
    bool planted{false};
    bool defusing{false};
    float time_until_detonate{0.0f};
    float time_until_defused{0.0f};
    math::Vec3 origin;
};

} // namespace vanta::game
