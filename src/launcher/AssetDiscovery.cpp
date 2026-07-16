#include "linkspan/launcher/AssetDiscovery.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <string>

namespace LinkSpan::Launcher {
namespace {

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::optional<Game> ClassifyRom(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    std::array<unsigned char, 64> header{};
    if (!stream.read(reinterpret_cast<char*>(header.data()), header.size())) {
        return std::nullopt;
    }

    const std::array<unsigned char, 4> magic{header[0], header[1], header[2], header[3]};
    if (magic == std::array<unsigned char, 4>{0x37, 0x80, 0x40, 0x12}) {
        for (std::size_t index = 0; index < header.size(); index += 2) {
            std::swap(header[index], header[index + 1]);
        }
    } else if (magic == std::array<unsigned char, 4>{0x40, 0x12, 0x37, 0x80}) {
        for (std::size_t index = 0; index < header.size(); index += 4) {
            std::reverse(header.begin() + static_cast<std::ptrdiff_t>(index),
                         header.begin() + static_cast<std::ptrdiff_t>(index + 4));
        }
    } else if (magic != std::array<unsigned char, 4>{0x80, 0x37, 0x12, 0x40}) {
        return std::nullopt;
    }

    std::string title(reinterpret_cast<const char*>(header.data() + 0x20), 20);
    std::transform(title.begin(), title.end(), title.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    if (title.find("MAJORA") != std::string::npos) {
        return Game::Mm;
    }
    if (title.find("ZELDA") != std::string::npos) {
        return Game::Oot;
    }
    return std::nullopt;
}

void Add(AssetSet& assets, Game game, const std::filesystem::path& source) {
    if (game == Game::Oot) {
        assets.oot = true;
        assets.ootSources.push_back(source);
    } else {
        assets.mm = true;
        assets.mmSources.push_back(source);
    }
}

} // namespace

bool AssetSet::Has(Game game) const noexcept {
    return game == Game::Oot ? oot : mm;
}

bool AssetSet::Empty() const noexcept {
    return !oot && !mm;
}

bool AssetSet::HasBoth() const noexcept {
    return oot && mm;
}

AssetSet DiscoverAssets(const std::filesystem::path& directory) {
    AssetSet assets;
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, error);
    if (error) {
        return assets;
    }

    for (const auto& entry : iterator) {
        if (!entry.is_regular_file(error) || error) {
            error.clear();
            continue;
        }
        const std::filesystem::path& path = entry.path();
        const std::string filename = Lower(path.filename().string());
        if (filename == "oot.o2r" || filename == "oot.otr") {
            Add(assets, Game::Oot, path);
            continue;
        }
        if (filename == "mm.o2r") {
            Add(assets, Game::Mm, path);
            continue;
        }

        const std::string extension = Lower(path.extension().string());
        if (extension == ".z64" || extension == ".v64" || extension == ".n64") {
            if (const auto game = ClassifyRom(path); game.has_value()) {
                Add(assets, *game, path);
            }
        }
    }
    return assets;
}

const char* GameId(Game game) noexcept {
    return game == Game::Oot ? "oot" : "mm";
}

std::vector<std::string> DiscoverDualWorldMods(const std::filesystem::path& directory) {
    std::vector<std::string> result;
    const auto mods = directory / "mods";
    std::error_code error;
    if (!std::filesystem::is_directory(mods, error)) {
        return result;
    }
    // Keep the launcher core independent from Lua/TOML. This intentionally
    // handles unpacked development mods; packaged .shipmod files are parsed
    // by the host and report their own compatibility error when selected.
    const std::regex requirement(R"(^\s*requires_both_games\s*=\s*true\s*(?:#.*)?$)",
                                 std::regex_constants::icase);
    std::filesystem::recursive_directory_iterator it(mods, error), end;
    for (; it != end && !error; it.increment(error)) {
        if (!it->is_regular_file(error) || error || it->path().filename() != "manifest.toml") {
            error.clear();
            continue;
        }
        std::ifstream stream(it->path());
        std::string line;
        bool requiresBoth = false;
        while (std::getline(stream, line)) {
            if (std::regex_match(line, requirement)) {
                requiresBoth = true;
                break;
            }
        }
        if (requiresBoth) {
            result.push_back(it->path().parent_path().filename().string());
        }
    }
    return result;
}

} // namespace LinkSpan::Launcher
