#pragma once

#include <filesystem>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

enum class ModSourceKind {
    Directory,
    Package,
};

struct DiscoveredMod {
    std::filesystem::path path;
    ModSourceKind kind;
};

// Scans only direct children of `root`. A directory is a candidate when it
// contains a regular manifest.toml. A package must use the .shipmod extension
// and carry a recognized ZIP signature. Symbolic links are never followed.
// Results are sorted by normalized path for deterministic loading.
Result<std::vector<DiscoveredMod>> DiscoverMods(const std::filesystem::path& root);

} // namespace ShipLua
