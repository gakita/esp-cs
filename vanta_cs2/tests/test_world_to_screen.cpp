#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "math/WorldToScreen.hpp"

using namespace vanta::math;
using Catch::Matchers::WithinAbs;

static Mat4 identity_vp() {
    Mat4 m{};
    for (int i = 0; i < 4; ++i) m(i, i) = 1.0f;
    return m;
}

TEST_CASE("world_to_screen: point behind near plane returns false", "[w2s]") {
    Mat4 vp = identity_vp();
    Vec3 behind{0, 0, -5};
    // With identity, w = -5 + 1 = -4 (if last row (0,0,0,1)); actually w = 1
    // because row(3) = (0,0,0,1). So force behind by making row(3) negative-leaning.
    vp(3, 2) = 1.0f;  vp(3, 3) = 0.0f;      // w = z
    Vec2 out{};
    REQUIRE_FALSE(world_to_screen(vp, {0, 0, 0.001f}, 1920, 1080, out));
    REQUIRE_FALSE(world_to_screen(vp, {0, 0, -1.0f}, 1920, 1080, out));
}

TEST_CASE("world_to_screen: center projects to screen midpoint", "[w2s]") {
    Mat4 vp = identity_vp();
    Vec2 out{};
    REQUIRE(world_to_screen(vp, {0, 0, 0}, 1920, 1080, out));
    REQUIRE_THAT(out.x, WithinAbs(960.0f,  0.5f));
    REQUIRE_THAT(out.y, WithinAbs(540.0f,  0.5f));
}

TEST_CASE("world_to_screen: +x maps right, +y maps up (NDC convention)", "[w2s]") {
    Mat4 vp = identity_vp();
    Vec2 out{};
    REQUIRE(world_to_screen(vp, {0.5f, 0, 0}, 1920, 1080, out));
    REQUIRE(out.x > 960.0f);
    REQUIRE_THAT(out.y, WithinAbs(540.0f, 0.5f));

    REQUIRE(world_to_screen(vp, {0, 0.5f, 0}, 1920, 1080, out));
    REQUIRE_THAT(out.x, WithinAbs(960.0f, 0.5f));
    REQUIRE(out.y < 540.0f);
}

TEST_CASE("world_to_screen: scaling matches screen dims", "[w2s]") {
    Mat4 vp = identity_vp();
    Vec2 out{};
    REQUIRE(world_to_screen(vp, {1.0f, 0, 0}, 800, 600, out));
    REQUIRE_THAT(out.x, WithinAbs(800.0f, 0.5f));
}
