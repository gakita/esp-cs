#pragma once
#include "offsets/OffsetSource.hpp"

namespace vanta::offsets {

// GenericJsonSource — fetches a JSON document over HTTPS and parses it
// into an OffsetBundle. The expected shape is intentionally conservative;
// it does NOT ship with a baked-in URL.
//
// Expected JSON schema:
// {
//   "schema":  1,
//   "build":   "<optional free-text game build id>",
//   "version": "<optional source-side revision string>",
//   "fields":  [
//     {
//       "name":     "dwLocalPlayerPawn",
//       "value":    "0x1234ABCD" | 305441741,
//       "base":     "module:client.dll",
//       "evidence": "optional free-text note"
//     },
//     ...
//   ]
// }
//
// Only `name` and `value` are required per field. Missing or malformed
// fields set Compat::Invalid for that field but do NOT abort the bundle.
class GenericJsonSource final : public IOffsetSource {
public:
    std::string type_id() const override { return "generic-json"; }
    FetchResult fetch(const SourceConfig& cfg) override;
};

} // namespace vanta::offsets
