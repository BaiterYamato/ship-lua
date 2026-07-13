#pragma once

#include <map>
#include <string>
#include <vector>

#include "shiplua/manifest/Manifest.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

struct ModResolution {
    // Loadable manifest ids in deterministic load order.
    std::vector<std::string> orderedIds;
    // Rejected manifest id -> human-readable reason.
    std::map<std::string, std::string> rejected;
};

// Resolves required dependencies and load-order hints. Missing/incompatible
// dependencies reject only the affected mod and its dependents. Hints that
// refer to an absent mod are ignored. Duplicate ids and graph cycles are
// rejected without preventing independent mods from loading.
Result<ModResolution> ResolveMods(const std::vector<Manifest>& manifests);

} // namespace ShipLua
