#include "window.hpp"
#include <dwmapi.h>
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace overlay {

static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return 1;
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

bool Window::create() {
    HINSTANCE hi = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = proc; wc.hInstance = hi;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"vanta_overlay";
    RegisterClassExW(&wc);
    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"", WS_POPUP, 0, 0, 1920, 1080,
        nullptr, nullptr, hi, nullptr);
    if (!hwnd_) return false;
    SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
    MARGINS m{-1,-1,-1,-1}; DwmExtendFrameIntoClientArea(hwnd_, &m);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE); UpdateWindow(hwnd_);
    return true;
}
void Window::destroy() { if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; } }
bool Window::pump(HWND target) {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) return false;
    }
    if (target && IsWindow(target)) {
        RECT r{};
        if (GetClientRect(target, &r)) {
            POINT p{0,0}; ClientToScreen(target, &p);
            width_  = r.right - r.left;
            height_ = r.bottom - r.top;
            SetWindowPos(hwnd_, HWND_TOPMOST, p.x, p.y, width_, height_, SWP_NOACTIVATE);
        }
    }
    return true;
}

void set_clickthrough(HWND h, bool on) {
    if (!h) return;
    LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
    if (on) ex |=  WS_EX_TRANSPARENT;
    else    ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(h, GWL_EXSTYLE, ex);
}

}