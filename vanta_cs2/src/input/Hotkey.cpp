#include "input/Hotkey.hpp"

#include "core/Config.hpp"

#include <chrono>
#include <thread>
#include <windows.h>

namespace vanta::input {

static const char* vk_name(std::uint32_t vk) {
    switch (vk) {
        case 0x01: return "LMB";
        case 0x02: return "RMB";
        case 0x04: return "MMB";
        case 0x05: return "X1";
        case 0x06: return "X2";
        case VK_SPACE:   return "Space";
        case VK_SHIFT:   return "Shift";
        case VK_CONTROL: return "Ctrl";
        case VK_MENU:    return "Alt";
        case VK_ESCAPE:  return "Esc";
        case VK_RETURN:  return "Enter";
        case VK_TAB:     return "Tab";
        default: return nullptr;
    }
}

std::string Hotkey::display() const {
    if (vk == 0) return "<unbound>";
    std::string out;
    if (ctrl)  out += "Ctrl+";
    if (shift) out += "Shift+";
    if (alt)   out += "Alt+";
    if (const char* n = vk_name(vk)) { out += n; return out; }
    if (vk >= 0x30 && vk <= 0x39) { out.push_back((char)vk); return out; }
    if (vk >= 0x41 && vk <= 0x5A) { out.push_back((char)vk); return out; }
    if (vk >= VK_F1 && vk <= VK_F24) { out += "F" + std::to_string(vk - VK_F1 + 1); return out; }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "VK_%02X", vk);
    out += buf;
    return out;
}

nlohmann::json Hotkey::to_json() const {
    return {{"vk", vk}, {"ctrl", ctrl}, {"shift", shift}, {"alt", alt}};
}

Hotkey Hotkey::from_json(const nlohmann::json& j) {
    Hotkey h;
    h.vk    = j.value("vk", 0u);
    h.ctrl  = j.value("ctrl", false);
    h.shift = j.value("shift", false);
    h.alt   = j.value("alt", false);
    return h;
}

bool is_pressed(std::uint32_t vk) {
    if (vk == 0) return false;
    return (::GetAsyncKeyState((int)vk) & 0x8000) != 0;
}

bool is_hotkey_down(const Hotkey& h) {
    if (h.empty()) return false;
    if (h.ctrl  && !is_pressed(VK_CONTROL)) return false;
    if (h.shift && !is_pressed(VK_SHIFT))   return false;
    if (h.alt   && !is_pressed(VK_MENU))    return false;
    return is_pressed(h.vk);
}

Hotkey capture_blocking(const std::function<bool()>& cancel) {
    while (!cancel()) {
        // Skip pure-modifier captures.
        for (int vk = 1; vk < 0xFF; ++vk) {
            if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU) continue;
            if (::GetAsyncKeyState(vk) & 0x8000) {
                Hotkey h;
                h.vk = (std::uint32_t)vk;
                h.ctrl  = is_pressed(VK_CONTROL);
                h.shift = is_pressed(VK_SHIFT);
                h.alt   = is_pressed(VK_MENU);
                return h;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return {};
}

void HotkeyMap::save_to_settings() {
    nlohmann::json j = nlohmann::json::object();
    for (auto& [k, v] : entries) j[k] = v.to_json();
    config::Settings::instance().set("hotkeys", std::move(j));
    config::Settings::instance().save();
}

void HotkeyMap::load_from_settings() {
    auto j = config::Settings::instance().get("hotkeys", nlohmann::json::object());
    entries.clear();
    if (!j.is_object()) return;
    for (auto it = j.begin(); it != j.end(); ++it) {
        entries.emplace(it.key(), Hotkey::from_json(it.value()));
    }
}

std::vector<std::string> HotkeyMap::collisions(const Hotkey& h, const std::string& skip) const {
    std::vector<std::string> out;
    if (h.empty()) return out;
    for (auto& [k, v] : entries) {
        if (k == skip) continue;
        if (v == h) out.push_back(k);
    }
    return out;
}

} // namespace vanta::input
