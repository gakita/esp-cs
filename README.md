# cs_cheat — External CS2 Overlay + Aimbot

C++20, DirectX 11, external (no injection). Offsets from
sezzyaep/CS2-OFFSETS dump (build 14178, 2026-08-28).

## Build
    cmake -B build -A x64
    cmake --build build --config Release

Needs: Windows 10/11 x64, MSVC v143, Windows SDK.
ImGui is fetched by `fetch_imgui.ps1` into `src/imgui/`.

## Run
Launch CS2, then run `cs_cheat.exe` as administrator (RPM needs it).
INSERT toggles the menu, END quits.