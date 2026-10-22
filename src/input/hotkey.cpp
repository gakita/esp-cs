#include "hotkey.hpp"
#include <Windows.h>
#include <array>
namespace input {
static std::array<bool, 256> last_state{};
bool key_pressed(int vk) {
    if (vk < 0 || vk >= 256) return false;
    const bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool was = last_state[vk];
    last_state[vk] = now;
    return now && !was;
}
}