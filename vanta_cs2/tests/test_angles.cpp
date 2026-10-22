#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "math/Angles.hpp"

using namespace vanta::math;
using Catch::Matchers::WithinAbs;

TEST_CASE("normalize_angle wraps to [-180,180]", "[angles]") {
    REQUIRE_THAT(normalize_angle(190.0f),  WithinAbs(-170.0f, 1e-3f));
    REQUIRE_THAT(normalize_angle(-190.0f), WithinAbs( 170.0f, 1e-3f));
    REQUIRE_THAT(normalize_angle(720.0f),  WithinAbs(   0.0f, 1e-3f));
}

TEST_CASE("calc_angle: looking straight at target along +X gives yaw=0", "[angles]") {
    Angles a = calc_angle({0, 0, 0}, {10, 0, 0});
    REQUIRE_THAT(a.yaw,   WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(a.pitch, WithinAbs(0.0f, 1e-3f));
}

TEST_CASE("calc_angle: target above gives negative pitch (looking up)", "[angles]") {
    Angles a = calc_angle({0, 0, 0}, {10, 0, 10});
    REQUIRE(a.pitch < 0.0f);
}

TEST_CASE("delta: shortest arc chosen", "[angles]") {
    Angles d = delta({0, 170, 0}, {0, -170, 0});
    // 170 -> -170 shortest arc is +20, not -340.
    REQUIRE_THAT(d.yaw, WithinAbs(20.0f, 1e-3f));
}

TEST_CASE("degrees_to_mouse: sign of yaw flipped to screen X", "[angles]") {
    auto md = degrees_to_mouse(0.0f, 10.0f, 2.0f);
    // positive yaw delta (turn left in Source) must come out as negative mouse X.
    REQUIRE(md.dx < 0);
    REQUIRE(md.dy == 0);
}

TEST_CASE("degrees_to_mouse: proportional magnitude", "[angles]") {
    auto a = degrees_to_mouse(1.0f, 0.0f, 1.0f, 0.022f, 0.022f);
    auto b = degrees_to_mouse(2.0f, 0.0f, 1.0f, 0.022f, 0.022f);
    REQUIRE(std::abs(b.dy) > std::abs(a.dy));
}
