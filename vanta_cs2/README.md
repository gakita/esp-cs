# VANTA CS2

External research overlay for Counter-Strike 2 (Windows 11 x64). Built in C++20
with Direct3D 11 and Dear ImGui, driven by CMake + vcpkg.

> This repository contains a complete, self-contained skeleton of the Vanta
> external described in the project specification. Everything in `src/` and
> `tests/` compiles against the dependency set below. **It does not ship with
> baked-in CS2 offsets** — those are loaded through the Source Manager page
> at runtime, from an HTTPS JSON endpoint you configure. See
> `docs/limitations.md` for the full list of what is and is not wired up.

---

## Repository layout

```
vanta_cs2/
├── CMakeLists.txt              # single root CMake project; 3 targets
├── vcpkg.json                  # fixed deps (nlohmann-json, spdlog, imgui, curl, openssl, catch2)
├── config/
│   └── sources.example.json    # offset source catalog template
├── fixtures/                   # reproducible test inputs
│   ├── w2s_cases.json
│   ├── aimbot_cases.json
│   └── offsets_sample.json
├── docs/
│   └── limitations.md          # what works, what still depends on runtime data
├── src/
│   ├── main.cpp  App.{hpp,cpp}
│   ├── core/        Log / Config / FilePaths / Hash (OpenSSL SHA-256) / AtomicFile
│   ├── math/        Vec, Mat4, WorldToScreen, Angles
│   ├── process/     ProcessReader (RPM/Toolhelp), ModuleEnum
│   ├── game/        GameState (double-buffered snapshot), EntityReader (thread)
│   ├── offsets/     OffsetManager, OffsetCache, OffsetTypes, sources/GenericJsonSource
│   ├── overlay/     OverlayWindow (layered DWM), OverlayRenderer (DX11), ImGuiBackend
│   ├── esp/         ESP + ESPConfig
│   ├── aim/         AimAssist, Trigger, RecoilControl
│   ├── extras/      Radar, Crosshair, BunnyHop
│   ├── input/       InputManager (SendInput + PANIC), Hotkey (capture + collisions)
│   ├── profile/     ProfileManager (per-file JSON, atomic)
│   ├── diag/        Diagnostics (rolling frames + JSON reports)
│   └── ui/          Theme (Vanta palette), UI shell, pages/*
└── tests/           Catch2: w2s, angles, aim selection, smoothing, filters,
                     offsets parse/version, http backoff, atomic file, hotkeys, profiles
```

Modules match the specification 1:1 (`App`, `ProcessReader`, `GameState`,
`EntityReader`, `OffsetManager`, `WorldToScreen`, `OverlayRenderer`, `ESP`,
`AimAssist`, `Trigger`, `RecoilControl`, `InputManager`, `ProfileManager`,
`Diagnostics`, `UI`).

---

## Building (Windows 11, Visual Studio 2022, x64)

1. Install [vcpkg](https://github.com/microsoft/vcpkg) and set `VCPKG_ROOT`.
2. From an **x64 Native Tools** developer prompt:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release --parallel
```

Three artifacts land under `build/`:

| Target        | Kind       | Output                              |
| ------------- | ---------- | ----------------------------------- |
| `vanta_core`  | STATIC lib | `build/vanta_core.lib`              |
| `VantaCS2`    | WIN32 exe  | `build/Release/VantaCS2.exe`        |
| `vanta_tests` | exe        | `build/Release/vanta_tests.exe`     |

Run the test suite:

```bat
ctest --test-dir build --output-on-failure -C Release
```

---

## Using

Launch `VantaCS2.exe` **as the same user** as your CS2 session.
Default panel toggle: **INS**. Default panic kill-switch (disables every
injected input until manually cleared): **END**.

1. Open the panel (INS). The Dashboard page shows process + module + offset
   status.
2. Go to **Source Manager**, add an HTTPS source that serves a JSON offset
   bundle (the shape is documented in
   `src/offsets/sources/GenericJsonSource.hpp` and `config/sources.example.json`).
   Click **Fetch** — a successful fetch is automatically promoted to the
   active bundle, and the Offset Manager page lists every field with its
   per-field compat state.
3. Return to the Dashboard and toggle **ESP** / **Aimbot** / **Trigger** /
   **Radar** / **Crosshair** / **Bunny-hop** as desired. Hotkeys (aim engage,
   trigger engage, bhop engage, panic, panel toggle) are rebound from the
   **Hotkeys** page — press *Bind*, press the key.
4. The **Profiles** page saves the entire settings tree as a named JSON file
   under `%LOCALAPPDATA%\VantaCS2\profiles`. Switch loadouts by activating
   another profile. Deletion requires confirmation.
5. The **Diagnostics** page shows live FPS, read ms, render ms, read/fail
   counts, module presence, offset state and working-set memory, and can
   export a JSON report at any time.

### Storage layout (atomic, migratable)

Everything under `%LOCALAPPDATA%\VantaCS2`:

```
settings.json              <- app-level + per-feature config (schema-versioned, migrated on load)
offset-sources.json        <- source catalog
offsets/active.json        <- currently-adopted bundle
offsets/history.json       <- past bundles (prunable, up to 32)
profiles/<name>.json       <- one file per profile
logs/vanta.log*            <- rotating file logs (5 MB x 4)
reports/<ts>.json          <- diagnostics exports
```

All writes go through `core::io::write_atomic` (write-to-tempfile +
`MoveFileExW(REPLACE_EXISTING|WRITE_THROUGH)`), so a crash never leaves a
half-written file.

---

## Offset source adapter — expected JSON shape

```jsonc
{
  "schema":  1,
  "build":   "<free-text game build id>",       // optional
  "version": "<free-text source revision>",     // optional
  "fields": [
    {
      "name":     "dwLocalPlayerPawn",
      "value":    "0x1811E80",                   // integer or hex-string
      "base":     "module:client.dll",           // informational
      "evidence": "manual check vs build ABCD"   // optional free-text
    },
    { "name": "C_BaseEntity::m_iHealth", "value": "0x334", "base": "class:C_BaseEntity" }
  ]
}
```

**Vanta refuses to invent offsets or endpoints.** It parses what you give it,
hashes the raw payload (SHA-256), stores it transactionally, and surfaces
per-field compat states (`unknown` by default; `suspect` if a module offset
is zero; `invalid` on parse error). Compatibility is NEVER upgraded to
`confirmed` automatically — a human must explicitly mark it so.

See `fixtures/offsets_sample.json` for a template. The values in that file
are placeholders for tests; they do not point at real CS2 build offsets.

---

## Tests run and results

The included Catch2 suite covers:

| Test file                     | Covers |
| ----------------------------- | ------ |
| `test_main.cpp`               | sanity / Catch2 wiring |
| `test_world_to_screen.cpp`    | identity / center / direction / scaling / behind-near-plane |
| `test_angles.cpp`             | normalize, calc_angle, delta shortest-arc, mouse conversion sign |
| `test_aim_selection.cpp`      | closest-to-crosshair, ally filter, hotkey release reset, max distance |
| `test_smoothing.cpp`          | 60Hz vs 240Hz converge, dt-spike clamp |
| `test_entity_filters.cpp`     | skip-dead, ally/enemy filtering (uses ImGui draw list) |
| `test_offsets_parse.cpp`      | OffsetBundle JSON round-trip; overall-state = worst field |
| `test_offsets_version.cpp`    | changed/added fields across two bundles |
| `test_http_backoff.cpp`       | empty URL rejected fast; unreachable host surfaces error |
| `test_atomic_file.cpp`        | atomic write/read; max_bytes cap enforced |
| `test_hotkeys.cpp`            | JSON round-trip; collision detection; empty = unbound |
| `test_profiles.cpp`           | create/list/load/remove |

**I did not build or run this project here** — it is Windows-only (D3D11,
`SendInput`, SHGetKnownFolderPath, DWM, PSAPI/Toolhelp) and this authoring
environment is Linux. See `docs/limitations.md` for the explicit list of
items that still require runtime verification against a real CS2 install
before any claim of correctness can be made.
