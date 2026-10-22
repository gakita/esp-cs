#include "game/GameState.hpp"

namespace vanta::game {

void GameState::publish(std::shared_ptr<const Snapshot> s) {
    std::lock_guard lock(m_mx);
    m_cur = std::move(s);
}

std::shared_ptr<const Snapshot> GameState::current() const {
    std::lock_guard lock(m_mx);
    return m_cur;
}

} // namespace vanta::game
