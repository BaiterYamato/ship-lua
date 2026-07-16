#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

namespace LinkSpan::Launcher {

enum class Game {
    Oot,
    Mm,
};

enum class StartupDecision {
    MissingAssets,
    LaunchOot,
    LaunchMm,
    ChooseGame,
    DualGameAssetsRequired,
};

struct AssetSet {
    bool oot = false;
    bool mm = false;
    std::vector<std::filesystem::path> ootSources;
    std::vector<std::filesystem::path> mmSources;

    bool Has(Game game) const noexcept;
    bool Empty() const noexcept;
    bool HasBoth() const noexcept;
};

AssetSet DiscoverAssets(const std::filesystem::path& directory);
// Returns IDs of unpacked directory mods and packaged .shipmod files explicitly
// declaring that both worlds are required. Ordinary games = ["oot", "mm"]
// remains compatible with either host; only requires_both_games = true is a
// hard dual-world requirement.
std::vector<std::string> DiscoverDualWorldMods(const std::filesystem::path& directory);
StartupDecision DecideStartup(const AssetSet& assets, bool dualWorldModInstalled) noexcept;
const char* GameId(Game game) noexcept;

} // namespace LinkSpan::Launcher
