#include "shiplua/manifest/ModDiscovery.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <string>

namespace ShipLua {

namespace {

bool IsRegularFileWithoutSymlink(const std::filesystem::path& path, std::error_code& error) {
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory) {
        error.clear();
        return false;
    }
    return !error && std::filesystem::is_regular_file(status) &&
           !std::filesystem::is_symlink(status);
}

bool HasZipSignature(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::array<unsigned char, 4> signature{};
    if (!file.read(reinterpret_cast<char*>(signature.data()), signature.size())) {
        return false;
    }
    return signature == std::array<unsigned char, 4>{0x50, 0x4b, 0x03, 0x04} ||
           signature == std::array<unsigned char, 4>{0x50, 0x4b, 0x05, 0x06} ||
           signature == std::array<unsigned char, 4>{0x50, 0x4b, 0x07, 0x08};
}

} // namespace

Result<std::vector<DiscoveredMod>> DiscoverMods(const std::filesystem::path& root) {
    std::error_code error;
    const auto rootStatus = std::filesystem::symlink_status(root, error);
    if (error || !std::filesystem::is_directory(rootStatus) ||
        std::filesystem::is_symlink(rootStatus)) {
        return Result<std::vector<DiscoveredMod>>::err(
            ErrorCode::InvalidArgument, "mods root is not a readable directory: '" + root.string() + "'");
    }

    std::vector<DiscoveredMod> mods;
    std::filesystem::directory_iterator iterator(
        root, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::directory_iterator end;
    if (error) {
        return Result<std::vector<DiscoveredMod>>::err(
            ErrorCode::InvalidArgument, "cannot scan mods root '" + root.string() + "': " + error.message());
    }

    while (iterator != end) {
        const std::filesystem::directory_entry entry = *iterator;
        const auto status = entry.symlink_status(error);
        if (error) {
            return Result<std::vector<DiscoveredMod>>::err(
                ErrorCode::InvalidArgument, "cannot inspect mod candidate '" +
                                                entry.path().string() + "': " + error.message());
        }

        if (!std::filesystem::is_symlink(status) && std::filesystem::is_directory(status)) {
            const auto manifestPath = entry.path() / "manifest.toml";
            if (IsRegularFileWithoutSymlink(manifestPath, error)) {
                mods.push_back({entry.path(), ModSourceKind::Directory});
            } else if (error) {
                return Result<std::vector<DiscoveredMod>>::err(
                    ErrorCode::InvalidArgument, "cannot inspect manifest '" +
                                                    manifestPath.string() + "': " + error.message());
            }
        } else if (!std::filesystem::is_symlink(status) &&
                   std::filesystem::is_regular_file(status) &&
                   entry.path().extension() == ".shipmod" && HasZipSignature(entry.path())) {
            mods.push_back({entry.path(), ModSourceKind::Package});
        }

        iterator.increment(error);
        if (error) {
            return Result<std::vector<DiscoveredMod>>::err(
                ErrorCode::InvalidArgument, "cannot continue scanning mods root '" +
                                                root.string() + "': " + error.message());
        }
    }

    std::sort(mods.begin(), mods.end(), [](const DiscoveredMod& left, const DiscoveredMod& right) {
        return left.path.generic_string() < right.path.generic_string();
    });
    return Result<std::vector<DiscoveredMod>>::ok(std::move(mods));
}

} // namespace ShipLua
