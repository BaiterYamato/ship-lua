#include "linkspan/launcher/AssetDiscovery.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <miniz.h>

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

bool RequiresBothGames(std::string_view manifest) {
    static const std::regex requirement(R"(^\s*requires_both_games\s*=\s*true\s*(?:#.*)?$)",
                                        std::regex_constants::icase);
    std::istringstream stream{std::string(manifest)};
    std::string line;
    while (std::getline(stream, line)) {
        if (std::regex_match(line, requirement)) {
            return true;
        }
    }
    return false;
}

bool PackageRequiresBothGames(const std::filesystem::path& package) {
    constexpr mz_uint64 kMaximumManifestSize = 64U * 1024U;
    mz_zip_archive archive{};
    const std::string nativePath = package.string();
    if (!mz_zip_reader_init_file(&archive, nativePath.c_str(), 0)) {
        return false;
    }

    const int index = mz_zip_reader_locate_file(
        &archive, "manifest.toml", nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
    if (index < 0) {
        mz_zip_reader_end(&archive);
        return false;
    }

    mz_zip_archive_file_stat stat{};
    if (!mz_zip_reader_file_stat(&archive, static_cast<mz_uint>(index), &stat) ||
        stat.m_uncomp_size > kMaximumManifestSize) {
        mz_zip_reader_end(&archive);
        return false;
    }

    std::string manifest(static_cast<std::size_t>(stat.m_uncomp_size), '\0');
    const bool extracted = mz_zip_reader_extract_to_mem(
        &archive, static_cast<mz_uint>(index), manifest.data(), manifest.size(), 0) != 0;
    mz_zip_reader_end(&archive);
    return extracted && RequiresBothGames(manifest);
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

StartupDecision DecideStartup(const AssetSet& assets, bool dualWorldModInstalled) noexcept {
    if (assets.Empty()) {
        return StartupDecision::MissingAssets;
    }
    if (dualWorldModInstalled && !assets.HasBoth()) {
        return StartupDecision::DualGameAssetsRequired;
    }
    if (assets.HasBoth()) {
        return StartupDecision::ChooseGame;
    }
    return assets.oot ? StartupDecision::LaunchOot : StartupDecision::LaunchMm;
}

std::vector<std::string> DiscoverDualWorldMods(const std::filesystem::path& directory) {
    std::vector<std::string> result;
    const auto mods = directory / "mods";
    std::error_code error;
    if (!std::filesystem::is_directory(mods, error)) {
        return result;
    }
    std::filesystem::recursive_directory_iterator it(mods, error), end;
    for (; it != end && !error; it.increment(error)) {
        if (it->is_directory(error) && !error && it->path().filename() == ".shiplua-cache") {
            it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file(error) || error) {
            error.clear();
            continue;
        }

        const auto& path = it->path();
        bool requiresBoth = false;
        std::string id;
        if (path.filename() == "manifest.toml") {
            std::ifstream stream(path);
            const std::string manifest((std::istreambuf_iterator<char>(stream)),
                                       std::istreambuf_iterator<char>());
            requiresBoth = RequiresBothGames(manifest);
            id = path.parent_path().filename().string();
        } else if (Lower(path.extension().string()) == ".shipmod") {
            requiresBoth = PackageRequiresBothGames(path);
            id = path.stem().string();
        }
        if (requiresBoth) {
            result.push_back(std::move(id));
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

} // namespace LinkSpan::Launcher
