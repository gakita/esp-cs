#pragma once
#include <Windows.h>
#include <d3d11.h>
namespace overlay {
class Renderer {
public:
    bool init(HWND hwnd); void shutdown();
    void resize(int w, int h);
    void begin_frame(); void end_frame(int w, int h);
private:
    void release_rtv(); void create_rtv();
    HWND hwnd_{nullptr};
    IDXGISwapChain*         sc_{nullptr};
    ID3D11Device*           dev_{nullptr};
    ID3D11DeviceContext*    ctx_{nullptr};
    ID3D11RenderTargetView* rtv_{nullptr};
    int w_{0}, h_{0};
};
}