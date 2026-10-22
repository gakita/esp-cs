cmake_minimum_required(VERSION 3.20)
project(cs_cheat CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "Windows only")
endif()

add_compile_definitions(
    WIN32_LEAN_AND_MEAN NOMINMAX
    _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE
)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

add_executable(cs_cheat WIN32
    src/main.cpp
    src/memory/process.cpp
    src/overlay/window.cpp
    src/overlay/renderer.cpp
    src/features/entity_cache.cpp
    src/features/esp.cpp
    src/features/aimbot.cpp
    src/input/mouse.cpp
    src/input/hotkey.cpp
)
target_include_directories(cs_cheat PRIVATE src)
target_link_libraries(cs_cheat PRIVATE
    imgui d3d11 dxgi dwmapi d3dcompiler
    user32 gdi32 ole32 shlwapi
)
target_compile_options(cs_cheat PRIVATE /W4 /permissive- /Zc:__cplusplus)