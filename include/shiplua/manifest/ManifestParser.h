#pragma once

#include <string>

#include "shiplua/manifest/Manifest.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

// Parses manifest.toml content into a Manifest.
//
// Never throws. TOML syntax errors and missing required fields
// (id, name, version, api, entrypoint) are reported as
// ErrorCode::InvalidArgument with a human-readable message that includes
// `sourceName` (and line/column for syntax errors). Unknown/extra keys are
// ignored for forward compatibility.
Result<Manifest> ParseManifestString(const std::string& toml,
                                     const std::string& sourceName = "manifest.toml");

// Reads `path` from disk and parses it. Missing/unreadable files are
// reported as ErrorCode::InvalidArgument.
Result<Manifest> ParseManifestFile(const std::string& path);

} // namespace ShipLua
