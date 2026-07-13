#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

struct PackageLimits {
    std::size_t maxEntries = 1024;
    std::uint64_t maxFileBytes = 64ULL * 1024ULL * 1024ULL;
    std::uint64_t maxTotalBytes = 256ULL * 1024ULL * 1024ULL;
    std::uint64_t maxCompressionRatio = 200;
};

// Extracts a .shipmod ZIP into `destination`. All entries are validated before
// any bytes are written. Extraction happens in a sibling staging directory and
// is committed with a rename only after every entry succeeds. Destination must
// not already exist.
Result<void> ExtractShipmod(const std::filesystem::path& package,
                            const std::filesystem::path& destination,
                            const PackageLimits& limits = {});

} // namespace ShipLua
