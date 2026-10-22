#pragma once
#include <Windows.h>
namespace overlay {
class Window {
public:
    bool create(); void destroy();
    bool pump(HWND target);
    HWND hwnd() const { return hwnd_; }
    int  width() const { return width_; }
    int  height() const { return height_; }
private:
    HWND hwnd_{nullptr}; int width_{0}, height_{0};
};
void set_clickthrough(HWND h, bool on);
}