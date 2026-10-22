#pragma once
#include "App.hpp"

namespace vanta::ui {

enum class Page {
    Dashboard,
    ESP,
    Aim,
    TriggerRecoil,
    RadarExtras,
    OffsetManager,
    OffsetCatalog,
    SourceManager,
    VersionHistory,
    Profiles,
    Hotkeys,
    InterfaceSettings,
    Diagnostics,
    About,
    Count,
};

const char* page_name(Page p);

void draw_root(App& app, bool& panel_open);

// Page draw signatures live inside the implementation files.
void draw_dashboard(App&);
void draw_esp(App&);
void draw_aim(App&);
void draw_trigger_recoil(App&);
void draw_radar_extras(App&);
void draw_offset_manager(App&);
void draw_offset_catalog(App&);
void draw_source_manager(App&);
void draw_version_history(App&);
void draw_profiles(App&);
void draw_hotkeys(App&);
void draw_interface_settings(App&);
void draw_diagnostics(App&);
void draw_about(App&);

} // namespace vanta::ui
