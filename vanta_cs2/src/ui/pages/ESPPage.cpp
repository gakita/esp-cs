#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <imgui.h>

namespace vanta::ui {

static void color_edit(const char* label, esp::Color& c) {
    float v[4] = {c.r, c.g, c.b, c.a};
    if (ImGui::ColorEdit4(label, v,
            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs)) {
        c.r = v[0]; c.g = v[1]; c.b = v[2]; c.a = v[3];
    }
}

void draw_esp(App& app) {
    auto& cfg = app.esp_cfg;
    ImGui::TextUnformatted("ESP");
    ImGui::TextDisabled("Every option here maps to a draw call in esp::ESP::render.");

    section_header("MASTER");
    pill_toggle("esp.master", &cfg.enabled); ImGui::SameLine(); ImGui::TextUnformatted("ESP enabled");

    section_header("SHAPES");
    ImGui::Checkbox("2D Box",        &cfg.box_2d);
    ImGui::Checkbox("Cornered Box",  &cfg.box_cornered);
    ImGui::Checkbox("Skeleton",      &cfg.skeleton);
    ImGui::Checkbox("Snaplines",     &cfg.snapline);
    ImGui::Checkbox("Off-screen arrows", &cfg.offscreen_arrows);

    section_header("INFO STRIPS");
    ImGui::Checkbox("Name",      &cfg.name);
    ImGui::SameLine(150);
    ImGui::Checkbox("Weapon",    &cfg.weapon);
    ImGui::Checkbox("Health",    &cfg.health);
    ImGui::SameLine(150);
    ImGui::Checkbox("Armor",     &cfg.armor);
    ImGui::Checkbox("Distance",  &cfg.distance);
    ImGui::SameLine(150);
    ImGui::Checkbox("Team tag",  &cfg.team_id);
    ImGui::Checkbox("Target indicator", &cfg.target_indicator);
    ImGui::Checkbox("Bomb info",        &cfg.bomb_info);

    section_header("FILTERS");
    ImGui::Checkbox("Draw enemies", &cfg.show_enemies);
    ImGui::SameLine(170);
    ImGui::Checkbox("Draw allies",  &cfg.show_allies);
    ImGui::Checkbox("Skip dead players", &cfg.skip_dead);
    ImGui::SliderFloat("Max distance (units)", &cfg.max_distance, 0.0f, 6000.0f, "%.0f");

    section_header("STYLE");
    color_edit("Enemy color", cfg.enemy_color);
    color_edit("Ally color",  cfg.ally_color);
    color_edit("Text color",  cfg.text_color);
    color_edit("Bomb color",  cfg.bomb_color);
    ImGui::SliderFloat("Line thickness", &cfg.line_thickness, 0.5f, 4.0f, "%.1f");
    ImGui::SliderFloat("Opacity",        &cfg.opacity,         0.1f, 1.0f, "%.2f");
    ImGui::SliderFloat("Text size",      &cfg.text_size,       10.0f, 22.0f, "%.0f");

    section_header("DRAW STATS (this frame)");
    ImGui::Text("boxes:    %u", app.esp_stats.boxes);
    ImGui::Text("skeleton: %u", app.esp_stats.skeletons);
    ImGui::Text("snaplines:%u", app.esp_stats.snaplines);
    ImGui::Text("off-screen arrows: %u", app.esp_stats.offscreen);

    ImGui::Spacing();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Palette::text_dim),
        "Visibility (`spotted`) comes from the game's own publish bits; this build does "
        "not perform ray-cast line-of-sight verification. See Diagnostics > Limits.");
}

} // namespace vanta::ui
