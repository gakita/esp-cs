#pragma once
#include <cstdint>

namespace vanta::esp {

struct Color { float r{1}, g{1}, b{1}, a{1}; };

struct ESPConfig {
    bool enabled{false};

    bool box_2d{true};
    bool box_cornered{false};
    bool skeleton{false};
    bool name{true};
    bool health{true};
    bool armor{false};
    bool weapon{false};
    bool distance{false};
    bool snapline{false};
    bool team_id{true};
    bool target_indicator{false};
    bool offscreen_arrows{false};
    bool bomb_info{true};

    bool show_allies{false};
    bool show_enemies{true};
    bool skip_dead{true};

    float max_distance{300.0f * 12.0f}; // in-world units (12 units per foot)

    Color enemy_color{1.0f, 0.35f, 0.4f, 1.0f};
    Color ally_color {0.4f, 0.8f, 1.0f, 1.0f};
    Color text_color {0.95f, 0.96f, 0.98f, 1.0f};
    Color bomb_color {1.0f, 0.75f, 0.14f, 1.0f};

    float line_thickness{1.5f};
    float opacity{1.0f};
    float text_size{13.0f};
};

} // namespace vanta::esp
