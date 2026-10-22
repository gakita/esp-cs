#include "input/InputManager.hpp"

#include <windows.h>

namespace vanta::input {

void InputManager::panic()   { m_panic.store(true); }
void InputManager::unpanic() { m_panic.store(false); }

void InputManager::send_mouse_move(const math::MouseDelta& d) {
    if (m_panic.load() || (d.dx == 0 && d.dy == 0)) return;
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = d.dx;
    in.mi.dy = d.dy;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    ::SendInput(1, &in, sizeof(in));
}

static DWORD mouse_down_flag(int b) {
    switch (b) {
        case 0: return MOUSEEVENTF_LEFTDOWN;
        case 1: return MOUSEEVENTF_RIGHTDOWN;
        case 2: return MOUSEEVENTF_MIDDLEDOWN;
        default: return 0;
    }
}
static DWORD mouse_up_flag(int b) {
    switch (b) {
        case 0: return MOUSEEVENTF_LEFTUP;
        case 1: return MOUSEEVENTF_RIGHTUP;
        case 2: return MOUSEEVENTF_MIDDLEUP;
        default: return 0;
    }
}

void InputManager::send_mouse_down(int b) {
    if (m_panic.load()) return;
    INPUT in{}; in.type = INPUT_MOUSE; in.mi.dwFlags = mouse_down_flag(b);
    if (in.mi.dwFlags) ::SendInput(1, &in, sizeof(in));
}
void InputManager::send_mouse_up(int b) {
    if (m_panic.load()) return;
    INPUT in{}; in.type = INPUT_MOUSE; in.mi.dwFlags = mouse_up_flag(b);
    if (in.mi.dwFlags) ::SendInput(1, &in, sizeof(in));
}

static void send_key(std::uint32_t vk, bool down, bool panicked) {
    if (panicked) return;
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = (WORD)vk;
    in.ki.wScan = (WORD)::MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    ::SendInput(1, &in, sizeof(in));
}

void InputManager::send_key_down(std::uint32_t vk) { send_key(vk, true, m_panic.load()); }
void InputManager::send_key_up(std::uint32_t vk)   { send_key(vk, false, m_panic.load()); }
void InputManager::send_key_tap(std::uint32_t vk) {
    send_key_down(vk);
    send_key_up(vk);
}

} // namespace vanta::input
