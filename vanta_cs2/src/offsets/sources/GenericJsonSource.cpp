#include <algorithm>
#include "offsets/sources/GenericJsonSource.hpp"

#include "core/Hash.hpp"
#include "core/Log.hpp"

#include <chrono>
#include <string>
#include <thread>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace vanta::offsets {

using nlohmann::json;
using clock_t_ = std::chrono::steady_clock;

static size_t curl_write(void* ptr, size_t sz, size_t n, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(static_cast<char*>(ptr), sz * n);
    return sz * n;
}

static int curl_debug(CURL*, curl_infotype type, char* data, size_t len, void*) {
    if (type == CURLINFO_TEXT) {
        VLOG_TRACE("curl: {}", std::string(data, len));
    }
    return 0;
}

struct HttpOut {
    bool ok{false};
    long status{0};
    std::string body;
    std::string err;
};

static HttpOut do_get(const std::string& url, int timeout_ms) {
    HttpOut r;
    CURL* c = curl_easy_init();
    if (!c) { r.err = "curl_easy_init failed"; return r; }

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT_MS, (long)std::min(timeout_ms, 3000));
    curl_easy_setopt(c, CURLOPT_USERAGENT, "VantaCS2/0.1 (+offset-source)");
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, &curl_write);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(c, CURLOPT_DEBUGFUNCTION, &curl_debug);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode rc = curl_easy_perform(c);
    if (rc != CURLE_OK) {
        r.err = curl_easy_strerror(rc);
    } else {
        curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &r.status);
        r.ok = (r.status >= 200 && r.status < 300);
        if (!r.ok) r.err = "HTTP " + std::to_string(r.status);
    }
    curl_easy_cleanup(c);
    return r;
}

static std::uint64_t parse_value(const json& v) {
    if (v.is_number_unsigned()) return v.get<std::uint64_t>();
    if (v.is_number_integer())  return static_cast<std::uint64_t>(v.get<std::int64_t>());
    if (v.is_string()) {
        const std::string s = v.get<std::string>();
        try {
            if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
                return std::stoull(s.substr(2), nullptr, 16);
            }
            return std::stoull(s, nullptr, 10);
        } catch (...) {}
    }
    throw std::runtime_error("not a numeric offset");
}

static OffsetBundle parse_bundle(const std::string& raw, const SourceConfig& cfg,
                                 std::string& err) {
    OffsetBundle b;
    b.source_name    = cfg.type + ":" + cfg.name;
    b.fetched_at_unix = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    b.sha256_hex = hash::to_hex(hash::sha256(raw));

    json root;
    try {
        root = json::parse(raw);
    } catch (const std::exception& e) {
        err = std::string("JSON parse: ") + e.what();
        return b;
    }
    if (!root.is_object()) { err = "root is not an object"; return b; }

    b.source_version = root.value("version", "");
    b.game_build     = root.value("build",   "");

    auto fields = root.find("fields");
    if (fields == root.end() || !fields->is_array()) {
        err = "missing 'fields' array";
        return b;
    }

    for (const auto& f : *fields) {
        OffsetField of;
        if (!f.is_object() || !f.contains("name")) {
            of.name    = "<unnamed>";
            of.compat  = Compat::Invalid;
            of.evidence = "malformed field entry";
            b.fields.push_back(of);
            continue;
        }
        of.name = f.value("name", "<unnamed>");
        of.base = f.value("base", "");
        of.evidence = f.value("evidence", "");
        try {
            of.value  = parse_value(f.at("value"));
            of.compat = Compat::Unknown;
            // Sanity: a plain zero on a module offset is almost always wrong.
            if (of.value == 0 && of.base.rfind("module:", 0) == 0) {
                of.compat   = Compat::Suspect;
                of.evidence = (of.evidence.empty() ? "" : of.evidence + "; ") +
                              "zero module offset";
            }
        } catch (const std::exception& e) {
            of.compat   = Compat::Invalid;
            of.evidence = e.what();
        }
        b.fields.push_back(of);
    }

    if (b.fields.empty()) {
        err = "fields array is empty";
    }
    return b;
}

FetchResult GenericJsonSource::fetch(const SourceConfig& cfg) {
    FetchResult out;
    if (cfg.url.empty()) {
        out.error = "SourceConfig.url is empty";
        return out;
    }

    const auto t0 = clock_t_::now();
    HttpOut http;

    for (int attempt = 0; attempt <= cfg.max_retries; ++attempt) {
        http = do_get(cfg.url, cfg.timeout_ms);
        if (http.ok) break;
        // Exponential backoff: 250ms, 500ms, 1000ms, capped.
        const int wait_ms = std::min(250 << attempt, 4000);
        VLOG_WARN("source {} fetch attempt {} failed ({}); retrying in {}ms",
               cfg.name, attempt + 1, http.err, wait_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    }

    out.duration    = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t_::now() - t0);
    out.http_status = (int)http.status;

    if (!http.ok) {
        out.error = http.err;
        return out;
    }

    std::string err;
    OffsetBundle b = parse_bundle(http.body, cfg, err);
    if (!err.empty()) {
        out.error  = err;
        out.bundle = std::move(b);
        return out;
    }

    out.ok     = true;
    out.bundle = std::move(b);
    return out;
}

} // namespace vanta::offsets
