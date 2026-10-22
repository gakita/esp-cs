#include <algorithm>
#include "overlay/OverlayRenderer.hpp"

#include "core/Log.hpp"

#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace vanta::overlay {

bool OverlayRenderer::initialize(HWND hwnd) {
    m_hwnd = hwnd;
    RECT r{};
    ::GetClientRect(hwnd, &r);
    m_w = r.right - r.left;
    m_h = r.bottom - r.top;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width  = m_w;
    sd.BufferDesc.Height = m_h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = {60, 1};
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL requested[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        requested, (UINT)std::size(requested), D3D11_SDK_VERSION,
        &sd, m_swap.ReleaseAndGetAddressOf(),
        m_dev.ReleaseAndGetAddressOf(), &got,
        m_ctx.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        VLOG_ERROR("D3D11CreateDeviceAndSwapChain failed: 0x{:08X}", (unsigned)hr);
        return false;
    }
    create_rtv();
    return true;
}

void OverlayRenderer::shutdown() {
    m_rtv.Reset();
    m_swap.Reset();
    m_ctx.Reset();
    m_dev.Reset();
}

void OverlayRenderer::create_rtv() {
    ComPtr<ID3D11Texture2D> bb;
    if (SUCCEEDED(m_swap->GetBuffer(0, IID_PPV_ARGS(&bb)))) {
        m_dev->CreateRenderTargetView(bb.Get(), nullptr, m_rtv.ReleaseAndGetAddressOf());
    }
}

void OverlayRenderer::on_resize(int w, int h) {
    if (w == m_w && h == m_h) return;
    if (!m_swap) return;
    m_w = std::max(1, w);
    m_h = std::max(1, h);
    m_rtv.Reset();
    m_swap->ResizeBuffers(0, (UINT)m_w, (UINT)m_h, DXGI_FORMAT_UNKNOWN, 0);
    create_rtv();
}

ID3D11RenderTargetView* OverlayRenderer::begin_frame(float r, float g, float b, float a) {
    const float cc[4] = {r, g, b, a};
    m_ctx->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);
    m_ctx->ClearRenderTargetView(m_rtv.Get(), cc);
    D3D11_VIEWPORT vp{};
    vp.Width  = (float)m_w;
    vp.Height = (float)m_h;
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    m_ctx->RSSetViewports(1, &vp);
    return m_rtv.Get();
}

void OverlayRenderer::end_frame(bool vsync) {
    m_swap->Present(vsync ? 1 : 0, 0);
}

} // namespace vanta::overlay
