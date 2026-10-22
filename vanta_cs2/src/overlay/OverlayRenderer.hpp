#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace vanta::overlay {

class OverlayRenderer {
public:
    bool initialize(HWND hwnd);
    void shutdown();

    // Resize swap chain to match current client rect.
    void on_resize(int w, int h);

    // Call at the start of every frame; returns the back-buffer RTV.
    ID3D11RenderTargetView* begin_frame(float clear_r, float clear_g,
                                        float clear_b, float clear_a);
    void end_frame(bool vsync);

    ID3D11Device*        device()  const { return m_dev.Get(); }
    ID3D11DeviceContext* context() const { return m_ctx.Get(); }

private:
    void create_rtv();

    HWND m_hwnd{nullptr};
    Microsoft::WRL::ComPtr<ID3D11Device>        m_dev;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_ctx;
    Microsoft::WRL::ComPtr<IDXGISwapChain>      m_swap;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    int m_w{0}, m_h{0};
};

} // namespace vanta::overlay
