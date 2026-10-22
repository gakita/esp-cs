#pragma once
#include "offsets/OffsetCache.hpp"
#include "offsets/OffsetSource.hpp"

#include <atomic>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace vanta::offsets {

// Status of the most recent fetch for a given source.
struct SourceStatus {
    std::string name;
    bool last_ok{false};
    std::string last_error;
    int last_http{0};
    std::chrono::milliseconds last_duration{0};
    std::uint64_t last_attempt_unix{0};
    std::string last_sha256;     // on success
    std::string last_build;      // on success
};

class OffsetManager {
public:
    OffsetManager();
    ~OffsetManager();

    // Source catalog persisted at paths::sources_file().
    std::vector<SourceConfig> list_sources() const;
    void upsert_source(const SourceConfig& cfg);
    void remove_source(const std::string& name);

    // Kicks off an async fetch of the given source. The future carries the
    // raw FetchResult; the manager has already updated status(). After a
    // successful fetch the operator chooses whether to adopt via adopt().
    std::future<FetchResult> fetch_async(const std::string& source_name);

    // Fetches every enabled source in parallel. Returns futures in the same
    // order as list_sources()'s enabled subset.
    std::vector<std::future<FetchResult>> fetch_all_async();

    // Promote a fetched bundle to active (commits via OffsetCache).
    bool adopt(const OffsetBundle& b);

    // Convenience: direct read access to the active bundle, cached in memory.
    std::optional<OffsetBundle> active_cached() const;
    void refresh_active_from_disk();

    // Latest status for every source, newest first.
    std::vector<SourceStatus> status() const;

    // Field lookup against the active bundle. Returns 0 and sets `*found`
    // false if the field is missing or the active bundle is empty.
    std::uint64_t resolve(std::string_view field_name, bool* found = nullptr) const;

    OffsetCache& cache() { return m_cache; }

private:
    void load_sources();
    void save_sources();

    SourceConfig* find_source(const std::string& name);
    void record_status(const std::string& src, const FetchResult& r);

    mutable std::mutex m_mx;
    std::vector<SourceConfig> m_sources;
    std::vector<SourceStatus> m_status;
    std::optional<OffsetBundle> m_active_cache;

    OffsetCache m_cache;
};

} // namespace vanta::offsets
