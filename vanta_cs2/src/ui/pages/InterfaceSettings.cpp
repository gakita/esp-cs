#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"
#include "core/Config.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_interface_settings(App& /*app*/) {
    ImGui::TextUnformatted("Interface");

    auto& s = config::Settings::instance();
    float dpi = s.get("ui.dpi_scale", 1.0f);
    if (ImGui::SliderFloat("DPI scale", &dpi, 0.5f, 2.5f, "%.2fx")) {
        s.set("ui.dpi_scale", dpi);
        ImGui::GetIO().FontGlobalScale = dpi;
    }

    section_header("DEMO PREVIEW");
    ImGui::TextDisabled("Fake game frame to style ESP without CS2 running.");
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = 500, h = 300;
    dl->AddRectFilled(p, {p.x + w, p.y + h}, Palette::surface, 8);
    dl->AddRect(p, {p.x + w, p.y + h}, Palette::border, 8);
    // Fake player box.
    dl->AddRect({p.x + 120, p.y + 60}, {p.x + 180, p.y + 230},
                IM_COL32(255, 90, 100, 240), 0, 0, 1.6f);
    dl->AddText({p.x + 110, p.y + 234}, IM_COL32(255,255,255,220), "demo-enemy");
    dl->AddLine({p.x + w/2, p.y + h}, {p.x + 150, p.y + 230},
                IM_COL32(255, 90, 100, 180), 1.0f);
    dl->AddText({p.x + 8, p.y + h - 18}, Palette::text_dim, "DEMO PREVIEW — not from the running game");
    ImGui::Dummy({w, h});
}

} // namespace vanta::ui
