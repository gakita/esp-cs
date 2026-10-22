#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace vanta::offsets {

// Compatibility classification. Set per-field; a bundle's overall state is
// the worst field state. Never upgraded automatically — "unknown" means
// nothing has confirmed compatibility for this build.
enum class Compat : std::uint8_t {
    Unknown = 0,     // No evidence either way.
    Confirmed,       // Operator marked as verified against the running build.
    Suspect,         // Automated check flagged a mismatch (e.g. value is 0, out of range).
    Invalid,         // Parse error or impossible value.
    Stale,           // Bundle predates currently-attached build hash.
};

const char* compat_label(Compat c);

// A single named offset. Fields are:
//   - name:     stable identifier used by the rest of the app
//   - value:    absolute offset in bytes from its parent base
//   - base:     "module:client.dll" | "class:C_CSPlayerPawn" | ...
//   - compat:   latest classification
//   - evidence: free-form note from the source adapter
struct OffsetField {
    std::string name;
    std::uint64_t value{0};
    std::string base;
    Compat compat{Compat::Unknown};
    std::string evidence;
};

// A parsed offset bundle fetched from a source.
struct OffsetBundle {
    std::string source_name;          // e.g. "generic-json:example-source"
    std::string source_version;       // whatever the source reports (free text)
    std::string game_build;           // whatever the source reports (free text)
    std::string sha256_hex;           // digest of the raw payload
    std::uint64_t fetched_at_unix{0};
    std::vector<OffsetField> fields;

    // Convenience: find a field by name. nullptr if absent.
    const OffsetField* find(std::string_view name) const;

    // Overall state = worst field state. Empty bundle = Invalid.
    Compat overall() const;

    nlohmann::json to_json() const;
    static OffsetBundle from_json(const nlohmann::json& j, std::string* err = nullptr);
};

} // namespace vanta::offsets
