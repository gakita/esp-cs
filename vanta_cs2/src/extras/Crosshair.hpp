#pragma once

struct ImDrawList;

namespace vanta::extras {

struct CrosshairConfig {
    bool enabled{false};
    float gap{3.0f};
    float length{7.0f};
    float thickness{1.0f};
    bool  outline{true};
    float r{0.4f}, g{0.95f}, b{0.6f}, a{1.0f};
    bool  dot{true};
    bool  show_recoil_marker{false};
};

class Crosshair {
public:
    void render(ImDrawList* dl, float sw, float sh, const CrosshairConfig& cfg,
                float recoil_dx = 0.0f, float recoil_dy = 0.0f);
};

} // namespace vanta::extras
