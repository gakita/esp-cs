#include "overlay/ImGuiBackend.hpp"

#include "ui/Theme.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

namespace vanta::overlay {

bool ImGuiBackend::initialize(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

    ui::apply_theme();

    if (!ImGui_ImplWin32_Init(hwnd))      return false;
    if (!ImGui_ImplDX11_Init(dev, ctx))   return false;

    m_hwnd = hwnd;
    m_initialized = true;
    return true;
}

void ImGuiBackend::shutdown() {
    if (!m_initialized) return;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiBackend::new_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBackend::render(ID3D11RenderTargetView* /*rtv*/) {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiBackend::on_dpi_change(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
    ImGui::GetStyle().ScaleAllSizes(1.0f); // keep base sizes; UI pages render in logical px
}

} // namespace vanta::overlay
