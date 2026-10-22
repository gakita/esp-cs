// Catch2 provides its own main via Catch2WithMain. This TU exists so the
// test executable compiles even when every other test file is #ifdef'ed out.
#include <catch2/catch_test_macros.hpp>

TEST_CASE("sanity", "[meta]") {
    REQUIRE(1 + 1 == 2);
}
