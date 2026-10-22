#include "App.hpp"

#include "core/Config.hpp"
#include "core/FilePaths.hpp"
#include "core/Log.hpp"
#include "ui/UI.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <imgui.h>
#include <windows.h>

using clock_t_ = std::chrono::steady_clock;

// Debug counters defined in EntityReader.cpp.
extern std::uint32_t g_dbg_scanned, g_dbg_accepted;
extern std::uintptr_t g_dbg_system, g_dbg_page0, g_dbg_ent0;

namespace vanta {

App::App() {
    log::init(paths::logs_dir().string());
    config::Settings::instance().load();
    m_hotkeys.load_from_settings();
    m_profiles.refresh();

    load_configs();

    if (m_hotkeys.entries.find("panel.toggle") == m_hotkeys.entries.end()) {
        input::Hotkey h; h.vk = VK_INSERT; m_hotkeys.entries["panel.toggle"] = h;
    }
    if (m_hotkeys.entries.find("panic.kill") == m_hotkeys.entries.end()) {
        input::Hotkey h; h.vk = VK_END; m_hotkeys.entries["panic.kill"] = h;
    }
    m_hotkeys.save_to_settings();
}

App::~App() {
    save_configs();
    config::Settings::instance().save();
}

void App::load_configs() {
    auto& s = config::Settings::instance();
    esp_cfg.enabled    = s.get("esp.enabled",    false);
    aim_cfg.enabled    = s.get("aim.enabled",    false);
    trig_cfg.enabled   = s.get("trigger.enabled",false);
    recoil_cfg.enabled = s.get("recoil.enabled", false);
    radar_cfg.enabled  = s.get("radar.enabled",  false);
    xhair_cfg.enabled  = s.get("crosshair.enabled", false);
    bhop_cfg.enabled   = s.get("bhop.enabled",   false);
}

void App::save_configs() {
    auto& s = config::Settings::instance();
    s.set("esp.enabled",       esp_cfg.enabled);
    s.set("aim.enabled",       aim_cfg.enabled);
    s.set("trigger.enabled",   trig_cfg.enabled);
    s.set("recoil.enabled",    recoil_cfg.enabled);
    s.set("radar.enabled",     radar_cfg.enabled);
    s.set("crosshair.enabled", xhair_cfg.enabled);
    s.set("bhop.enabled",      bhop_cfg.enabled);
}

void App::attempt_attach() {
    if (m_proc.is_attached() && m_proc.is_process_alive()) return;
    m_proc.detach();
    if (!m_proc.attach(L"cs2.exe")) return;
    module_checks = process::verify_modules(m_proc);
    if (!process::required_modules_ok(module_checks)) {
        VLOG_WARN("some required CS2 modules missing");
    }
    HWND target = overlay::find_target_window(L"cs2.exe");
    m_window.set_target(target);
}

int App::run() {
    if (!m_window.create(L"VantaOverlayCls", L"VantaCS2")) return 1;
    if (!m_renderer.initialize(m_window.hwnd()))           return 2;
    if (!m_imgui.initialize(m_window.hwnd(),
                            m_renderer.device(),
                            m_renderer.context()))         return 3;

    m_reader = std::make_unique<game::EntityReader>(m_proc, m_offs, m_state, game_focused);
    m_reader->start(240);

    bool running = true;
    auto last_frame = clock_t_::now();

    while (running) {
        running = m_window.pump_and_tick([&]{
            attempt_attach();

            const auto now = clock_t_::now();
            const float dt = std::chrono::duration<float>(now - last_frame).count();
            last_frame = now;

            m_renderer.on_resize(m_window.client_w(), m_window.client_h());

            static bool prev_toggle = false;
            const auto& htoggle = m_hotkeys.entries["panel.toggle"];
            const auto& hpanic  = m_hotkeys.entries["panic.kill"];
            bool now_toggle = input::is_hotkey_down(htoggle);
            if (now_toggle && !prev_toggle) {
                m_panel_open = !m_panel_open;
                m_window.set_click_through(!m_panel_open);
            }
            prev_toggle = now_toggle;

            if (input::is_hotkey_down(hpanic)) m_input.panic();

            auto snap = m_state.current();
            game::Snapshot empty;
            if (!snap) snap = std::make_shared<game::Snapshot>(empty);
            auto s_mut = std::const_pointer_cast<game::Snapshot>(snap);
            s_mut->client_w = (float)m_window.client_w();
            s_mut->client_h = (float)m_window.client_h();
            game_focused.store(m_window.target_focused());

            math::Angles view{};
            if (auto client = m_proc.find_module("client.dll")) {
                bool ok=false;
                auto o_va = m_offs.resolve("dwViewAngles", &ok);
                if (ok) {
                    auto va = m_proc.read<math::Vec3>(client->base + o_va);
                    if (va) { view.pitch = va->x; view.yaw = va->y; view.roll = va->z; }
                }
            }

            const bool fire_held = input::is_pressed(VK_LBUTTON);

            const auto& h_aim    = m_hotkeys.entries["aim.engage"];
            const auto& h_trig   = m_hotkeys.entries["trigger.engage"];
            const auto& h_bhop   = m_hotkeys.entries["bhop.engage"];
            const bool  aim_down = input::is_hotkey_down(h_aim);
            const bool  trg_down = input::is_hotkey_down(h_trig);
            const bool  bh_down  = input::is_hotkey_down(h_bhop);

            auto aim_out = m_aim.tick(*snap, aim_cfg, view, sensitivity,
                                      now, dt, aim_down, fire_held);
            if (aim_out.active) m_input.send_mouse_move(aim_out.mouse);
            aim_stats = m_aim.stats();

            auto trg_out = m_trigger.tick(*snap, trig_cfg, view, now, trg_down);
            if (trg_out.fire)    m_input.send_mouse_down(0);
            if (trg_out.release) m_input.send_mouse_up(0);

            auto rec_out = m_recoil.tick(*snap, recoil_cfg, sensitivity, fire_held);
            if (rec_out.active) m_input.send_mouse_move(rec_out.mouse);

            auto bh_out = m_bhop.tick(*snap, bhop_cfg, bh_down, true);
            if (bh_out.send_jump)    m_input.send_key_down(VK_SPACE);
            if (bh_out.release_jump) m_input.send_key_up(VK_SPACE);

            const auto r0 = clock_t_::now();
            m_imgui.new_frame();

            ImDrawList* bg = ImGui::GetBackgroundDrawList();

            // === Debug HUD (top-left) ================================
            {
                char line1[320], line2[256];
                std::snprintf(line1, sizeof(line1),
                    "VANTA | focus:%s attach:%s local:%s players:%zu scan:%u acc:%u fail:%llu",
                    snap->game_focused ? "Y" : "N",
                    snap->attached     ? "Y" : "N",
                    snap->local        ? "Y" : "N",
                    snap->players.size(),
                    g_dbg_scanned, g_dbg_accepted,
                    (unsigned long long)m_proc.read_fail_count());
                std::snprintf(line2, sizeof(line2),
                    "sys:0x%llX page0:0x%llX ent0:0x%llX",
                    (unsigned long long)g_dbg_system,
                    (unsigned long long)g_dbg_page0,
                    (unsigned long long)g_dbg_ent0);
                bg->AddRectFilled({4, 4}, {900, 40}, IM_COL32(0,0,0,180), 4);
                bg->AddText({10, 6},  IM_COL32(103, 232, 249, 255), line1);
                bg->AddText({10, 22}, IM_COL32(167, 139, 250, 255), line2);
            }

            m_esp.render(*snap, esp_cfg, bg);
            esp_stats = m_esp.last_stats();

            m_radar.render(*snap, radar_cfg, view, bg);
            radar_stats = m_radar.stats();

            m_xhair.render(bg, (float)m_window.client_w(),
                               (float)m_window.client_h(), xhair_cfg);

            if (aim_cfg.enabled && aim_cfg.draw_fov_circle) {
                const float fov_px = ((float)m_window.client_h() * 0.5f) *
                                     std::tan(aim_cfg.fov_deg * math::kDegRad);
                bg->AddCircle({(float)m_window.client_w() * 0.5f,
                               (float)m_window.client_h() * 0.5f},
                              fov_px, IM_COL32(167, 139, 250, 180), 64, 1.0f);
            }

            ui::draw_root(*this, m_panel_open);

            m_renderer.begin_frame(0, 0, 0, 0);
            m_imgui.render(nullptr);
            m_renderer.end_frame(true);

            const float render_ms =
                std::chrono::duration<float, std::milli>(clock_t_::now() - r0).count();
            const float read_ms =
                std::chrono::duration<float, std::milli>(
                    m_reader->last_tick_duration()).count();
            frame_tick(render_ms, read_ms);
        });
    }

    m_reader->stop();
    m_imgui.shutdown();
    m_renderer.shutdown();
    m_window.destroy();
    return 0;
}

void App::frame_tick(float render_ms, float read_ms) {
    auto snap = m_state.current();
    const std::uint32_t ents = snap ?
        (std::uint32_t)snap->players.size() + (snap->local ? 1 : 0) : 0;
    m_diag.record_frame(render_ms, read_ms, ents);
}

} // namespace vanta
