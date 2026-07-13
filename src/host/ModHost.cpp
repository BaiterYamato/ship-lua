#include "shiplua/host/ModHost.h"

#include <filesystem>
#include <fstream>
#include <atomic>
#include <chrono>
#include <sstream>
#include <utility>

#include "shiplua/manifest/ManifestParser.h"

namespace ShipLua {

namespace {

Result<std::string> ReadTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<std::string>::err(ErrorCode::InvalidArgument,
                                        "cannot open file '" + path + "'");
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad()) {
        return Result<std::string>::err(ErrorCode::InvalidArgument,
                                        "failed to read file '" + path + "'");
    }
    return Result<std::string>::ok(contents.str());
}

Result<std::filesystem::path> ResolveEntrypoint(const std::filesystem::path& modDir,
                                                const std::string& entrypoint) {
    const std::filesystem::path relative(entrypoint);
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() ||
        relative.has_root_directory()) {
        return Result<std::filesystem::path>::err(
            ErrorCode::PermissionDenied, "entrypoint must be a relative path inside the mod directory");
    }

    const std::filesystem::path normalized = relative.lexically_normal();
    for (const auto& component : normalized) {
        if (component == "..") {
            return Result<std::filesystem::path>::err(
                ErrorCode::PermissionDenied, "entrypoint cannot contain parent traversal ('..')");
        }
    }

    std::error_code error;
    std::filesystem::path current = std::filesystem::weakly_canonical(modDir, error);
    if (error) {
        return Result<std::filesystem::path>::err(
            ErrorCode::InvalidArgument, "cannot resolve mod directory: " + error.message());
    }
    for (const auto& component : normalized) {
        current /= component;
        const auto status = std::filesystem::symlink_status(current, error);
        if (error) {
            return Result<std::filesystem::path>::err(
                ErrorCode::InvalidArgument, "cannot inspect entrypoint path: " + error.message());
        }
        if (std::filesystem::is_symlink(status)) {
            return Result<std::filesystem::path>::err(
                ErrorCode::PermissionDenied, "entrypoint path cannot contain symbolic links");
        }
    }
    return Result<std::filesystem::path>::ok(std::move(current));
}

std::filesystem::path MakePackageDirectory(const std::filesystem::path& root) {
    static std::atomic<std::uint64_t> counter{0};
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return root / (".shiplua-package-" + std::to_string(ticks) + "-" +
                   std::to_string(counter.fetch_add(1, std::memory_order_relaxed)));
}

} // namespace

ModHost::ModHost(Logger logger) : mLogger(std::move(logger)) {}

ModHost::~ModHost() {
    UnloadAll();
}

ModHost::ModHost(ModHost&& other) noexcept
    : mLogger(std::move(other.mLogger)), mMods(std::move(other.mMods)) {}

ModHost& ModHost::operator=(ModHost&& other) noexcept {
    if (this != &other) {
        UnloadAll();
        mLogger = std::move(other.mLogger);
        mMods = std::move(other.mMods);
    }
    return *this;
}

Result<void> ModHost::LoadModFromDirectory(const std::string& dir, std::size_t memoryLimitBytes) {
    const std::filesystem::path modDir(dir);
    const std::string manifestPath = (modDir / "manifest.toml").string();

    Result<Manifest> parsed = ParseManifestFile(manifestPath);
    if (!parsed.isOk()) {
        return Result<void>::err(parsed.code, parsed.message);
    }
    const Manifest& manifest = *parsed.value;

    auto entrypointPath = ResolveEntrypoint(modDir, manifest.entrypoint);
    if (!entrypointPath.isOk()) {
        return Result<void>::err(entrypointPath.code,
                                 "mod '" + manifest.id + "': " + entrypointPath.message);
    }
    Result<std::string> source = ReadTextFile(entrypointPath.value->string());
    if (!source.isOk()) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "mod '" + manifest.id + "': cannot read entrypoint '" +
                                     manifest.entrypoint + "': " + source.message);
    }

    return LoadModFromManifestAndSource(manifest, *source.value, memoryLimitBytes);
}

Result<void> ModHost::LoadModFromManifestAndSource(const Manifest& manifest,
                                                   const std::string& luaSource,
                                                   std::size_t memoryLimitBytes) {
    if (manifest.id.empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument, "manifest has an empty id");
    }
    if (mMods.find(manifest.id) != mMods.end()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "mod '" + manifest.id + "' is already loaded");
    }

    auto runtime = std::make_unique<LuaRuntime>(manifest.id, mLogger, memoryLimitBytes);
    if (runtime->State() == nullptr) {
        return Result<void>::err(ErrorCode::HostFailure,
                                 "mod '" + manifest.id + "': failed to create Lua runtime");
    }

    const std::string chunkName = manifest.id + "/" + manifest.entrypoint;
    Result<void> run = runtime->DoString(luaSource, chunkName);
    if (!run.isOk()) {
        // Roll back: the runtime is destroyed here; the mod is never stored.
        return run;
    }

    mLogger.info(manifest.id, "loaded mod '" + manifest.name + "' v" + manifest.version);
    mMods.emplace(manifest.id, LoadedMod{manifest, std::move(runtime), {}});
    return Result<void>::ok();
}

Result<void> ModHost::LoadModFromPackage(const std::string& package,
                                         const std::string& extractionRoot,
                                         const PackageLimits& limits,
                                         std::size_t memoryLimitBytes) {
    const std::filesystem::path destination = MakePackageDirectory(extractionRoot);
    Result<void> extracted = ExtractShipmod(package, destination, limits);
    if (!extracted.isOk()) {
        return extracted;
    }

    auto cleanup = [&destination]() {
        std::error_code ignored;
        std::filesystem::remove_all(destination, ignored);
    };
    Result<Manifest> parsed = ParseManifestFile((destination / "manifest.toml").string());
    if (!parsed.isOk()) {
        cleanup();
        return Result<void>::err(parsed.code, parsed.message);
    }

    const std::string id = parsed.value->id;
    Result<void> loaded = LoadModFromDirectory(destination.string(), memoryLimitBytes);
    if (!loaded.isOk()) {
        cleanup();
        return loaded;
    }
    auto it = mMods.find(id);
    if (it == mMods.end()) {
        cleanup();
        return Result<void>::err(ErrorCode::HostFailure,
                                 "loaded package was not registered by its manifest id");
    }
    it->second.ownedDirectory = destination;
    return Result<void>::ok();
}

bool ModHost::IsLoaded(const std::string& id) const {
    return mMods.find(id) != mMods.end();
}

std::size_t ModHost::Count() const {
    return mMods.size();
}

Result<void> ModHost::UnloadMod(const std::string& id) {
    auto it = mMods.find(id);
    if (it == mMods.end()) {
        return Result<void>::err(ErrorCode::InvalidHandle, "mod '" + id + "' is not loaded");
    }
    const std::filesystem::path ownedDirectory = std::move(it->second.ownedDirectory);
    mMods.erase(it); // destroys the LuaRuntime (closes its lua_State)
    if (!ownedDirectory.empty()) {
        std::error_code error;
        std::filesystem::remove_all(ownedDirectory, error);
        if (error) {
            return Result<void>::err(ErrorCode::HostFailure,
                                     "mod unloaded but extracted package cleanup failed: " +
                                         error.message());
        }
    }
    mLogger.info(id, "unloaded mod");
    return Result<void>::ok();
}

void ModHost::UnloadAll() {
    while (!mMods.empty()) {
        UnloadMod(mMods.begin()->first);
    }
}

LuaRuntime* ModHost::GetRuntime(const std::string& id) {
    auto it = mMods.find(id);
    return (it != mMods.end()) ? it->second.runtime.get() : nullptr;
}

} // namespace ShipLua
