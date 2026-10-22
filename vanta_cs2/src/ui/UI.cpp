#include "ui/UI.hpp"

#include "core/Config.hpp"
#include "ui/Theme.hpp"

#include <cctype>
#include <string>

#include <imgui.h>

namespace vanta::ui {

const char* page_name(Page p) {
    switch (p) {
        case Page::Dashboard:          return "Dashboard";
        case Page::ESP:                return "ESP";
        case Page::Aim:                return "Aimbot";
        case Page::TriggerRecoil:      return "Trigger & Recoil";
        case Page::RadarExtras:        return "Radar & Extras";
        case Page::OffsetManager:      return "Offset Manager";
        case Page::OffsetCatalog:      return "Offset Catalog";
        case Page::SourceManager:      return "Source Manager";
        case Page::VersionHistory:     return "Version History";
        case Page::Profiles:           return "Profiles";
        case Page::Hotkeys:            return "Hotkeys";
        case Page::InterfaceSettings:  return "Interface";
        case Page::Diagnostics:        return "Diagnostics";
        case Page::About:              return "About";
        default: return "?";
    }
}

static Page page_from_settings() {
    auto s = config::Settings::instance().get("ui.page", "dashboard").get<std::string>();
    for (int i = 0; i < (int)Page::Count; ++i) {
        auto p = (Page)i;
        std::string key = page_name(p);
        for (auto& c : key) c = (char)std::tolower((unsigned char)c);
        if (key.find(s) != std::string::npos || s == key.substr(0, s.size())) return p;
    }
    return Page::Dashboard;
}

static void save_page(Page p) {
    std::string n = page_name(p);
    for (auto& c : n) c = (char)std::tolower((unsigned char)c);
    config::Settings::instance().set("ui.page", n);
}

void draw_root(App& app, bool& panel_open) {
    if (!panel_open) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = io.DisplaySize;
    center.x *= 0.5f; center.y *= 0.5f;
    ImGui::SetNextWindowSize({kWindowDefaultW, kWindowDefaultH}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, {0.5f, 0.5f});

    const ImGuiWindowFlags wf = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
    if (!ImGui::Begin("VANTA  //  CS2", &panel_open, wf)) {
        ImGui::End();
        return;
    }

    static Page current = page_from_settings();

    ImGui::BeginChild("##sidebar", {kSidebarWidth, 0}, true,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Palette::accent));
    ImGui::TextUnformatted("V A N T A");
    ImGui::PopStyleColor();
    ImGui::TextDisabled("CS2  //  0.1.0");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const Page order[] = {
        Page::Dashboard, Page::ESP, Page::Aim, Page::TriggerRecoil, Page::RadarExtras,
        Page::OffsetManager, Page::OffsetCatalog, Page::SourceManager, Page::VersionHistory,
        Page::Profiles, Page::Hotkeys, Page::InterfaceSettings, Page::Diagnostics, Page::About
    };
    for (auto p : order) {
        bool sel = (p == current);
        if (ImGui::Selectable(page_name(p), sel, 0, {0, 24})) {
            current = p;
            save_page(current);
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##content", {0, 0}, false);
    switch (current) {
        case Page::Dashboard:         draw_dashboard(app); break;
        case Page::ESP:               draw_esp(app); break;
        case Page::Aim:               draw_aim(app); break;
        case Page::TriggerRecoil:     draw_trigger_recoil(app); break;
        case Page::RadarExtras:       draw_radar_extras(app); break;
        case Page::OffsetManager:     draw_offset_manager(app); break;
        case Page::OffsetCatalog:     draw_offset_catalog(app); break;
        case Page::SourceManager:     draw_source_manager(app); break;
        case Page::VersionHistory:    draw_version_history(app); break;
        case Page::Profiles:          draw_profiles(app); break;
        case Page::Hotkeys:           draw_hotkeys(app); break;
        case Page::InterfaceSettings: draw_interface_settings(app); break;
        case Page::Diagnostics:       draw_diagnostics(app); break;
        case Page::About:             draw_about(app); break;
        default: break;
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace vanta::ui
