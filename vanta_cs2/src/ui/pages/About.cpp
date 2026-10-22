#include "ui/UI.hpp"
#include "ui/Theme.hpp"
#include "App.hpp"

#include <imgui.h>

namespace vanta::ui {

void draw_about(App& /*app*/) {
    ImGui::TextUnformatted("VANTA CS2");
    ImGui::TextDisabled("version 0.1.0 — single-player research build");
    section_header("STACK");
    ImGui::BulletText("C++20 / MSVC x64");
    ImGui::BulletText("Direct3D 11 + Dear ImGui + Win32 overlay");
    ImGui::BulletText("CMake + vcpkg manifest");
    ImGui::BulletText("Dependencies: nlohmann-json, spdlog, imgui, libcurl, OpenSSL, Catch2");
    section_header("REPOSITORY LAYOUT");
    ImGui::TextWrapped(
        "src/{core,math,process,game,offsets,overlay,esp,aim,extras,input,profile,diag,ui} + tests/"
        "\nCMakeLists.txt at repo root. See README.md for build instructions.");
    section_header("STORAGE");
    ImGui::TextWrapped(
        "All runtime state lives at %%LOCALAPPDATA%%\\VantaCS2 — "
        "settings.json, offset-sources.json, offsets\\active.json + history.json, profiles\\*.json, "
        "logs\\vanta.log*, reports\\*.json.");
}

} // namespace vanta::ui
