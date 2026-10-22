#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_diagnostics(App& app) {
    ImGui::TextUnformatted("Diagnostics");

    auto snap = app.diagnostics().compute(
        app.process_reader(), app.offsets(), app.module_checks,
        app.aim_stats, app.esp_stats, app.radar_stats, app.game_focused.load());

    section_header("RENDER");
    ImGui::Text("FPS            : %.1f", snap.fps);
    ImGui::Text("render ms (avg): %.2f", snap.avg_render_ms);
    ImGui::Text("read ms (avg)  : %.2f", snap.avg_read_ms);
    ImGui::Text("entities total : %llu", (unsigned long long)snap.total_entities);
    ImGui::Text("working set    : %llu MB", (unsigned long long)snap.memory_working_set_mb);

    section_header("PROCESS");
    status_dot(snap.proc_attached ? Palette::success : Palette::error,
               snap.proc_attached ? "attached" : "not attached");
    status_dot(snap.proc_alive ? Palette::success : Palette::warning,
               snap.proc_alive ? "process alive" : "process not alive");
    ImGui::Text("required modules present: %u / %u", snap.module_present, snap.module_required);
    ImGui::Text("reads: %llu  (failed: %llu)",
                (unsigned long long)snap.read_count,
                (unsigned long long)snap.read_fail);

    section_header("OFFSETS");
    ImGui::Text("state: %s", snap.offset_state.c_str());

    section_header("ERRORS");
    if (snap.recent_errors.empty()) ImGui::TextDisabled("none");
    for (auto& e : snap.recent_errors) ImGui::TextWrapped("%s", e.c_str());

    section_header("REPORTS");
    static char reason[128] = "manual";
    ImGui::InputText("reason", reason, sizeof(reason));
    if (ImGui::Button("Export JSON report")) {
        app.diagnostics().export_report(reason);
    }
}

} // namespace vanta::ui
