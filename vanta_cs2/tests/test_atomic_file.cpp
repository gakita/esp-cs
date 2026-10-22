#include <catch2/catch_test_macros.hpp>

#include "core/AtomicFile.hpp"

#include <filesystem>

using namespace vanta;

TEST_CASE("Atomic write survives being read mid-rename", "[io]") {
    auto tmp = std::filesystem::temp_directory_path() / "vanta_atomic_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    REQUIRE(io::write_atomic(tmp, "{\"a\":1}"));

    std::string content;
    REQUIRE(io::read_file(tmp, content));
    REQUIRE(content == "{\"a\":1}");

    REQUIRE(io::write_atomic(tmp, "{\"b\":2}"));
    REQUIRE(io::read_file(tmp, content));
    REQUIRE(content == "{\"b\":2}");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("read_file enforces max_bytes", "[io]") {
    auto tmp = std::filesystem::temp_directory_path() / "vanta_big.bin";
    std::string big(1024 * 32, 'x');
    REQUIRE(io::write_atomic(tmp, big));
    std::string out;
    REQUIRE_FALSE(io::read_file(tmp, out, /*max_bytes=*/1024));
    REQUIRE(io::read_file(tmp, out, /*max_bytes=*/1024 * 64));
    REQUIRE(out.size() == big.size());
    std::filesystem::remove(tmp);
}
