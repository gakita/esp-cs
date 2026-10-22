#include "renderer.hpp"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <dxgi1_2.h>

namespace overlay {

bool Renderer::init(HWND hwnd) {
    hwnd_ = hwnd;
    const D3D_FEATURE_LEVEL want[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL got{};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            want, 1, D3D11_SDK_VERSION, &dev_, &got, &ctx_))) return false;

    IDXGIDevice*   dxgi_dev = nullptr;
    IDXGIAdapter*  adapter  = nullptr;
    IDXGIFactory2* factory  = nullptr;
    dev_->QueryInterface(IID_PPV_ARGS(&dxgi_dev));
    dxgi_dev->GetAdapter(&adapter);
    adapter->GetParent(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SampleDesc.Count = 1;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode  = DXGI_ALPHA_MODE_PREMULTIPLIED;

    IDXGISwapChain1* sc1 = nullptr;
    HRESULT hr = factory->CreateSwapChainForHwnd(dev_, hwnd, &sd, nullptr, nullptr, &sc1);
    if (adapter) adapter->Release();
    if (dxgi_dev) dxgi_dev->Release();
    if (factory) factory->Release();
    if (FAILED(hr)) return false;
    sc_ = sc1;
    create_rtv();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev_, ctx_);
    return true;
}
void Renderer::shutdown() {
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    release_rtv();
    if (sc_)  { sc_->Release();  sc_  = nullptr; }
    if (ctx_) { ctx_->Release(); ctx_ = nullptr; }
    if (dev_) { dev_->Release(); dev_ = nullptr; }
}
void Renderer::create_rtv() {
    ID3D11Texture2D* bb{}; sc_->GetBuffer(0, IID_PPV_ARGS(&bb));
    if (bb) { dev_->CreateRenderTargetView(bb, nullptr, &rtv_); bb->Release(); }
}
void Renderer::release_rtv() { if (rtv_) { rtv_->Release(); rtv_ = nullptr; } }
void Renderer::resize(int w, int h) {
    if (!sc_ || (w == w_ && h == h_) || w <= 0 || h <= 0) return;
    release_rtv();
    sc_->ResizeBuffers(0, UINT(w), UINT(h), DXGI_FORMAT_UNKNOWN, 0);
    create_rtv();
    w_ = w; h_ = h;
}
void Renderer::begin_frame() {
    ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
    const float clear[4]{0,0,0,0};
    ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
    ctx_->ClearRenderTargetView(rtv_, clear);
}
void Renderer::end_frame(int, int) {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    sc_->Present(1, 0);
}

}