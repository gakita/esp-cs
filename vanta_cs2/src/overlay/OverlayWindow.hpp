#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <windows.h>

namespace vanta::overlay {

// Transparent, layered, click-through-by-default top-most window that tracks
// a target window's position and client size each frame.
class OverlayWindow {
public:
    using TickFn = std::function<void()>;

    bool create(const wchar_t* class_name, const wchar_t* title);
    void destroy();

    // Target to track (the CS2 top-level window). nullptr detaches.
    void set_target(HWND target);
    HWND target() const { return m_target; }

    // Pumps messages + runs the user's tick. Returns false when the message
    // loop requests exit (WM_QUIT / WM_CLOSE).
    bool pump_and_tick(const TickFn& fn);

    // Toggles whether mouse events pass through to the game under us
    // (default true). When the user opens the ImGui panel, set this to false.
    void set_click_through(bool enable);

    HWND hwnd() const { return m_hwnd; }
    int  client_w() const { return m_client_w; }
    int  client_h() const { return m_client_h; }
    bool target_focused() const { return m_target_focused; }

private:
    static LRESULT CALLBACK wnd_proc(HWND h, UINT m, WPARAM w, LPARAM l);
    void sync_to_target();

    HWND m_hwnd{nullptr};
    HWND m_target{nullptr};
    int  m_client_w{0};
    int  m_client_h{0};
    bool m_click_through{true};
    std::atomic<bool> m_target_focused{false};
};

// Finds the top-level window belonging to a process with the given image name.
HWND find_target_window(const wchar_t* exe_image);

} // namespace vanta::overlay
