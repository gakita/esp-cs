#include "aimbot.hpp"
#include "sdk/math.hpp"
#include "input/mouse.hpp"
#include <Windows.h>
#include <algorithm>
#include <random>

namespace features {
static thread_local std::mt19937 rng{std::random_device{}()};
static float noise(float amp) { std::uniform_real_distribution<float> d(-amp, amp); return d(rng); }
static float angle_delta(float a, float b) { return sdk::wrap_angle(b - a); }

void Aimbot::tick(const Snapshot& snap, const AimbotConfig& cfg) {
    if (!cfg.enabled) return;
    if (cfg.require_hold && cfg.hotkey_vk) {
        if ((GetAsyncKeyState(cfg.hotkey_vk) & 0x8000) == 0) return;
    }
    const sdk::Vec3 eye  = snap.local_eye;
    const sdk::QAngle va = snap.local_view_angles;

    float best_cost = cfg.fov_degrees;
    sdk::QAngle best_target{};
    int best_idx = -1;

    for (std::size_t i = 0; i < snap.players.size(); ++i) {
        const auto& p = snap.players[i];
        if (!p.valid || !p.alive || p.dormant) continue;
        if (p.team == snap.local_team) continue;
        if (cfg.require_valid_bones && !p.bones_valid) continue;

        const sdk::Vec3 aim_pt = cfg.target_head ? p.bone_head : p.bone_chest;
        const sdk::QAngle want = sdk::calc_angles(eye, aim_pt);
        const float dp = angle_delta(va.pitch, want.pitch);
        const float dy = angle_delta(va.yaw,   want.yaw);
        const float cost = std::sqrt(dp*dp + dy*dy);
        if (cost < best_cost) { best_cost = cost; best_idx = int(i); best_target = want; }
    }
    if (best_idx < 0) return;

    float d_pitch = angle_delta(va.pitch, best_target.pitch);
    float d_yaw   = angle_delta(va.yaw,   best_target.yaw);
    const float strength = 1.f - std::clamp(cfg.smoothing, 0.f, 0.98f);
    d_pitch *= strength; d_yaw *= strength;
    d_pitch += noise(cfg.jitter_px * 0.02f);
    d_yaw   += noise(cfg.jitter_px * 0.02f);

    constexpr float kMouseConst = 0.022f;
    const float px_per_deg = 1.f / (cfg.sensitivity * kMouseConst);
    float dx =  d_yaw   * px_per_deg;
    float dy = -d_pitch * px_per_deg;
    dx = std::clamp(dx, -cfg.max_px_frame, cfg.max_px_frame);
    dy = std::clamp(dy, -cfg.max_px_frame, cfg.max_px_frame);
    if (std::abs(dx) < 0.5f && std::abs(dy) < 0.5f) return;
    input::mouse_move_rel(int(dx), int(dy));
}
}