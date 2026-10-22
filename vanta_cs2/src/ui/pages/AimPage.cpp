#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <imgui.h>

namespace vanta::ui {

static const char* bone_name(game::Bone b) {
    switch (b) {
        case game::Bone::Head:   return "Head";
        case game::Bone::Neck:   return "Neck";
        case game::Bone::Chest:  return "Chest";
        case game::Bone::Pelvis: return "Pelvis";
        default: return "?";
    }
}

static const char* prio_name(aim::SelectionPriority p) {
    switch (p) {
        case aim::SelectionPriority::ClosestToCrosshair: return "Closest to crosshair";
        case aim::SelectionPriority::ClosestInWorld:     return "Closest in world";
        case aim::SelectionPriority::LowestHealth:       return "Lowest health";
    }
    return "?";
}

void draw_aim(App& app) {
    auto& cfg = app.aim_cfg;
    ImGui::TextUnformatted("Aimbot");
    ImGui::TextDisabled("Angle target selection + frame-rate-independent smoothing.");

    section_header("ACTIVATION");
    pill_toggle("aim.master", &cfg.enabled); ImGui::SameLine(); ImGui::TextUnformatted("Aim enabled");
    bool hold = cfg.mode == aim::ActivationMode::Hold;
    if (ImGui::RadioButton("Hold",  hold))  cfg.mode = aim::ActivationMode::Hold;
    ImGui::SameLine();
    if (ImGui::RadioButton("Toggle", !hold)) cfg.mode = aim::ActivationMode::Toggle;
    ImGui::TextUnformatted("Hotkey: configure on the Hotkeys page as 'aim.engage'.");

    section_header("TARGETING");
    ImGui::SliderFloat("FOV (degrees)",       &cfg.fov_deg, 0.5f, 45.0f, "%.1f");
    ImGui::Checkbox("Draw FOV circle",        &cfg.draw_fov_circle);
    ImGui::SliderFloat("Smoothing rate (Hz)", &cfg.smooth_rate_hz, 1.0f, 60.0f, "%.1f");
    help_tooltip("Higher = snappier. Framerate-independent: effective per-frame fraction is "
                 "1 - exp(-dt / (1/rate)).");

    if (ImGui::BeginCombo("Target bone", bone_name(cfg.target_bone))) {
        for (int i = 0; i < (int)game::Bone::Count; ++i) {
            if (ImGui::Selectable(bone_name((game::Bone)i), (int)cfg.target_bone == i)) {
                cfg.target_bone = (game::Bone)i;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Priority", prio_name(cfg.priority))) {
        for (int i = 0; i < 3; ++i) {
            if (ImGui::Selectable(prio_name((aim::SelectionPriority)i), (int)cfg.priority == i)) {
                cfg.priority = (aim::SelectionPriority)i;
            }
        }
        ImGui::EndCombo();
    }

    section_header("FILTERS");
    ImGui::Checkbox("Only enemies",       &cfg.only_enemies);
    ImGui::Checkbox("Require spotted bit", &cfg.require_visible);
    help_tooltip("Uses the game's own spotted bit. This is NOT a line-of-sight trace.");
    ImGui::Checkbox("Require firing",     &cfg.require_firing);
    ImGui::Checkbox("Hold last target",   &cfg.hold_last_target);
    ImGui::SliderFloat("Switch delay (ms)", &cfg.target_switch_delay_ms, 0.0f, 500.0f, "%.0f");
    ImGui::SliderFloat("Max distance (units)", &cfg.max_distance, 0.0f, 6000.0f, "%.0f");

    section_header("PER-WEAPON PROFILE");
    char profile_buf[128];
    std::snprintf(profile_buf, sizeof(profile_buf), "%s", cfg.active_weapon_profile.c_str());
    if (ImGui::InputText("Weapon profile key", profile_buf, sizeof(profile_buf))) {
        cfg.active_weapon_profile = profile_buf;
    }
    help_tooltip("Free-form string — Profile Manager can swap settings keyed off this field.");

    section_header("STATS");
    ImGui::Text("ticks: %llu", (unsigned long long)app.aim_stats.ticks);
    ImGui::Text("selected: %llu", (unsigned long long)app.aim_stats.targets_selected);
    ImGui::Text("lost: %llu", (unsigned long long)app.aim_stats.targets_lost);
    ImGui::Text("mouse events: %llu", (unsigned long long)app.aim_stats.mouse_events_sent);
}

} // namespace vanta::ui
