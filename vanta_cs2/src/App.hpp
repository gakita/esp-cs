#pragma once
#include "aim/AimAssist.hpp"
#include "aim/RecoilControl.hpp"
#include "aim/Trigger.hpp"
#include "diag/Diagnostics.hpp"
#include "esp/ESP.hpp"
#include "extras/BunnyHop.hpp"
#include "extras/Crosshair.hpp"
#include "extras/Radar.hpp"
#include "game/EntityReader.hpp"
#include "game/GameState.hpp"
#include "input/Hotkey.hpp"
#include "input/InputManager.hpp"
#include "offsets/OffsetManager.hpp"
#include "overlay/ImGuiBackend.hpp"
#include "overlay/OverlayRenderer.hpp"
#include "overlay/OverlayWindow.hpp"
#include "process/ProcessReader.hpp"
#include "profile/ProfileManager.hpp"

#include <atomic>
#include <memory>

namespace vanta {

// Top-level owner. Builds every subsystem and runs the main loop.
// All UI pages reach their state via this object.
class App {
public:
    App();
    ~App();

    // Starts the overlay window + reader thread + enters the main loop.
    // Returns a process exit code.
    int run();

    // --- state exposed to UI pages ----------------------------------------
    process::ProcessReader&   process_reader()  { return m_proc; }
    offsets::OffsetManager&   offsets()         { return m_offs; }
    profile::ProfileManager&  profiles()        { return m_profiles; }
    input::HotkeyMap&         hotkeys()         { return m_hotkeys; }
    input::InputManager&      input()           { return m_input; }
    diag::Diagnostics&        diagnostics()     { return m_diag; }
    game::GameState&          state()           { return m_state; }

    // Live configurations (edited by UI pages, read by hot path).
    esp::ESPConfig            esp_cfg;
    aim::AimConfig            aim_cfg;
    aim::TriggerConfig        trig_cfg;
    aim::RecoilConfig         recoil_cfg;
    extras::RadarConfig       radar_cfg;
    extras::CrosshairConfig   xhair_cfg;
    extras::BunnyHopConfig    bhop_cfg;

    // Latest module check (refreshed each attach / periodically).
    std::vector<process::ModuleCheck> module_checks;

    // Latest draw stats (populated by hot path).
    esp::ESP::DrawStats       esp_stats;
    extras::RadarStats        radar_stats;
    aim::AimStats             aim_stats;

    // Game sensitivity, read from attached process when available.
    // Defaults to 1.0 so manual override on the UI stays usable.
    float sensitivity{1.0f};

    // Target process + overlay bookkeeping.
    std::atomic<bool> game_focused{false};

private:
    void load_configs();
    void save_configs();
    void attempt_attach();
    void frame_tick(float render_ms, float read_ms);

    overlay::OverlayWindow   m_window;
    overlay::OverlayRenderer m_renderer;
    overlay::ImGuiBackend    m_imgui;

    process::ProcessReader   m_proc;
    offsets::OffsetManager   m_offs;
    profile::ProfileManager  m_profiles;
    input::HotkeyMap         m_hotkeys;
    input::InputManager      m_input;
    diag::Diagnostics        m_diag;

    game::GameState          m_state;
    std::unique_ptr<game::EntityReader> m_reader;

    aim::AimAssist           m_aim;
    aim::Trigger             m_trigger;
    aim::RecoilControl       m_recoil;
    esp::ESP                 m_esp;
    extras::Radar            m_radar;
    extras::Crosshair        m_xhair;
    extras::BunnyHop         m_bhop;

    bool m_panel_open{true};
};

} // namespace vanta
