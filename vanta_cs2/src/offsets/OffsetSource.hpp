#pragma once
#include "offsets/OffsetTypes.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <string>

namespace vanta::offsets {

// Adapter configuration: how to reach a source.
// Only `type` and `name` are required.
struct SourceConfig {
    std::string name;             // unique id in the catalog
    std::string type;             // "generic-json" today; register more adapters later
    std::string url;              // HTTPS endpoint; required by generic-json
    std::string notes;            // operator-written explanation of the source
    int         timeout_ms{5000};
    int         max_retries{3};
    bool        enabled{true};

    nlohmann::json to_json() const;
    static SourceConfig from_json(const nlohmann::json& j);
};

struct FetchResult {
    bool ok{false};
    std::string error;            // populated on !ok
    OffsetBundle bundle;          // populated on ok
    int http_status{0};
    std::chrono::milliseconds duration{0};
};

// Base interface for all source adapters.
class IOffsetSource {
public:
    virtual ~IOffsetSource() = default;
    virtual std::string type_id() const = 0;

    // Blocking fetch. Called from OffsetManager's worker thread, so returning
    // a std::future lets the UI remain responsive without the adapter owning
    // the thread itself.
    virtual FetchResult fetch(const SourceConfig& cfg) = 0;
};

std::shared_ptr<IOffsetSource> make_source(const std::string& type_id);

} // namespace vanta::offsets
