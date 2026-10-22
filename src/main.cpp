#include "memory/process.hpp"
#include "overlay/window.hpp"
#include "overlay/renderer.hpp"
#include "features/entity_cache.hpp"
#include "features/esp.hpp"
#include "features/aimbot.hpp"
#include "input/hotkey.hpp"
#include "imgui.h"
#include <Windows.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;
static bool g_show_menu = true;

static void menu(features::EspConfig& esp, features::AimbotConfig& aim, bool& running) {
    if (!g_show_menu) return;
    ImGui::SetNextWindowSize({360, 500}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Vanta", &g_show_menu);
    if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled",    &esp.enabled);
        ImGui::Checkbox("Box",        &esp.box);
        ImGui::Checkbox("Health bar", &esp.health);
        ImGui::Checkbox("Distance",   &esp.distance);
        ImGui::Checkbox("Snapline",   &esp.snapline);
        ImGui::Checkbox("Team check", &esp.team_check);
        ImGui::SliderFloat("Max dist (m)", &esp.max_distance, 10.f, 500.f, "%.0f");
    }
    if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled",            &aim.enabled);
        ImGui::Checkbox("Require hold",       &aim.require_hold);
        ImGui::InputInt("Hotkey VK",          &aim.hotkey_vk);
        ImGui::SliderFloat("Sensitivity",     &aim.sensitivity, 0.1f, 6.f, "%.2f");
        ImGui::SliderFloat("FOV (deg)",       &aim.fov_degrees, 1.f, 30.f, "%.1f");
        ImGui::SliderFloat("Smoothing",       &aim.smoothing,   0.f, 0.95f, "%.2f");
        ImGui::SliderFloat("Max px/frm",      &aim.max_px_frame, 4.f, 200.f, "%.0f");
        ImGui::SliderFloat("Jitter",          &aim.jitter_px,    0.f, 3.f,  "%.1f");
        ImGui::Checkbox("Head",               &aim.target_head);
        ImGui::Checkbox("Require valid bones",&aim.require_valid_bones);
    }
    ImGui::Separator();
    if (ImGui::Button("Exit")) running = false;
    ImGui::SameLine(); ImGui::TextDisabled("INS=menu  END=exit");
    ImGui::End();
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    mem::Process proc;
    while (!proc.attach(L"cs2.exe")) std::this_thread::sleep_for(500ms);

    overlay::Window win; if (!win.create()) return 1;
    overlay::Renderer gfx; if (!gfx.init(win.hwnd())) return 2;

    features::EntityCache cache; cache.start(proc);
    features::EspConfig    esp_cfg{};
    features::AimbotConfig aim_cfg{};
    features::Aimbot       aim;

    bool running = true;
    while (running) {
        if (!win.pump(proc.window())) break;
        gfx.resize(win.width(), win.height());
        if (input::key_pressed(VK_INSERT)) g_show_menu = !g_show_menu;
        if (input::key_pressed(VK_END))    break;

        overlay::set_clickthrough(win.hwnd(), !g_show_menu);

        const auto& snap = cache.snapshot();
        aim.tick(snap, aim_cfg);

        gfx.begin_frame();
        features::draw_esp(ImGui::GetBackgroundDrawList(), snap, esp_cfg);
        menu(esp_cfg, aim_cfg, running);
        gfx.end_frame(win.width(), win.height());
    }
    cache.stop(); gfx.shutdown(); win.destroy();
    return 0;
}