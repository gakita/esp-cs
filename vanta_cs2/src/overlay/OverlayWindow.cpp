#include "overlay/OverlayWindow.hpp"

#include "core/Log.hpp"

#include <dwmapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "dwmapi.lib")

#include <imgui.h>
#include <imgui_impl_win32.h>
// Forward-declare — vcpkg's imgui header surface sometimes hides it.
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace vanta::overlay {

static OverlayWindow* g_this = nullptr;

LRESULT CALLBACK OverlayWindow::wnd_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return 1;
    switch (m) {
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_NCHITTEST:
            // Transparent to mouse when click_through is on, otherwise normal.
            if (g_this && g_this->m_click_through) return HTTRANSPARENT;
            break;
    }
    return ::DefWindowProcW(h, m, w, l);
}

bool OverlayWindow::create(const wchar_t* cls, const wchar_t* title) {
    g_this = this;
    WNDCLASSEXW wc{sizeof(wc)};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &OverlayWindow::wnd_proc;
    wc.hInstance     = ::GetModuleHandleW(nullptr);
    wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = cls;
    if (!::RegisterClassExW(&wc)) {
        VLOG_ERROR("RegisterClassExW failed: {}", ::GetLastError());
        return false;
    }

    const DWORD ex_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW;
    const DWORD style    = WS_POPUP | WS_VISIBLE;

    m_hwnd = ::CreateWindowExW(ex_style, cls, title, style,
                               0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                               nullptr, nullptr, wc.hInstance, nullptr);
    if (!m_hwnd) {
        VLOG_ERROR("CreateWindowExW failed: {}", ::GetLastError());
        return false;
    }

    MARGINS mg{-1, -1, -1, -1};
    ::DwmExtendFrameIntoClientArea(m_hwnd, &mg);
    ::SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::UpdateWindow(m_hwnd);

    RECT r{};
    ::GetClientRect(m_hwnd, &r);
    m_client_w = r.right - r.left;
    m_client_h = r.bottom - r.top;
    return true;
}

void OverlayWindow::destroy() {
    if (m_hwnd) {
        ::DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void OverlayWindow::set_target(HWND t) { m_target = t; }

void OverlayWindow::set_click_through(bool e) {
    m_click_through = e;
    if (!m_hwnd) return;
    LONG_PTR ex = ::GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
    if (e) ex |=  WS_EX_TRANSPARENT; else ex &= ~WS_EX_TRANSPARENT;
    ::SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, ex);
}

void OverlayWindow::sync_to_target() {
    if (!m_target || !::IsWindow(m_target)) {
        m_target_focused.store(false);
        return;
    }
    RECT r{};
    if (!::GetClientRect(m_target, &r)) return;
    POINT tl{r.left, r.top};
    ::ClientToScreen(m_target, &tl);
    const int w = r.right - r.left;
    const int h = r.bottom - r.top;
    if (w <= 0 || h <= 0) return;

    ::SetWindowPos(m_hwnd, HWND_TOPMOST, tl.x, tl.y, w, h,
                   SWP_NOACTIVATE | SWP_SHOWWINDOW);
    m_client_w = w;
    m_client_h = h;
    m_target_focused.store(::GetForegroundWindow() == m_target);
}

bool OverlayWindow::pump_and_tick(const TickFn& fn) {
    MSG msg{};
    while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    sync_to_target();
    if (fn) fn();
    return true;
}

HWND find_target_window(const wchar_t* exe_image) {
    DWORD pid = 0;
    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W e{sizeof(e)};
        if (::Process32FirstW(snap, &e)) {
            do {
                if (_wcsicmp(e.szExeFile, exe_image) == 0) { pid = e.th32ProcessID; break; }
            } while (::Process32NextW(snap, &e));
        }
        ::CloseHandle(snap);
    }
    if (!pid) return nullptr;

    struct Ctx { DWORD pid; HWND out; };
    Ctx ctx{pid, nullptr};
    ::EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* c = reinterpret_cast<Ctx*>(lp);
        DWORD p = 0;
        ::GetWindowThreadProcessId(h, &p);
        if (p == c->pid && ::IsWindowVisible(h) && !::GetWindow(h, GW_OWNER)) {
            RECT r{};
            if (::GetClientRect(h, &r) && (r.right - r.left) > 100) {
                c->out = h; return FALSE;
            }
        }
        return TRUE;
    }, (LPARAM)&ctx);
    return ctx.out;
}

} // namespace vanta::overlay
