#include <catch2/catch_test_macros.hpp>

#include "offsets/OffsetTypes.hpp"

using namespace vanta::offsets;

TEST_CASE("Field comparison across versions", "[offsets][version]") {
    OffsetBundle a;
    a.fields.push_back({"dwLocalPlayerPawn", 0x1000, "module:client.dll", Compat::Confirmed, ""});
    a.fields.push_back({"C_BaseEntity::m_iHealth", 0x334, "class", Compat::Confirmed, ""});

    OffsetBundle b;
    b.fields.push_back({"dwLocalPlayerPawn", 0x1200, "module:client.dll", Compat::Unknown, ""});
    b.fields.push_back({"C_BaseEntity::m_iHealth", 0x334, "class", Compat::Unknown, ""});
    b.fields.push_back({"newField", 0x50, "class", Compat::Unknown, ""});

    std::vector<std::string> changed;
    for (auto& fa : a.fields) {
        if (auto fb = b.find(fa.name)) {
            if (fb->value != fa.value) changed.push_back(fa.name);
        }
    }
    REQUIRE(changed.size() == 1);
    REQUIRE(changed[0] == "dwLocalPlayerPawn");

    // Fields present in b but not a:
    std::vector<std::string> added;
    for (auto& fb : b.fields) {
        if (!a.find(fb.name)) added.push_back(fb.name);
    }
    REQUIRE(added.size() == 1);
    REQUIRE(added[0] == "newField");
}
