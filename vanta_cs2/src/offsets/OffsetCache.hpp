#pragma once
#include "offsets/OffsetTypes.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace vanta::offsets {

// On-disk cache of past bundles plus the current "active" bundle.
// All writes are atomic (see core/AtomicFile).
class OffsetCache {
public:
    // active bundle (nullptr if none ever committed).
    std::optional<OffsetBundle> active() const;

    // Replace active atomically. Appends the previous active to history.
    bool commit_active(const OffsetBundle& b);

    // Full history in chronological order (oldest first).
    std::vector<OffsetBundle> history() const;

    // Rollback: commits history[i] as active. False if i out of range.
    bool rollback_to(std::size_t index);

    // Reads a bundle from disk by sha256.
    std::optional<OffsetBundle> by_hash(const std::string& sha256_hex) const;

    // Prunes history to the most recent `keep` entries. 0 = clear.
    void prune(std::size_t keep);
};

} // namespace vanta::offsets
