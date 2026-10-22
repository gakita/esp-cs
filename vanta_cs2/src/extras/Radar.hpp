#pragma once
#include "game/GameState.hpp"
#include "math/Angles.hpp"

struct ImDrawList;

namespace vanta::extras {

struct RadarConfig {
    bool enabled{false};
    float size_px{200.0f};
    float range_units{3500.0f};   // world units mapped to the full radar radius
    float opacity{0.9f};
    bool  rotate_with_view{true};
    float offset_x_px{20.0f};
    float offset_y_px{20.0f};
    bool  draw_bomb{true};
    bool  only_enemies{false};
};

struct RadarStats { std::uint32_t dots{0}; };

class Radar {
public:
    void render(const game::Snapshot& snap, const RadarConfig& cfg,
                const math::Angles& view, ImDrawList* dl);

    RadarStats stats() const { return m_stats; }

private:
    RadarStats m_stats{};
};

} // namespace vanta::extras
