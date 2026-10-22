cmake_minimum_required(VERSION 3.26)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(VantaCS2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "VantaCS2 targets Windows x64 only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "VantaCS2 requires a 64-bit toolchain.")
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /EHsc /utf-8)
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        UNICODE _UNICODE
    )
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog        CONFIG REQUIRED)
find_package(imgui         CONFIG REQUIRED)
find_package(CURL          CONFIG REQUIRED)
find_package(OpenSSL       REQUIRED)

# ESP / Radar / Crosshair use ImGui draw lists, so vanta_core needs the header
# publicly. The D3D11/Win32 backends stay in the exe target only.

# ----------------------------------------------------------------- vanta_core
add_library(vanta_core STATIC
    src/core/Log.cpp
    src/core/Config.cpp
    src/core/FilePaths.cpp
    src/core/Hash.cpp
    src/core/AtomicFile.cpp

    src/math/WorldToScreen.cpp
    src/math/Angles.cpp

    src/process/ProcessReader.cpp
    src/process/ModuleEnum.cpp

    src/game/GameState.cpp
    src/game/EntityReader.cpp

    src/offsets/OffsetManager.cpp
    src/offsets/OffsetCache.cpp
    src/offsets/sources/GenericJsonSource.cpp

    src/esp/ESP.cpp
    src/aim/AimAssist.cpp
    src/aim/Trigger.cpp
    src/aim/RecoilControl.cpp
    src/extras/Radar.cpp
    src/extras/Crosshair.cpp
    src/extras/BunnyHop.cpp
    src/input/InputManager.cpp
    src/input/Hotkey.cpp
    src/profile/ProfileManager.cpp
    src/diag/Diagnostics.cpp
)

target_include_directories(vanta_core PUBLIC src)
target_link_libraries(vanta_core PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    imgui::imgui
    CURL::libcurl
    OpenSSL::Crypto
)
target_link_libraries(vanta_core PUBLIC psapi.lib shell32.lib ole32.lib)

# ----------------------------------------------------------------- vanta_app
add_executable(VantaCS2 WIN32
    src/main.cpp
    src/App.cpp

    src/overlay/OverlayWindow.cpp
    src/overlay/OverlayRenderer.cpp
    src/overlay/ImGuiBackend.cpp

    src/ui/Theme.cpp
    src/ui/UI.cpp
    src/ui/pages/Dashboard.cpp
    src/ui/pages/ESPPage.cpp
    src/ui/pages/AimPage.cpp
    src/ui/pages/TriggerRecoil.cpp
    src/ui/pages/RadarExtras.cpp
    src/ui/pages/OffsetPages.cpp
    src/ui/pages/Profiles.cpp
    src/ui/pages/Hotkeys.cpp
    src/ui/pages/InterfaceSettings.cpp
    src/ui/pages/DiagnosticsPage.cpp
    src/ui/pages/About.cpp
)
target_link_libraries(VantaCS2 PRIVATE
    vanta_core
    imgui::imgui
    d3d11.lib dxgi.lib d3dcompiler.lib dwmapi.lib
)

# ----------------------------------------------------------------- vanta_tests
enable_testing()
find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(vanta_tests
    tests/test_main.cpp
    tests/test_world_to_screen.cpp
    tests/test_angles.cpp
    tests/test_aim_selection.cpp
    tests/test_smoothing.cpp
    tests/test_entity_filters.cpp
    tests/test_offsets_parse.cpp
    tests/test_offsets_version.cpp
    tests/test_http_backoff.cpp
    tests/test_atomic_file.cpp
    tests/test_hotkeys.cpp
    tests/test_profiles.cpp
)
target_link_libraries(vanta_tests PRIVATE vanta_core Catch2::Catch2WithMain)
catch_discover_tests(vanta_tests)

install(TARGETS VantaCS2 RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION bin/config)
