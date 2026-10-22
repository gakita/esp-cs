#include <catch2/catch_test_macros.hpp>

#include "offsets/sources/GenericJsonSource.hpp"

#include <chrono>

using namespace vanta::offsets;

TEST_CASE("GenericJsonSource rejects empty URL fast", "[offsets][http]") {
    GenericJsonSource src;
    SourceConfig cfg;
    cfg.name = "empty"; cfg.type = "generic-json"; cfg.url = "";
    cfg.timeout_ms = 500; cfg.max_retries = 0;

    const auto t0 = std::chrono::steady_clock::now();
    auto r = src.fetch(cfg);
    const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    REQUIRE_FALSE(r.ok);
    REQUIRE(r.error.find("empty") != std::string::npos);
    REQUIRE(dt < 100);
}

TEST_CASE("GenericJsonSource returns bundle on unreachable host after retries", "[offsets][http]") {
    GenericJsonSource src;
    SourceConfig cfg;
    cfg.name = "unreachable";
    cfg.type = "generic-json";
    cfg.url  = "https://127.0.0.1:1/does-not-exist.json";
    cfg.timeout_ms = 300;
    cfg.max_retries = 1;

    auto r = src.fetch(cfg);
    REQUIRE_FALSE(r.ok);
    REQUIRE_FALSE(r.error.empty());
}
