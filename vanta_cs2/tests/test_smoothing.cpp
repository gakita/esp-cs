#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "aim/AimAssist.hpp"
#include "game/GameState.hpp"

using namespace vanta;
using namespace vanta::aim;
using Catch::Matchers::WithinAbs;

static game::Entity make_player(int idx, game::Team t, math::Vec3 pos) {
    game::Entity e;
    e.index = (std::uint32_t)idx;
    e.team  = t;
    e.alive = true;
    e.health = 100;
    e.origin = pos;
    e.eye_pos = pos; e.eye_pos.z += 64.0f;
    for (auto& b : e.bones) { b.pos = pos; b.pos.z += 64.0f; b.valid = true; }
    e.bones[(std::size_t)game::Bone::Head].pos = {pos.x, pos.y, pos.z + 72.0f};
    return e;
}

TEST_CASE("Smoothing: 60Hz and 240Hz converge in similar wall time", "[smoothing]") {
    game::Snapshot s;
    s.attached = true; s.game_focused = true;
    s.client_w = 1920; s.client_h = 1080;
    s.local = make_player(0, game::Team::CounterTerr, {0, 0, 0});
    s.players.push_back(make_player(1, game::Team::Terrorist, {500, 500, 0}));

    AimConfig cfg;
    cfg.enabled = true;
    cfg.fov_deg = 180.0f;
    cfg.smooth_rate_hz = 10.0f; // tau = 0.1s

    auto now = std::chrono::steady_clock::now();

    auto run = [&](float dt, int frames) {
        AimAssist a;
        math::Angles view{0, 0, 0};
        for (int i = 0; i < frames; ++i) {
            auto o = a.tick(s, cfg, view, 1.0f, now + std::chrono::milliseconds(i * (int)(dt*1000)),
                            dt, true, false);
            // Approximate updated view: move by the raw delta's effect.
            view.yaw   += -(float)o.mouse.dx * 1.0f * 0.022f;
            view.pitch +=  (float)o.mouse.dy * 1.0f * 0.022f;
        }
        return view;
    };

    // 0.5 second @ both rates: 30 frames at 60Hz, 120 at 240Hz.
    auto v60  = run(1.0f / 60.0f, 30);
    auto v240 = run(1.0f / 240.0f, 120);
    // Both should have swung most of the way to the target yaw (~45 deg).
    REQUIRE(std::abs(v60.yaw)  > 20.0f);
    REQUIRE(std::abs(v240.yaw) > 20.0f);
    // The two end states should be within a few degrees of each other.
    REQUIRE_THAT(std::abs(v60.yaw - v240.yaw), WithinAbs(0.0f, 8.0f));
}

TEST_CASE("Smoothing: dt spike is clamped by per-frame cap", "[smoothing]") {
    game::Snapshot s;
    s.attached = true; s.game_focused = true;
    s.client_w = 1920; s.client_h = 1080;
    s.local = make_player(0, game::Team::CounterTerr, {0, 0, 0});
    s.players.push_back(make_player(1, game::Team::Terrorist, {500, 500, 0}));
    AimConfig cfg;
    cfg.enabled = true;
    cfg.fov_deg = 180.0f;
    cfg.smooth_rate_hz = 60.0f;
    AimAssist a;
    auto o = a.tick(s, cfg, math::Angles{0,0,0}, 1.0f,
                    std::chrono::steady_clock::now(), /*dt=*/5.0f,
                    true, false);
    REQUIRE(std::abs(o.mouse.dx) <= 400);
    REQUIRE(std::abs(o.mouse.dy) <= 400);
}
