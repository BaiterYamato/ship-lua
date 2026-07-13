#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ShipLua {

// In-memory representation of a mod's manifest.toml.
// Mirrors schema/manifest.schema.json; required fields are id, name,
// version, apiRange (the `api` key) and entrypoint. Everything else is
// optional and defaulted. Unknown keys in the TOML are ignored so newer
// manifests still load on older hosts (forward compatibility).
struct Manifest {
    // --- required -----------------------------------------------------------
    std::string id;         // e.g. "community.low_gravity"
    std::string name;       // human-readable display name
    std::string version;    // semver string, e.g. "1.2.0"
    std::string apiRange;   // ShipLua API range, e.g. ">=0.1 <1.0" (TOML key: api)
    std::string entrypoint; // e.g. "main.lua", relative to the mod directory

    // --- optional -----------------------------------------------------------
    std::string description;
    std::vector<std::string> authors;
    // Target games, e.g. {"oot", "mm"}. Empty means "both / any".
    std::vector<std::string> games;

    // [host] — per-host version ranges. Unset means "no constraint".
    std::optional<std::string> hostShipwright; // key: shipwright
    std::optional<std::string> hostTwoShip;    // key: two_ship

    // [capabilities]
    std::vector<std::string> capabilitiesRequired; // key: required
    std::vector<std::string> capabilitiesOptional; // key: optional

    // [dependencies] — pairs of (mod id, version range). Order preserved.
    // Dependency *resolution* is out of scope here (MOD-004..007).
    std::vector<std::pair<std::string, std::string>> dependencies;

    // [load]
    int loadPriority = 50;              // key: priority
    std::vector<std::string> loadAfter; // key: after
    std::vector<std::string> loadBefore; // key: before

    // [permissions]
    bool permStorage = false;   // key: storage
    bool permNetwork = false;   // key: network
    bool permClipboard = false; // key: clipboard
};

} // namespace ShipLua
