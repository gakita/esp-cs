#include <catch2/catch_test_macros.hpp>

#include "profile/ProfileManager.hpp"
#include "core/Config.hpp"

#include <algorithm>

using namespace vanta::profile;

TEST_CASE("Create / list / delete profile round-trip", "[profiles]") {
    ProfileManager pm;
    pm.refresh();

    nlohmann::json data = {
        {"esp", {{"enabled", true}}},
        {"ui",  {{"page", "dashboard"}}},
    };
    const std::string nm = "unit-test-profile";
    (void)pm.remove(nm);

    REQUIRE(pm.create(nm, data));

    auto list = pm.list();
    REQUIRE(std::find(list.begin(), list.end(), nm) != list.end());

    auto loaded = pm.load(nm);
    REQUIRE(loaded.has_value());
    REQUIRE((*loaded)["esp"]["enabled"].get<bool>() == true);

    REQUIRE(pm.remove(nm));
    list = pm.list();
    REQUIRE(std::find(list.begin(), list.end(), nm) == list.end());
}
