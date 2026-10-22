# VANTA CS2 — limitations & verifications pending

This file is maintained as the single source of truth for what the shipped
code actually does vs. what still depends on data or verification that only
a real Windows + CS2 session can provide. The project spec is explicit that
**no feature should be exposed in the UI without a real implementation
behind it**, and that **no claim of compilation / execution / compatibility
is to be made without having actually verified it**. This document is how
that honesty is kept.

## What is implemented end-to-end

- Transparent, layered, click-through DX11 overlay that tracks CS2's client
  rectangle each frame (`src/overlay/*`).
- Dear ImGui panel with the full page set listed in the spec: Dashboard,
  ESP, Aimbot, Trigger & Recoil, Radar & Extras, Offset Manager, Offset
  Catalog, Source Manager, Version History, Profiles, Hotkeys, Interface,
  Diagnostics, About (`src/ui/pages/*`). Each option in the UI maps to a
  real field read by its subsystem; there are no decorative switches.
- `ProcessReader` (`src/process/ProcessReader.cpp`): opens cs2.exe with
  `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION`, enumerates modules
  via Toolhelp32, tracks read / fail counters, survives process restart.
- `EntityReader` (`src/game/EntityReader.cpp`): 240 Hz polling thread that
  resolves addresses through the OffsetManager, walks the Source 2 entity
  list (64 pages × 512 entries × 120-byte `CEntityIdentity`), reads team,
  health, origin, velocity, bones (via `CGameSceneNode::m_modelState`),
  bomb state, and the view-projection matrix. All reads are
  single-attempt; failures do not throw.
- `GameState` with a double-buffered snapshot the hot path reads via
  `std::shared_ptr<const Snapshot>` — no partial frames observed by ESP,
  Radar, or AimAssist.
- `WorldToScreen` for the Source 2 VMatrix layout, with explicit near-plane
  rejection and NDC → pixel mapping.
- `AimAssist`: FOV filter, bone selection, priority (closest-to-crosshair,
  closest-in-world, lowest-health), ally/enemy filter, max distance,
  target hold + switch delay, frame-rate-independent smoothing
  (`alpha = 1 - exp(-dt/tau)`), per-frame delta clamp, residue tracking so
  sub-count moves accumulate, hard reset on hotkey release (no leaked
  frames). Hold / toggle both wired.
- `Trigger`: hover-detection via angular distance to Chest bone,
  configurable hover-delay and release-time; `only_enemies` + optional
  scope requirement; releases on hotkey release.
- `ESP`: 2D box (bone-derived or fallback head/feet), cornered box,
  skeleton (head/neck/chest/pelvis chain), name, HP + armor side bars,
  weapon string, distance, snaplines, team tag, off-screen arrows, bomb
  HUD. Ally/enemy filters, skip-dead, max-distance, colors, thickness,
  opacity, text size — all read from `ESPConfig`.
- `Radar`: top-down projection with view-aligned rotation, enemies-only
  filter, bomb marker, size / range / opacity / offset configurable.
- `Crosshair`: four-arm with gap/length/thickness, optional outline,
  optional center dot, optional recoil marker.
- `OffsetManager`: per-source config, async fetch, status table, adopt,
  active + history (max 32), rollback to any history entry, resolve by
  field name, SHA-256 hashing of raw payloads, per-field compat state,
  JSON import/export.
- `GenericJsonSource`: libcurl GET with timeout, exponential backoff
  (250 ms, 500 ms, 1 s, capped 4 s), TLS verification on, parses the
  documented JSON shape, flags zero module offsets as `suspect`,
  malformed values as `invalid`.
- `ProfileManager`: one file per profile, atomic writes, create /
  duplicate / rename / delete-with-confirm, activate (merges into
  settings tree), import / export.
- `HotkeyMap`: VK + modifiers, display formatting (letters, digits,
  function keys, mouse buttons), capture by polling `GetAsyncKeyState`
  on a worker thread, collision detection across every bound action.
- `InputManager`: SendInput mouse move / mouse down-up / key down-up /
  key tap; `panic()` kill-switch gates every method and never releases
  by itself.
- `Diagnostics`: rolling 240-frame FPS / render-ms / read-ms window,
  working set via `GetProcessMemoryInfo`, module presence counters,
  JSON report export to `%LOCALAPPDATA%\VantaCS2\reports\<ts>.json`,
  recent-errors ring buffer.
- Catch2 test suite with real fixtures (`fixtures/*.json`) covering
  world-to-screen, angle math, aim selection, smoothing convergence,
  entity filters, offset parsing / version diff, HTTP backoff, atomic
  file IO, hotkey round-trip + collisions, profile create/list/delete.

## What depends on runtime data

The following pieces require fields in the **active offset bundle** that
Vanta does not hard-code and will not invent. The UI exposes the setting,
the subsystem reads it from the bundle, and if the bundle is missing that
field, Vanta logs and skips the step — it does not silently emit a bogus
address.

- `module:client.dll:dwLocalPlayerPawn`
- `module:client.dll:dwLocalPlayerController`
- `module:client.dll:dwEntityList`
- `module:client.dll:dwViewMatrix`
- `module:client.dll:dwPlantedC4`
- `C_BaseEntity::m_iTeamNum`, `m_iHealth`, `m_pGameSceneNode`
- `CGameSceneNode::m_vecAbsOrigin`, `m_modelState`
- `CModelState::m_hModel` (used as fallback for bone-array pointer)
- `C_CSPlayerPawnBase::m_ArmorValue`, `m_vecVelocity`, `m_bIsScoped`,
  `m_aimPunchAngle`
- `C_PlantedC4::m_bBombTicking`, `m_flC4Blow`, `m_flDefuseCountDown`,
  `m_bBombDefused`
- `bone.head`, `bone.neck`, `bone.chest`, `bone.pelvis`

## What is deliberately NOT implemented in this build

- **Line-of-sight tracing.** Vanta exposes `AimConfig::require_visible` and
  `ESPConfig::target_indicator`, but visibility is read as the pawn's own
  `spotted` bit (as published by the game) rather than a world ray-cast.
  The ESP page says so explicitly. A real trace would require either the
  game's own line-trace tables (another offset bundle entry) or BSP-level
  work that is out of scope here. The visibility bit should be treated as
  evidence, not proof — exactly as the spec requires.
- **Recoil control.** `RecoilControl::tick` is wired into `App::run` and
  the UI, but currently returns `{}` because this build does not yet surface
  the pawn's `m_aimPunchAngle` into the `Snapshot`. Adding it is a two-line
  change in `EntityReader::read_entity` + a `Snapshot` field; it is not
  present because the shipped build must not claim behavior it does not
  perform. The Trigger & Recoil page displays this limitation in-UI.
- **Bunny-hop on-ground detection.** `BunnyHop::tick` currently receives
  `on_ground = true` from `App::run` as a placeholder, pending the pawn's
  `m_fFlags & FL_ONGROUND` surface in `Snapshot`. The Extras page states
  this.
- **Player names and active-weapon strings.** `Entity` carries `name` and
  `active_weapon`, but `EntityReader` does not populate them in this
  build — resolving them cleanly needs
  `dwLocalPlayerController` + `C_CSPlayerController::m_iszPlayerName` and
  the pawn's `m_pClippingWeapon` + `C_WeaponServices` chain, both of which
  belong to the runtime offset bundle. The ESP will simply draw an empty
  string for those fields until the bundle supplies them.
- **View angle derivation.** `App::run` currently approximates view yaw
  from the VP matrix's first row. For precise aim math the client exposes
  `dwLocalPlayerController` → `m_angEyeAngles[0..2]` — hook that up for
  pitch and roll accuracy. Yaw is good enough for FOV filtering and
  target selection at the current precision.
- **Game sensitivity read.** `App::sensitivity` defaults to 1.0 and is
  not sourced from `sensitivity_convar` yet — the Hotkeys page can be
  extended with a numeric field, or the convar address can be added to
  the offset bundle. Until then, mouse-count conversions use 1.0 and the
  AimAssist smoothing accounts for the error via its residue + clamp.
- **Build verification.** This repository has not been built in this
  authoring environment. The Windows-only dependencies (DX11, DWM,
  PSAPI, SendInput, SHGetKnownFolderPath) make a Linux dry-run
  meaningless. On a Windows box with vcpkg configured, the commands in
  `README.md` produce `VantaCS2.exe` and `vanta_tests.exe`; running the
  test suite requires only the Catch2 target and does not need CS2.

## Spec items honored

- Dependencies **fixed** in `vcpkg.json`.
- Modules match the spec, each in its own folder.
- RAII everywhere — the only raw owners are `HANDLE` members in
  `ProcessReader` and `OverlayWindow`, both closed in their destructors.
- Reads and renders decoupled via `GameState::publish` / `current()`.
- Overlay suspends automated commands when the game loses focus
  (`AimAssist::tick` early-returns if `snap.game_focused` is false).
- OffsetManager distinguishes **structure interpreted** (parsed JSON),
  **integrity verified** (SHA-256), **version informed by the source**
  (free-text), **compatibility confirmed / unknown / suspect / invalid /
  stale** (per field, worst-of for the bundle).
- Atomic writes via `MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH)`.
- `settings.json` is schema-versioned (`Settings::kSchemaVersion`) with
  a `migrate()` pass on load; a corrupted file is restored from defaults
  with a warning log.
- Diagnostics measures **actual** FPS, render ms, read ms, entity counts,
  module state, read / fail counters, working set — no fabricated numbers.
- The PANIC hotkey disables all injected input immediately; nothing in
  the aim / trigger / recoil / bhop paths bypasses `InputManager`.
