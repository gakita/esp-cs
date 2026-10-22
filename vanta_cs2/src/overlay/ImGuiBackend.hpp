#pragma once
#include <d3d11.h>
#include <windows.h>

namespace vanta::overlay {

// Thin wrapper around imgui_impl_win32 + imgui_impl_dx11.
class ImGuiBackend {
public:
    bool initialize(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx);
    void shutdown();

    void new_frame();
    void render(ID3D11RenderTargetView* rtv);

    // Called from the window DPI/resize path when needed.
    void on_dpi_change(float scale);

private:
    HWND m_hwnd{nullptr};
    bool m_initialized{false};
};

} // namespace vanta::overlay
