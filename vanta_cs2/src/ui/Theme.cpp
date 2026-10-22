#include "ui/Theme.hpp"

namespace vanta::ui {

static ImVec4 to4(ImU32 c) {
    return ImGui::ColorConvertU32ToFloat4(c);
}

void apply_theme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding   = {18, 18};
    s.FramePadding    = {10, 6};
    s.ItemSpacing     = {10, 8};
    s.ItemInnerSpacing= {6, 6};
    s.ScrollbarSize   = 10;
    s.GrabMinSize     = 8;
    s.WindowRounding  = 10;
    s.FrameRounding   = 7;
    s.PopupRounding   = 7;
    s.GrabRounding    = 6;
    s.ScrollbarRounding = 8;
    s.TabRounding     = 7;
    s.WindowBorderSize = 1;

    auto& c = s.Colors;
    c[ImGuiCol_WindowBg]            = to4(Palette::bg);
    c[ImGuiCol_ChildBg]             = to4(Palette::surface);
    c[ImGuiCol_PopupBg]             = to4(Palette::elevated);
    c[ImGuiCol_Border]              = to4(Palette::border);
    c[ImGuiCol_FrameBg]             = to4(Palette::elevated);
    c[ImGuiCol_FrameBgHovered]      = ImVec4(0.17f, 0.21f, 0.29f, 1.0f);
    c[ImGuiCol_FrameBgActive]       = ImVec4(0.21f, 0.26f, 0.37f, 1.0f);
    c[ImGuiCol_TitleBg]             = to4(Palette::surface);
    c[ImGuiCol_TitleBgActive]       = to4(Palette::elevated);
    c[ImGuiCol_MenuBarBg]           = to4(Palette::surface);
    c[ImGuiCol_ScrollbarBg]         = to4(Palette::surface);
    c[ImGuiCol_ScrollbarGrab]       = to4(Palette::border);
    c[ImGuiCol_ScrollbarGrabHovered]= to4(Palette::accent);
    c[ImGuiCol_ScrollbarGrabActive] = to4(Palette::accent);
    c[ImGuiCol_SliderGrab]          = to4(Palette::accent);
    c[ImGuiCol_SliderGrabActive]    = to4(Palette::secondary);
    c[ImGuiCol_Button]              = to4(Palette::elevated);
    c[ImGuiCol_ButtonHovered]       = ImVec4(0.22f, 0.28f, 0.4f, 1.0f);
    c[ImGuiCol_ButtonActive]        = to4(Palette::accent);
    c[ImGuiCol_Header]              = to4(Palette::elevated);
    c[ImGuiCol_HeaderHovered]       = ImVec4(0.23f, 0.29f, 0.42f, 1.0f);
    c[ImGuiCol_HeaderActive]        = to4(Palette::accent);
    c[ImGuiCol_Separator]           = to4(Palette::border);
    c[ImGuiCol_SeparatorHovered]    = to4(Palette::accent);
    c[ImGuiCol_SeparatorActive]     = to4(Palette::accent);
    c[ImGuiCol_Tab]                 = to4(Palette::surface);
    c[ImGuiCol_TabHovered]          = ImVec4(0.26f, 0.33f, 0.47f, 1.0f);
    c[ImGuiCol_TabActive]           = to4(Palette::elevated);
    c[ImGuiCol_Text]                = to4(Palette::text);
    c[ImGuiCol_TextDisabled]        = to4(Palette::text_dim);
    c[ImGuiCol_CheckMark]           = to4(Palette::accent);
}

bool pill_toggle(const char* id, bool* v, const char* label_on, const char* label_off) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 sz{80, 24};
    ImGui::PushID(id);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    cursor.x += avail.x - sz.x;
    ImGui::SetCursorScreenPos(cursor);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 track_on  = Palette::accent;
    const ImU32 track_off = Palette::border;
    const ImU32 knob      = IM_COL32(0xF1, 0xF5, 0xF9, 0xFF);

    ImGui::InvisibleButton("##hit", sz);
    bool changed = false;
    if (ImGui::IsItemClicked(0)) { *v = !*v; changed = true; }

    dl->AddRectFilled(cursor,
                      {cursor.x + sz.x, cursor.y + sz.y},
                      *v ? track_on : track_off, 12);
    float knob_x = *v ? cursor.x + sz.x - 20 : cursor.x + 4;
    dl->AddCircleFilled({knob_x + 8, cursor.y + sz.y * 0.5f}, 8, knob, 24);
    const char* lbl = *v ? label_on : label_off;
    ImVec2 ts = ImGui::CalcTextSize(lbl);
    dl->AddText({cursor.x + (sz.x - ts.x) * 0.5f + (*v ? -8.0f : 8.0f),
                 cursor.y + (sz.y - ts.y) * 0.5f},
                IM_COL32(0, 0, 0, 220), lbl);
    ImGui::PopID();
    return changed;
}

void section_header(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, to4(Palette::text_dim));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

void status_dot(ImU32 color, const char* label) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled({p.x + 6, p.y + 10}, 5, color, 16);
    ImGui::Dummy({14, 0});
    ImGui::SameLine(0, 2);
    ImGui::TextUnformatted(label);
}

void help_tooltip(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(380.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace vanta::ui
