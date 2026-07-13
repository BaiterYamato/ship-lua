#include "shiplua/manifest/PackageExtractor.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <limits>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#include <miniz.h>

namespace ShipLua {

namespace {

class ZipReader {
  public:
    explicit ZipReader(const std::filesystem::path& path) {
        mOpen = mz_zip_reader_init_file(&mArchive, path.string().c_str(), 0) != 0;
    }

    ~ZipReader() {
        if (mOpen) {
            mz_zip_reader_end(&mArchive);
        }
    }

    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    bool IsOpen() const { return mOpen; }
    mz_zip_archive& Archive() { return mArchive; }

  private:
    mz_zip_archive mArchive{};
    bool mOpen = false;
};

class StagingGuard {
  public:
    explicit StagingGuard(std::filesystem::path path) : mPath(std::move(path)) {}
    ~StagingGuard() {
        if (mActive) {
            std::error_code ignored;
            std::filesystem::remove_all(mPath, ignored);
        }
    }
    void Commit() { mActive = false; }

  private:
    std::filesystem::path mPath;
    bool mActive = true;
};

struct ValidatedEntry {
    mz_uint index;
    std::filesystem::path relativePath;
    bool directory;
};

Result<std::filesystem::path> ValidateEntryPath(std::string name) {
    std::replace(name.begin(), name.end(), '\\', '/');
    if (name.empty() || name.front() == '/' || name.find(':') != std::string::npos ||
        name.find('\0') != std::string::npos) {
        return Result<std::filesystem::path>::err(ErrorCode::PermissionDenied,
                                                   "package contains an absolute or invalid path");
    }

    while (!name.empty() && name.back() == '/') {
        name.pop_back();
    }
    if (name.empty()) {
        return Result<std::filesystem::path>::err(ErrorCode::PermissionDenied,
                                                   "package contains an invalid root entry");
    }
    const std::filesystem::path path(name);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return Result<std::filesystem::path>::err(ErrorCode::PermissionDenied,
                                                   "package contains an absolute path: '" + name + "'");
    }
    for (const auto& component : path) {
        if (component.empty() || component == "." || component == "..") {
            return Result<std::filesystem::path>::err(
                ErrorCode::PermissionDenied, "package contains an unsafe path: '" + name + "'");
        }
    }
    return Result<std::filesystem::path>::ok(path.lexically_normal());
}

bool ExceedsCompressionRatio(std::uint64_t uncompressed, std::uint64_t compressed,
                             std::uint64_t limit) {
    if (uncompressed == 0) {
        return false;
    }
    if (compressed == 0) {
        return true;
    }
    return compressed <= std::numeric_limits<std::uint64_t>::max() / limit &&
           uncompressed > compressed * limit;
}

bool IsSymlink(const mz_zip_archive_file_stat& stat) {
    constexpr std::uint32_t unixFileTypeMask = 0170000;
    constexpr std::uint32_t unixSymlink = 0120000;
    const std::uint32_t unixMode = (stat.m_external_attr >> 16U) & 0xffffU;
    return (unixMode & unixFileTypeMask) == unixSymlink;
}

std::string PortableKey(const std::filesystem::path& path) {
    std::string key = path.generic_string();
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    while (!key.empty() && key.back() == '/') {
        key.pop_back();
    }
    return key;
}

std::filesystem::path MakeStagingPath(const std::filesystem::path& destination) {
    static std::atomic<std::uint64_t> counter{0};
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return destination.parent_path() /
           (".shiplua-staging-" + std::to_string(ticks) + "-" +
            std::to_string(counter.fetch_add(1, std::memory_order_relaxed)));
}

Result<void> RejectSymlinkComponents(const std::filesystem::path& path) {
    std::error_code error;
    const std::filesystem::path absolute = std::filesystem::absolute(path, error);
    if (error) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "cannot resolve package destination: " + error.message());
    }
    std::filesystem::path current = absolute.root_path();
    for (const auto& component : absolute.relative_path()) {
        current /= component;
        const auto status = std::filesystem::symlink_status(current, error);
        if (error) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "cannot inspect package destination: " + error.message());
        }
        if (std::filesystem::is_symlink(status)) {
            return Result<void>::err(ErrorCode::PermissionDenied,
                                     "package destination cannot contain symbolic links");
        }
    }
    return Result<void>::ok();
}

} // namespace

Result<void> ExtractShipmod(const std::filesystem::path& package,
                            const std::filesystem::path& destination,
                            const PackageLimits& limits) {
    if (limits.maxEntries == 0 || limits.maxFileBytes == 0 || limits.maxTotalBytes == 0 ||
        limits.maxCompressionRatio == 0) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "package extraction limits must be greater than zero");
    }
    if (destination.empty() || destination.filename().empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "package destination must name a new directory");
    }

    std::error_code error;
    const auto packageStatus = std::filesystem::symlink_status(package, error);
    if (error || !std::filesystem::is_regular_file(packageStatus) ||
        std::filesystem::is_symlink(packageStatus)) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "package is not a regular file: '" + package.string() + "'");
    }
    if (std::filesystem::exists(destination, error) || error) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "package destination already exists or cannot be inspected: '" +
                                     destination.string() + "'");
    }

    ZipReader reader(package);
    if (!reader.IsOpen()) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "package is not a readable ZIP archive: '" + package.string() + "'");
    }

    const mz_uint entryCount = mz_zip_reader_get_num_files(&reader.Archive());
    if (entryCount == 0 || entryCount > limits.maxEntries) {
        return Result<void>::err(ErrorCode::ResourceLimit,
                                 "package entry count is empty or exceeds the configured limit");
    }

    std::vector<ValidatedEntry> entries;
    entries.reserve(entryCount);
    std::map<std::string, bool> portablePaths;
    std::uint64_t totalBytes = 0;
    for (mz_uint index = 0; index < entryCount; ++index) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&reader.Archive(), index, &stat)) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "package contains an unreadable central-directory entry");
        }
        auto relativePath = ValidateEntryPath(stat.m_filename != nullptr ? stat.m_filename : "");
        if (!relativePath.isOk()) {
            return Result<void>::err(relativePath.code, relativePath.message);
        }
        if (IsSymlink(stat)) {
            return Result<void>::err(ErrorCode::PermissionDenied,
                                     "package contains a symbolic link: '" +
                                         relativePath.value->generic_string() + "'");
        }

        const bool directory = mz_zip_reader_is_file_a_directory(&reader.Archive(), index) != 0;
        if (!directory) {
            if (stat.m_uncomp_size > limits.maxFileBytes ||
                totalBytes > limits.maxTotalBytes - stat.m_uncomp_size) {
                return Result<void>::err(ErrorCode::ResourceLimit,
                                         "package uncompressed size exceeds the configured limit");
            }
            if (ExceedsCompressionRatio(stat.m_uncomp_size, stat.m_comp_size,
                                        limits.maxCompressionRatio)) {
                return Result<void>::err(ErrorCode::ResourceLimit,
                                         "package compression ratio exceeds the configured limit");
            }
            totalBytes += stat.m_uncomp_size;
        }

        const std::string key = PortableKey(*relativePath.value);
        if (key.empty() || portablePaths.contains(key)) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "package contains duplicate or ambiguous paths");
        }
        for (const auto& [existing, existingIsDirectory] : portablePaths) {
            if ((!existingIsDirectory && key.starts_with(existing + "/")) ||
                (!directory && existing.starts_with(key + "/"))) {
                return Result<void>::err(ErrorCode::InvalidArgument,
                                         "package contains a file/directory path collision");
            }
        }
        portablePaths.emplace(key, directory);
        entries.push_back({index, std::move(*relativePath.value), directory});
    }

    const std::filesystem::path destinationParent =
        destination.parent_path().empty() ? std::filesystem::path(".") : destination.parent_path();
    std::filesystem::create_directories(destinationParent, error);
    if (error) {
        return Result<void>::err(ErrorCode::HostFailure,
                                 "cannot create package destination parent: " + error.message());
    }
    Result<void> safeParent = RejectSymlinkComponents(destinationParent);
    if (!safeParent.isOk()) {
        return safeParent;
    }
    const std::filesystem::path staging = MakeStagingPath(destination);
    std::filesystem::create_directory(staging, error);
    if (error) {
        return Result<void>::err(ErrorCode::HostFailure,
                                 "cannot create package staging directory: " + error.message());
    }
    StagingGuard guard(staging);

    for (const auto& entry : entries) {
        const std::filesystem::path output = staging / entry.relativePath;
        if (entry.directory) {
            std::filesystem::create_directories(output, error);
        } else {
            std::filesystem::create_directories(output.parent_path(), error);
            if (!error && !mz_zip_reader_extract_to_file(&reader.Archive(), entry.index,
                                                          output.string().c_str(), 0)) {
                return Result<void>::err(ErrorCode::InvalidArgument,
                                         "failed to extract package entry: '" +
                                             entry.relativePath.generic_string() + "'");
            }
        }
        if (error) {
            return Result<void>::err(ErrorCode::HostFailure,
                                     "cannot create extracted package path: " + error.message());
        }
    }

    std::filesystem::rename(staging, destination, error);
    if (error) {
        return Result<void>::err(ErrorCode::HostFailure,
                                 "cannot commit extracted package: " + error.message());
    }
    guard.Commit();
    return Result<void>::ok();
}

} // namespace ShipLua
