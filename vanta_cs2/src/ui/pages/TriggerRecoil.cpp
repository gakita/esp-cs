#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_trigger_recoil(App& app) {
    ImGui::TextUnformatted("Trigger & Recoil");

    section_header("TRIGGERBOT");
    auto& t = app.trig_cfg;
    pill_toggle("trig.master", &t.enabled); ImGui::SameLine(); ImGui::TextUnformatted("Trigger enabled");
    ImGui::TextDisabled("Hotkey configured on Hotkeys page as 'trigger.engage'.");
    ImGui::SliderFloat("Delay before fire (ms)",  &t.delay_ms,   0.0f, 500.0f, "%.0f");
    ImGui::SliderFloat("Hold time (ms)",          &t.release_ms, 1.0f, 200.0f, "%.0f");
    ImGui::Checkbox("Enemies only",               &t.only_enemies);
    ImGui::Checkbox("Require scope",              &t.require_scoped);
    help_tooltip("Only fires once a scoped weapon is zoomed; requires the "
                 "C_CSPlayerPawnBase::m_bIsScoped offset in the active bundle.");

    section_header("RECOIL CONTROL");
    auto& r = app.recoil_cfg;
    pill_toggle("recoil.master", &r.enabled); ImGui::SameLine(); ImGui::TextUnformatted("Recoil enabled");
    ImGui::SliderFloat("X compensation", &r.strength_x, 0.0f, 1.5f, "%.2f");
    ImGui::SliderFloat("Y compensation", &r.strength_y, 0.0f, 1.5f, "%.2f");
    ImGui::Checkbox("Only while firing", &r.only_while_firing);
    ImGui::Checkbox("Only full-auto weapons", &r.only_full_auto);
    ImGui::Spacing();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Palette::warning),
        "This build requires m_aimPunchAngle wired through EntityReader. "
        "Until that offset is in the active bundle AND the reader surfaces it, "
        "recoil control emits no mouse motion. See docs/limitations.md.");
}

} // namespace vanta::ui
