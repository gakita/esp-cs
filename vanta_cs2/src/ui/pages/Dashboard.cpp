#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"
#include "process/ModuleEnum.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_dashboard(App& app) {
    ImGui::PushFont(ImGui::GetFont());
    ImGui::TextUnformatted("Dashboard");
    ImGui::TextDisabled("At-a-glance status of every subsystem.");
    ImGui::PopFont();
    section_header("PROCESS");

    const bool attached = app.process_reader().is_attached();
    const bool alive    = app.process_reader().is_process_alive();
    const bool focused  = app.game_focused.load();
    status_dot(attached ? Palette::success : Palette::error,
               attached ? (alive ? "cs2.exe attached" : "cs2.exe exited") : "cs2.exe not running");
    status_dot(focused ? Palette::success : Palette::warning,
               focused ? "game window focused" : "game window NOT focused");
    ImGui::TextDisabled("pid: %u", app.process_reader().pid());

    section_header("MODULES");
    for (auto& m : app.module_checks) {
        ImU32 c = Palette::text_dim;
        if (m.required) c = m.present ? Palette::success : Palette::error;
        else            c = m.present ? Palette::secondary : Palette::text_dim;
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%-20s %s", m.name.c_str(),
                      m.present ? "present" : (m.required ? "MISSING" : "optional/absent"));
        status_dot(c, buf);
    }

    section_header("OFFSETS");
    if (auto a = app.offsets().active_cached()) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "active bundle: %s  build: %s  state: %s",
                      a->source_name.c_str(), a->game_build.c_str(),
                      offsets::compat_label(a->overall()));
        ImGui::TextWrapped("%s", buf);
        ImGui::TextDisabled("sha256: %s", a->sha256_hex.c_str());
        ImGui::TextDisabled("fields: %zu", a->fields.size());
    } else {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Palette::warning),
                           "No active offset bundle. Add a Source on the Source Manager page.");
    }

    section_header("FEATURES");
    bool c = app.esp_cfg.enabled;    if (pill_toggle("esp.enabled",    &c)) app.esp_cfg.enabled    = c;
    ImGui::SameLine(); ImGui::TextUnformatted("ESP");
    c = app.aim_cfg.enabled;         if (pill_toggle("aim.enabled",    &c)) app.aim_cfg.enabled    = c;
    ImGui::SameLine(); ImGui::TextUnformatted("Aimbot");
    c = app.trig_cfg.enabled;        if (pill_toggle("trig.enabled",   &c)) app.trig_cfg.enabled   = c;
    ImGui::SameLine(); ImGui::TextUnformatted("Trigger");
    c = app.radar_cfg.enabled;       if (pill_toggle("radar.enabled",  &c)) app.radar_cfg.enabled  = c;
    ImGui::SameLine(); ImGui::TextUnformatted("Radar");
    c = app.xhair_cfg.enabled;       if (pill_toggle("xhair.enabled",  &c)) app.xhair_cfg.enabled  = c;
    ImGui::SameLine(); ImGui::TextUnformatted("Crosshair");

    section_header("INPUT");
    bool panicked = app.input().is_panicked();
    status_dot(panicked ? Palette::error : Palette::success,
               panicked ? "PANIC active — all injection disabled" : "injection enabled");
    if (panicked && ImGui::Button("Clear PANIC")) app.input().unpanic();
}

} // namespace vanta::ui
