#include <catch2/catch_test_macros.hpp>

#include "aim/AimAssist.hpp"
#include "game/GameState.hpp"

using namespace vanta;
using namespace vanta::game;
using namespace vanta::aim;

static Entity make_player(int idx, Team t, math::Vec3 pos, int hp = 100) {
    Entity e;
    e.index = (std::uint32_t)idx;
    e.team  = t;
    e.alive = hp > 0;
    e.health = hp;
    e.origin = pos;
    e.eye_pos = pos; e.eye_pos.z += 64.0f;
    for (auto& b : e.bones) { b.pos = pos; b.pos.z += 64.0f; b.valid = true; }
    e.bones[(std::size_t)Bone::Head].pos = {pos.x, pos.y, pos.z + 72.0f};
    return e;
}

TEST_CASE("Aimbot picks closest-to-crosshair target inside FOV", "[aim]") {
    Snapshot s;
    s.attached = true; s.game_focused = true;
    s.client_w = 1920; s.client_h = 1080;
    s.local = make_player(0, Team::CounterTerr, {0, 0, 0});
    // Enemy A directly ahead +X.
    s.players.push_back(make_player(1, Team::Terrorist, {500, 0, 0}));
    // Enemy B slightly off-axis.
    s.players.push_back(make_player(2, Team::Terrorist, {500, 60, 0}));

    AimConfig cfg;
    cfg.enabled = true;
    cfg.fov_deg = 20.0f;
    cfg.only_enemies = true;
    cfg.hold_last_target = false;

    AimAssist a;
    auto out = a.tick(s, cfg, math::Angles{0, 0, 0}, 1.0f,
                      std::chrono::steady_clock::now(), 1.0f / 240.0f,
                      /*hotkey_down=*/true, /*firing=*/false);
    REQUIRE(out.active);
    REQUIRE(out.target_index.has_value());
    REQUIRE(*out.target_index == 1); // A is on-axis, closer in angle
}

TEST_CASE("Aimbot ignores allies when only_enemies set", "[aim]") {
    Snapshot s;
    s.attached = true; s.game_focused = true;
    s.client_w = 1920; s.client_h = 1080;
    s.local = make_player(0, Team::CounterTerr, {0, 0, 0});
    s.players.push_back(make_player(1, Team::CounterTerr, {500, 0, 0}));

    AimConfig cfg;
    cfg.enabled = true;
    cfg.fov_deg = 20.0f;
    cfg.only_enemies = true;

    AimAssist a;
    auto out = a.tick(s, cfg, math::Angles{0, 0, 0}, 1.0f,
                      std::chrono::steady_clock::now(), 1.0f / 240.0f,
                      true, false);
    REQUIRE_FALSE(out.target_index.has_value());
}

TEST_CASE("Aimbot resets on hotkey release", "[aim]") {
    Snapshot s;
    s.attached = true; s.game_focused = true;
    s.client_w = 1920; s.client_h = 1080;
    s.local = make_player(0, Team::CounterTerr, {0, 0, 0});
    s.players.push_back(make_player(1, Team::Terrorist, {500, 0, 0}));

    AimConfig cfg;
    cfg.enabled = true;
    cfg.fov_deg = 10.0f;
    cfg.hold_last_target = false;

    AimAssist a;
    auto now = std::chrono::steady_clock::now();
    auto out1 = a.tick(s, cfg, math::Angles{0, 0, 0}, 1.0f, now, 1.0f / 240.0f, true, false);
    REQUIRE(out1.active);
    auto out2 = a.tick(s, cfg, math::Angles{0, 0, 0}, 1.0f, now, 1.0f / 240.0f, false, false);
    REQUIRE_FALSE(out2.active);
    REQUIRE(out2.mouse.dx == 0);
    REQUIRE(out2.mouse.dy == 0);
}

TEST_CASE("Aimbot refuses targets outside max_distance", "[aim]") {
    Snapshot s;
    s.attached = true; s.game_focused = true;
    s.client_w = 1920; s.client_h = 1080;
    s.local = make_player(0, Team::CounterTerr, {0, 0, 0});
    s.players.push_back(make_player(1, Team::Terrorist, {10000, 0, 0}));

    AimConfig cfg;
    cfg.enabled = true;
    cfg.fov_deg = 45.0f;
    cfg.max_distance = 5000.0f;

    AimAssist a;
    auto out = a.tick(s, cfg, math::Angles{0, 0, 0}, 1.0f,
                      std::chrono::steady_clock::now(), 1.0f / 240.0f, true, false);
    REQUIRE_FALSE(out.target_index.has_value());
}
