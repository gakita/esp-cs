#pragma once
#include "game/Entity.hpp"
#include "math/Vec.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace vanta::game {

// Immutable snapshot the renderer / aim / esp consume.
// Produced by GameState::tick(); readers get a const shared_ptr.
struct Snapshot {
    std::uint64_t tick_index{0};
    std::uint64_t captured_at_ns{0};

    bool attached{false};
    bool game_focused{false};

    // Local player pawn, if resolved.
    std::optional<Entity> local;

    // Everyone else (not the local player, excluding dead if filtered).
    std::vector<Entity> players;

    // Current VP matrix for world->screen projection.
    math::Mat4 view_projection{};

    // Bomb state, if the bomb entity resolved.
    std::optional<BombState> bomb;

    // Overlay target window metrics; projection assumes these dims.
    float client_w{0};
    float client_h{0};
};

// GameState owns the double-buffered snapshot. Publish is a single atomic
// pointer swap, so readers never see a partially-filled frame.
class GameState {
public:
    void publish(std::shared_ptr<const Snapshot> s);
    std::shared_ptr<const Snapshot> current() const;

private:
    mutable std::mutex m_mx;
    std::shared_ptr<const Snapshot> m_cur;
};

} // namespace vanta::game
