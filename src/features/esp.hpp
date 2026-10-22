#pragma once
#include "entity_cache.hpp"
struct ImDrawList;
namespace features {
struct EspConfig {
    bool  enabled = true, box = true, health = true, name = false;
    bool  distance = true, snapline = false, team_check = true;
    float max_distance = 200.f;
};
void draw_esp(ImDrawList* draw, const Snapshot& snap, const EspConfig& cfg);
}