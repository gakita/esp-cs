#include <catch2/catch_test_macros.hpp>

#include "esp/ESP.hpp"
#include "game/GameState.hpp"

#include <imgui.h>

using namespace vanta;

static game::Entity player(int idx, game::Team t, math::Vec3 pos, int hp = 100) {
    game::Entity e;
    e.index = (std::uint32_t)idx;
    e.team  = t;
    e.alive = hp > 0;
    e.health = hp;
    e.origin = pos;
    e.eye_pos = pos; e.eye_pos.z += 64;
    for (auto& b : e.bones) { b.pos = pos; b.pos.z += 50; b.valid = true; }
    return e;
}

TEST_CASE("ESP skips dead when configured", "[esp]") {
    // Use ImGui context so AddText has a font.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = {1920, 1080};
    unsigned char* px; int w, h;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&px, &w, &h);
    ImGui::GetIO().Fonts->SetTexID((ImTextureID)1);

    game::Snapshot s;
    s.client_w = 1920; s.client_h = 1080;
    s.view_projection(0,0)=1; s.view_projection(1,1)=1;
    s.view_projection(2,2)=1; s.view_projection(3,3)=1;
    s.local = player(0, game::Team::CounterTerr, {0,0,0});
    s.players.push_back(player(1, game::Team::Terrorist, {10, 0, 0}, /*hp=*/0));
    s.players.push_back(player(2, game::Team::Terrorist, {-10, 0, 0}, /*hp=*/100));

    esp::ESPConfig cfg;
    cfg.enabled = true;
    cfg.box_2d = true;
    cfg.show_enemies = true;
    cfg.show_allies = false;
    cfg.skip_dead = true;

    esp::ESP e;
    ImDrawList dl(ImGui::GetDrawListSharedData());
    dl._ResetForNewFrame();
    dl.PushClipRectFullScreen();
    e.render(s, cfg, &dl);
    REQUIRE(e.last_stats().boxes == 1);

    ImGui::DestroyContext();
}

TEST_CASE("ESP respects ally/enemy filters", "[esp]") {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = {1920, 1080};
    unsigned char* px; int w, h;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&px, &w, &h);
    ImGui::GetIO().Fonts->SetTexID((ImTextureID)1);

    game::Snapshot s;
    s.client_w = 1920; s.client_h = 1080;
    s.view_projection(0,0)=1; s.view_projection(1,1)=1;
    s.view_projection(2,2)=1; s.view_projection(3,3)=1;
    s.local = player(0, game::Team::CounterTerr, {0,0,0});
    s.players.push_back(player(1, game::Team::CounterTerr, {10, 0, 0}));
    s.players.push_back(player(2, game::Team::Terrorist, {-10, 0, 0}));

    esp::ESPConfig cfg;
    cfg.enabled = true;
    cfg.box_2d = true;
    cfg.show_enemies = false;
    cfg.show_allies = true;

    esp::ESP e;
    ImDrawList dl(ImGui::GetDrawListSharedData());
    dl._ResetForNewFrame();
    dl.PushClipRectFullScreen();
    e.render(s, cfg, &dl);
    REQUIRE(e.last_stats().boxes == 1);
    ImGui::DestroyContext();
}
