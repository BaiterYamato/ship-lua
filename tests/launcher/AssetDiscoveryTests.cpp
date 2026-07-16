#include "linkspan/launcher/AssetDiscovery.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void WriteFile(const std::filesystem::path& path) {
    std::ofstream(path, std::ios::binary).put('\0');
}

void WriteRom(const std::filesystem::path& path, std::string title, int format) {
    std::array<unsigned char, 64> header{};
    header[0] = 0x80;
    header[1] = 0x37;
    header[2] = 0x12;
    header[3] = 0x40;
    title.resize(20, ' ');
    std::copy_n(title.begin(), 20, header.begin() + 0x20);
    if (format == 1) {
        for (std::size_t index = 0; index < header.size(); index += 2) {
            std::swap(header[index], header[index + 1]);
        }
    } else if (format == 2) {
        for (std::size_t index = 0; index < header.size(); index += 4) {
            std::reverse(header.begin() + static_cast<std::ptrdiff_t>(index),
                         header.begin() + static_cast<std::ptrdiff_t>(index + 4));
        }
    }
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(header.data()), header.size());
}

} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "link-span-asset-tests";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    Check(LinkSpan::Launcher::DiscoverAssets(root).Empty(), "empty directory must have no game");
    WriteFile(root / "oot.o2r");
    auto assets = LinkSpan::Launcher::DiscoverAssets(root);
    Check(assets.oot && !assets.mm, "oot.o2r must select only OoT");
    WriteFile(root / "mm.o2r");
    assets = LinkSpan::Launcher::DiscoverAssets(root);
    Check(assets.HasBoth(), "both O2R files must enable the chooser");

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    WriteRom(root / "zelda.z64", "THE LEGEND OF ZELDA", 0);
    WriteRom(root / "majora.v64", "ZELDA MAJORA'S MASK", 1);
    WriteRom(root / "majora.n64", "ZELDA MAJORA'S MASK", 2);
    WriteFile(root / "unknown.z64");
    assets = LinkSpan::Launcher::DiscoverAssets(root);
    Check(assets.HasBoth(), "N64 byte orders must be classified from the internal title");
    Check(assets.ootSources.size() == 1, "unknown ROM must not be treated as OoT");
    Check(assets.mmSources.size() == 2, "both MM byte orders must be detected");

    const auto mods = root / "mods" / "dual-mod";
    std::filesystem::create_directories(mods);
    {
        std::ofstream manifest(mods / "manifest.toml");
        manifest << "id = \"example.dual\"\n"
                    "requires_both_games = true\n";
    }
    auto dual = LinkSpan::Launcher::DiscoverDualWorldMods(root);
    Check(dual.size() == 1 && dual.front() == "dual-mod",
          "explicit dual-world manifest requirement must be discovered");
    std::filesystem::remove(mods / "manifest.toml");
    {
        std::ofstream manifest(mods / "manifest.toml");
        manifest << "games = [\"oot\", \"mm\"]\n";
    }
    Check(LinkSpan::Launcher::DiscoverDualWorldMods(root).empty(),
          "games list alone must remain compatible with either host");

    std::filesystem::remove_all(root);
    std::cout << "All Link-Span asset discovery checks passed\n";
}
