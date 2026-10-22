#pragma once
#include <imgui.h>

namespace vanta::ui {

// Vanta palette (constitutional spec).
struct Palette {
    static constexpr ImU32 bg          = IM_COL32(0x09, 0x0B, 0x10, 0xFF);
    static constexpr ImU32 surface     = IM_COL32(0x11, 0x16, 0x20, 0xFF);
    static constexpr ImU32 elevated    = IM_COL32(0x18, 0x1F, 0x2B, 0xFF);
    static constexpr ImU32 border      = IM_COL32(0x27, 0x31, 0x42, 0xFF);
    static constexpr ImU32 accent      = IM_COL32(0xA7, 0x8B, 0xFA, 0xFF);
    static constexpr ImU32 secondary   = IM_COL32(0x67, 0xE8, 0xF9, 0xFF);
    static constexpr ImU32 success     = IM_COL32(0x34, 0xD3, 0x99, 0xFF);
    static constexpr ImU32 warning     = IM_COL32(0xFB, 0xBF, 0x24, 0xFF);
    static constexpr ImU32 error       = IM_COL32(0xFB, 0x71, 0x85, 0xFF);
    static constexpr ImU32 text        = IM_COL32(0xF1, 0xF5, 0xF9, 0xFF);
    static constexpr ImU32 text_dim    = IM_COL32(0x89, 0x95, 0xA8, 0xFF);
};

void apply_theme();

// Shared sidebar width.
constexpr float kSidebarWidth    = 225.0f;
constexpr float kWindowDefaultW  = 1180.0f;
constexpr float kWindowDefaultH  = 760.0f;

// Reusable widgets.
bool pill_toggle(const char* id, bool* v, const char* label_on = "ON",
                 const char* label_off = "OFF");
void section_header(const char* label);
void status_dot(ImU32 color, const char* label);
void help_tooltip(const char* text);

} // namespace vanta::ui
