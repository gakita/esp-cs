#include <catch2/catch_test_macros.hpp>

#include "offsets/OffsetManager.hpp"

using namespace vanta::offsets;
using nlohmann::json;

TEST_CASE("Bundle round-trips through JSON", "[offsets]") {
    OffsetBundle b;
    b.source_name = "generic-json:test";
    b.source_version = "r1";
    b.game_build = "cs2-build-1234";
    b.sha256_hex = std::string(64, 'a');
    b.fetched_at_unix = 1700000000ULL;
    b.fields.push_back({"dwLocalPlayerPawn", 0x1234, "module:client.dll",
                        Compat::Confirmed, "verified vs build 1234"});
    b.fields.push_back({"C_BaseEntity::m_iHealth", 0x334, "class:C_BaseEntity",
                        Compat::Unknown, ""});

    json j = b.to_json();
    std::string err;
    auto back = OffsetBundle::from_json(j, &err);
    REQUIRE(err.empty());
    REQUIRE(back.source_name == b.source_name);
    REQUIRE(back.fields.size() == 2);
    REQUIRE(back.fields[0].name == "dwLocalPlayerPawn");
    REQUIRE(back.fields[0].value == 0x1234);
    REQUIRE(back.fields[0].compat == Compat::Confirmed);
    REQUIRE(back.find("C_BaseEntity::m_iHealth") != nullptr);
    REQUIRE(back.find("missing")                 == nullptr);
}

TEST_CASE("Bundle overall state picks the worst field", "[offsets]") {
    OffsetBundle b;
    b.fields.push_back({"a", 1, "", Compat::Confirmed, ""});
    b.fields.push_back({"b", 2, "", Compat::Suspect,   ""});
    b.fields.push_back({"c", 3, "", Compat::Confirmed, ""});
    REQUIRE(b.overall() == Compat::Suspect);
    b.fields[1].compat = Compat::Invalid;
    REQUIRE(b.overall() == Compat::Invalid);
    b.fields[1].compat = Compat::Confirmed;
    REQUIRE(b.overall() == Compat::Confirmed);
}

TEST_CASE("Empty bundle is Invalid", "[offsets]") {
    OffsetBundle b;
    REQUIRE(b.overall() == Compat::Invalid);
}
