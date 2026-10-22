// src/main.cpp — Vanta entry point.
// All subsystems (overlay, renderer, EntityReader, UI, AimAssist, …) are
// owned by vanta::App. We just construct it and run.
#include "App.hpp"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    vanta::App app;
    return app.run();
}