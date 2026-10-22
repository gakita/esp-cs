#pragma once
#include "entity_cache.hpp"
namespace features {
struct AimbotConfig {
    bool  enabled             = false;   // default OFF — liga pelo menu
    bool  require_hold        = true;
    bool  target_head         = true;
    bool  require_valid_bones = false;
    int   hotkey_vk           = 0x00;    // default: desabilitado (0 = nenhuma tecla)
    float fov_degrees         = 7.f;
    float smoothing           = 0.72f;
    float max_px_frame        = 42.f;
    float jitter_px           = 0.6f;
    float sensitivity         = 1.5f;
};
class Aimbot { public: void tick(const Snapshot& snap, const AimbotConfig& cfg); };
}