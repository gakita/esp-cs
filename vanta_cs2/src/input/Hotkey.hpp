#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace vanta::input {

// Hotkey = single virtual-key with optional modifiers.
struct Hotkey {
    std::uint32_t vk{0};
    bool ctrl{false};
    bool shift{false};
    bool alt{false};

    bool empty() const { return vk == 0; }
    std::string display() const;

    nlohmann::json to_json() const;
    static Hotkey from_json(const nlohmann::json& j);

    bool operator==(const Hotkey& o) const {
        return vk == o.vk && ctrl == o.ctrl && shift == o.shift && alt == o.alt;
    }
};

// Named hotkey map persisted in settings.json under "hotkeys".
struct HotkeyMap {
    std::unordered_map<std::string, Hotkey> entries;

    void save_to_settings();
    void load_from_settings();

    // Returns the names bound to the same keypress (collision detection).
    std::vector<std::string> collisions(const Hotkey& h, const std::string& skip) const;
};

// Captures the next virtual-key press (polling GetAsyncKeyState). Blocking;
// returns an empty Hotkey if `cancel_check` returns true before a key.
Hotkey capture_blocking(const std::function<bool()>& cancel_check);

// Returns true if the key is currently pressed (GetAsyncKeyState high bit).
bool is_pressed(std::uint32_t vk);

// Convenience: evaluates whether a Hotkey is "down right now" with modifiers.
bool is_hotkey_down(const Hotkey& h);

} // namespace vanta::input
