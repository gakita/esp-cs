#include <catch2/catch_test_macros.hpp>

#include "input/Hotkey.hpp"

using namespace vanta::input;

TEST_CASE("Hotkey round-trips JSON", "[hotkey]") {
    Hotkey h;
    h.vk = 0x41; // 'A'
    h.ctrl = true;
    h.alt  = false;
    auto j = h.to_json();
    auto back = Hotkey::from_json(j);
    REQUIRE(back == h);
    REQUIRE(back.display().find("Ctrl+") == 0);
}

TEST_CASE("Collisions detected across map", "[hotkey]") {
    HotkeyMap m;
    Hotkey h; h.vk = 0x42; h.ctrl = true;
    m.entries["aim"] = h;
    m.entries["panic"] = h;
    auto c = m.collisions(h, "aim");
    REQUIRE(c.size() == 1);
    REQUIRE(c[0] == "panic");
    c = m.collisions(h, "panic");
    REQUIRE(c.size() == 1);
    REQUIRE(c[0] == "aim");
}

TEST_CASE("Empty hotkey considered unbound", "[hotkey]") {
    Hotkey h;
    REQUIRE(h.empty());
    REQUIRE(h.display() == "<unbound>");
}
