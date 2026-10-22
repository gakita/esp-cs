#include "mouse.hpp"
#include <Windows.h>
namespace input {
void mouse_move_rel(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    INPUT in{}; in.type = INPUT_MOUSE;
    in.mi.dx = dx; in.mi.dy = dy; in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(in));
}
void mouse_click_left() {
    INPUT ev[2]{};
    ev[0].type = INPUT_MOUSE; ev[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    ev[1].type = INPUT_MOUSE; ev[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, ev, sizeof(INPUT));
}
}