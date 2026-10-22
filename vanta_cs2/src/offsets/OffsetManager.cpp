#include "offsets/OffsetManager.hpp"

#include "core/AtomicFile.hpp"
#include "core/FilePaths.hpp"
#include "core/Log.hpp"
#include "offsets/sources/GenericJsonSource.hpp"

#include <algorithm>
#include <chrono>

namespace vanta::offsets {

using nlohmann::json;

// --------- OffsetField / OffsetBundle serialization -----------------------

const char* compat_label(Compat c) {
    switch (c) {
        case Compat::Unknown:   return "unknown";
        case Compat::Confirmed: return "confirmed";
        case Compat::Suspect:   return "suspect";
        case Compat::Invalid:   return "invalid";
        case Compat::Stale:     return "stale";
    }
    return "unknown";
}

const OffsetField* OffsetBundle::find(std::string_view name) const {
    for (auto& f : fields) if (f.name == name) return &f;
    return nullptr;
}

Compat OffsetBundle::overall() const {
    if (fields.empty()) return Compat::Invalid;
    int worst = 0;
    auto rank = [](Compat c) {
        switch (c) {
            case Compat::Confirmed: return 0;
            case Compat::Unknown:   return 1;
            case Compat::Suspect:   return 2;
            case Compat::Stale:     return 3;
            case Compat::Invalid:   return 4;
        }
        return 1;
    };
    Compat out = Compat::Confirmed;
    for (auto& f : fields) {
        int r = rank(f.compat);
        if (r > worst) { worst = r; out = f.compat; }
    }
    return out;
}

json OffsetBundle::to_json() const {
    json j;
    j["source_name"]     = source_name;
    j["source_version"]  = source_version;
    j["game_build"]      = game_build;
    j["sha256"]          = sha256_hex;
    j["fetched_at_unix"] = fetched_at_unix;
    j["fields"]          = json::array();
    for (auto& f : fields) {
        j["fields"].push_back({
            {"name",     f.name},
            {"value",    f.value},
            {"base",     f.base},
            {"compat",   compat_label(f.compat)},
            {"evidence", f.evidence},
        });
    }
    return j;
}

static Compat compat_from_label(std::string_view s) {
    if (s == "confirmed") return Compat::Confirmed;
    if (s == "suspect")   return Compat::Suspect;
    if (s == "invalid")   return Compat::Invalid;
    if (s == "stale")     return Compat::Stale;
    return Compat::Unknown;
}

OffsetBundle OffsetBundle::from_json(const json& j, std::string* err) {
    OffsetBundle b;
    try {
        b.source_name    = j.value("source_name", "");
        b.source_version = j.value("source_version", "");
        b.game_build     = j.value("game_build", "");
        b.sha256_hex     = j.value("sha256", "");
        b.fetched_at_unix = j.value("fetched_at_unix", 0ULL);
        if (j.contains("fields") && j["fields"].is_array()) {
            for (auto& fj : j["fields"]) {
                OffsetField of;
                of.name     = fj.value("name", "");
                of.value    = fj.value("value", 0ULL);
                of.base     = fj.value("base", "");
                of.compat   = compat_from_label(fj.value("compat", "unknown"));
                of.evidence = fj.value("evidence", "");
                b.fields.push_back(std::move(of));
            }
        }
    } catch (const std::exception& e) {
        if (err) *err = e.what();
    }
    return b;
}

// --------- SourceConfig serialization -------------------------------------

json SourceConfig::to_json() const {
    return {
        {"name", name}, {"type", type}, {"url", url}, {"notes", notes},
        {"timeout_ms", timeout_ms}, {"max_retries", max_retries}, {"enabled", enabled}
    };
}

SourceConfig SourceConfig::from_json(const json& j) {
    SourceConfig c;
    c.name        = j.value("name", "");
    c.type        = j.value("type", "generic-json");
    c.url         = j.value("url", "");
    c.notes       = j.value("notes", "");
    c.timeout_ms  = j.value("timeout_ms", 5000);
    c.max_retries = j.value("max_retries", 3);
    c.enabled     = j.value("enabled", true);
    return c;
}

// --------- Adapter factory ------------------------------------------------

std::shared_ptr<IOffsetSource> make_source(const std::string& type_id) {
    if (type_id == "generic-json") return std::make_shared<GenericJsonSource>();
    return nullptr;
}

// --------- OffsetManager --------------------------------------------------

OffsetManager::OffsetManager() {
    load_sources();
    refresh_active_from_disk();
}
OffsetManager::~OffsetManager() = default;

void OffsetManager::load_sources() {
    std::string raw;
    if (!io::read_file(paths::sources_file(), raw)) {
        m_sources.clear();
        return;
    }
    try {
        json j = json::parse(raw);
        m_sources.clear();
        if (j.is_array()) {
            for (auto& item : j) m_sources.push_back(SourceConfig::from_json(item));
        }
    } catch (const std::exception& e) {
        VLOG_WARN("source catalog parse: {}", e.what());
        m_sources.clear();
    }
}

void OffsetManager::save_sources() {
    json j = json::array();
    for (auto& s : m_sources) j.push_back(s.to_json());
    io::write_atomic(paths::sources_file(), j.dump(2));
}

std::vector<SourceConfig> OffsetManager::list_sources() const {
    std::lock_guard lock(m_mx);
    return m_sources;
}

void OffsetManager::upsert_source(const SourceConfig& cfg) {
    std::lock_guard lock(m_mx);
    auto* ex = find_source(cfg.name);
    if (ex) *ex = cfg; else m_sources.push_back(cfg);
    save_sources();
}

void OffsetManager::remove_source(const std::string& name) {
    std::lock_guard lock(m_mx);
    m_sources.erase(
        std::remove_if(m_sources.begin(), m_sources.end(),
                       [&](const SourceConfig& c){ return c.name == name; }),
        m_sources.end());
    save_sources();
}

SourceConfig* OffsetManager::find_source(const std::string& name) {
    for (auto& s : m_sources) if (s.name == name) return &s;
    return nullptr;
}

void OffsetManager::record_status(const std::string& src, const FetchResult& r) {
    std::lock_guard lock(m_mx);
    SourceStatus* ex = nullptr;
    for (auto& s : m_status) if (s.name == src) { ex = &s; break; }
    if (!ex) { m_status.push_back({}); ex = &m_status.back(); ex->name = src; }
    ex->last_ok        = r.ok;
    ex->last_error     = r.error;
    ex->last_http      = r.http_status;
    ex->last_duration  = r.duration;
    ex->last_attempt_unix = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    if (r.ok) {
        ex->last_sha256 = r.bundle.sha256_hex;
        ex->last_build  = r.bundle.game_build;
    }
}

std::future<FetchResult> OffsetManager::fetch_async(const std::string& name) {
    SourceConfig cfg;
    {
        std::lock_guard lock(m_mx);
        auto* ex = find_source(name);
        if (!ex) {
            std::promise<FetchResult> p;
            FetchResult r; r.error = "source not found";
            p.set_value(r);
            return p.get_future();
        }
        cfg = *ex;
    }
    return std::async(std::launch::async, [this, cfg]() {
        auto adapter = make_source(cfg.type);
        FetchResult r;
        if (!adapter) {
            r.error = "unknown source type: " + cfg.type;
        } else {
            r = adapter->fetch(cfg);
        }
        record_status(cfg.name, r);
        return r;
    });
}

std::vector<std::future<FetchResult>> OffsetManager::fetch_all_async() {
    std::vector<std::future<FetchResult>> out;
    auto list = list_sources();
    for (auto& s : list) {
        if (!s.enabled) continue;
        out.push_back(fetch_async(s.name));
    }
    return out;
}

bool OffsetManager::adopt(const OffsetBundle& b) {
    if (!m_cache.commit_active(b)) return false;
    std::lock_guard lock(m_mx);
    m_active_cache = b;
    return true;
}

std::optional<OffsetBundle> OffsetManager::active_cached() const {
    std::lock_guard lock(m_mx);
    return m_active_cache;
}

void OffsetManager::refresh_active_from_disk() {
    auto a = m_cache.active();
    std::lock_guard lock(m_mx);
    m_active_cache = a;
}

std::uint64_t OffsetManager::resolve(std::string_view name, bool* found) const {
    std::lock_guard lock(m_mx);
    if (!m_active_cache) { if (found) *found = false; return 0; }
    const OffsetField* f = m_active_cache->find(name);
    if (!f || f->compat == Compat::Invalid) { if (found) *found = false; return 0; }
    if (found) *found = true;
    return f->value;
}

std::vector<SourceStatus> OffsetManager::status() const {
    std::lock_guard lock(m_mx);
    return m_status;
}

} // namespace vanta::offsets
