#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace vanta::hash {

// 32-byte SHA-256 digest.
using Sha256Digest = std::array<std::uint8_t, 32>;

Sha256Digest sha256(const void* data, std::size_t len);

inline Sha256Digest sha256(std::string_view s) {
    return sha256(s.data(), s.size());
}

// Lowercase hex, 64 chars.
std::string to_hex(const Sha256Digest& d);

} // namespace vanta::hash
