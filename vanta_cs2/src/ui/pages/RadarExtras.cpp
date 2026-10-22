#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_radar_extras(App& app) {
    ImGui::TextUnformatted("Radar & Extras");

    section_header("RADAR");
    auto& r = app.radar_cfg;
    pill_toggle("radar.master", &r.enabled); ImGui::SameLine(); ImGui::TextUnformatted("Radar enabled");
    ImGui::SliderFloat("Size (px)",       &r.size_px, 100.0f, 400.0f, "%.0f");
    ImGui::SliderFloat("Range (units)",   &r.range_units, 500.0f, 10000.0f, "%.0f");
    ImGui::SliderFloat("Opacity",         &r.opacity, 0.1f, 1.0f, "%.2f");
    ImGui::Checkbox("Rotate with view",   &r.rotate_with_view);
    ImGui::Checkbox("Draw bomb",          &r.draw_bomb);
    ImGui::Checkbox("Enemies only",       &r.only_enemies);
    ImGui::SliderFloat("Offset X",        &r.offset_x_px, 0.0f, 400.0f, "%.0f");
    ImGui::SliderFloat("Offset Y",        &r.offset_y_px, 0.0f, 400.0f, "%.0f");

    section_header("CROSSHAIR");
    auto& x = app.xhair_cfg;
    pill_toggle("xhair.master", &x.enabled); ImGui::SameLine(); ImGui::TextUnformatted("Crosshair enabled");
    ImGui::SliderFloat("Gap",             &x.gap, 0.0f, 10.0f, "%.1f");
    ImGui::SliderFloat("Length",          &x.length, 1.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Thickness",       &x.thickness, 0.5f, 3.0f, "%.1f");
    ImGui::Checkbox("Outline",            &x.outline);
    ImGui::Checkbox("Center dot",         &x.dot);
    ImGui::Checkbox("Recoil marker",      &x.show_recoil_marker);
    ImGui::ColorEdit4("Color",            &x.r,
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);

    section_header("BUNNY HOP");
    auto& b = app.bhop_cfg;
    pill_toggle("bhop.master", &b.enabled); ImGui::SameLine(); ImGui::TextUnformatted("Bunny-hop enabled");
    ImGui::Checkbox("Only while moving", &b.only_while_moving);
    ImGui::TextDisabled("Hotkey: 'bhop.engage' on Hotkeys page.");
    ImGui::Spacing();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Palette::warning),
        "Bunny-hop needs an on_ground indicator sourced from the pawn's flags; "
        "this build currently assumes on_ground = true as a placeholder (see docs/limitations.md).");
}

} // namespace vanta::ui
