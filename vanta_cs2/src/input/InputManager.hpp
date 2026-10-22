#pragma once
#include "math/Angles.hpp"

#include <atomic>
#include <cstdint>

namespace vanta::input {

// Wraps SendInput for mouse motion / button events.
// All methods are no-ops when panic() has been called (via panic hotkey).
class InputManager {
public:
    void panic();              // kill-switch: disables all injection until unpanic()
    void unpanic();
    bool is_panicked() const { return m_panic.load(); }

    void send_mouse_move(const math::MouseDelta& d);
    void send_mouse_down(int button /* 0 = L, 1 = R, 2 = M */);
    void send_mouse_up(int button);
    void send_key_down(std::uint32_t vk);
    void send_key_up(std::uint32_t vk);
    void send_key_tap(std::uint32_t vk);

private:
    std::atomic<bool> m_panic{false};
};

} // namespace vanta::input
