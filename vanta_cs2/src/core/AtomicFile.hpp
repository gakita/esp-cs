#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace vanta::io {

// Writes `contents` to `path` atomically: writes to <path>.tmp.<pid> first,
// fsync-equivalent flush, then renames over the target. Any existing
// corrupted temp file from a previous crash is overwritten.
// Returns true on success. Never partial-writes `path`.
bool write_atomic(const std::filesystem::path& path, std::string_view contents);

// Reads a file into a string. Returns std::nullopt-equivalent empty via `out`.
// Returns false if the file cannot be opened or exceeds `max_bytes`.
bool read_file(const std::filesystem::path& path, std::string& out,
               std::size_t max_bytes = 64 * 1024 * 1024);

} // namespace vanta::io
