#include "shiplua/host/ModHost.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <chrono>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

#include "shiplua/generated/ApiBindings.h"
#include "shiplua/manifest/DependencyResolver.h"
#include "shiplua/manifest/ManifestParser.h"
#include "shiplua/manifest/ModDiscovery.h"
#include "shiplua/manifest/SemVersion.h"

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

Result<void> EnsureDirectory(const std::filesystem::path& path, const std::string& label) {
    std::error_code error;
    std::filesystem::create_directories(path, error);
    if (error) {
        return Result<void>::err(ErrorCode::HostFailure,
                                 "cannot create " + label + " '" + path.string() +
                                     "': " + error.message());
    }
    const auto status = std::filesystem::symlink_status(path, error);
    if (error || !std::filesystem::is_directory(status) || std::filesystem::is_symlink(status)) {
        return Result<void>::err(
            ErrorCode::PermissionDenied,
            label + " must be a real directory without symbolic links: '" + path.string() + "'");
    }
    return Result<void>::ok();
}

Result<void> ValidateCompatibility(const Manifest& manifest, const LuaApiHostContext& context) {
    if (context.gameId != "oot" && context.gameId != "mm") {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "host game id must be 'oot' or 'mm' before loading mods");
    }
    if (!manifest.games.empty() &&
        std::find(manifest.games.begin(), manifest.games.end(), context.gameId) ==
            manifest.games.end()) {
        return Result<void>::err(ErrorCode::Unsupported,
                                 "mod does not support game '" + context.gameId + "'");
    }

    auto apiVersion = SemVersion::Parse(std::string(Generated::kApiVersion));
    auto apiRange = VersionRange::Parse(manifest.apiRange);
    if (!apiVersion.isOk() || !apiRange.isOk() ||
        !apiRange.value->Contains(*apiVersion.value)) {
        return Result<void>::err(ErrorCode::Unsupported,
                                 "mod API range '" + manifest.apiRange +
                                     "' does not include ShipLua API " +
                                     std::string(Generated::kApiVersion));
    }

    const std::optional<std::string>& hostRange =
        context.gameId == "oot" ? manifest.hostShipwright : manifest.hostTwoShip;
    if (hostRange) {
        auto hostVersion = SemVersion::Parse(context.hostVersion);
        auto range = VersionRange::Parse(*hostRange);
        if (!hostVersion.isOk() || !range.isOk() ||
            !range.value->Contains(*hostVersion.value)) {
            return Result<void>::err(ErrorCode::Unsupported,
                                     "mod host range '" + *hostRange + "' does not include " +
                                         context.hostVersion);
        }
    }
    return Result<void>::ok();
}

EventKind ToEventKind(Generated::EventKind kind) {
    switch (kind) {
        case Generated::EventKind::Observe: return EventKind::Observe;
        case Generated::EventKind::Filter: return EventKind::Filter;
        case Generated::EventKind::Transform: return EventKind::Transform;
        case Generated::EventKind::Consume: return EventKind::Consume;
    }
    return EventKind::Observe;
}

} // namespace

ModHost::ModHost(Logger logger) : ModHost(LuaApiHostContext{}, std::move(logger)) {}

ModHost::ModHost(LuaApiHostContext hostContext, Logger logger)
    : mLogger(std::move(logger)),
      mHostContext(std::move(hostContext)),
      mEvents(std::make_unique<EventDispatcher>()) {
    for (const Generated::EventBinding& event : Generated::kEvents) {
        const auto defined = mEvents->DefineEvent(std::string(event.name), ToEventKind(event.kind));
        if (!defined.isOk()) {
            mLogger.error("host", "failed to define generated event '" + std::string(event.name) +
                                      "': " + defined.message);
        }
    }
}

ModHost::~ModHost() {
    UnloadAll();
}

ModHost::ModHost(ModHost&& other) noexcept
    : mLogger(std::move(other.mLogger)),
      mHostContext(std::move(other.mHostContext)),
      mEvents(std::move(other.mEvents)),
      mNextModLoadOrder(other.mNextModLoadOrder),
      mMods(std::move(other.mMods)) {}

ModHost& ModHost::operator=(ModHost&& other) noexcept {
    if (this != &other) {
        UnloadAll();
        mLogger = std::move(other.mLogger);
        mHostContext = std::move(other.mHostContext);
        mEvents = std::move(other.mEvents);
        mNextModLoadOrder = other.mNextModLoadOrder;
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

    auto binding = std::make_unique<LuaApiBinding>(*runtime, *mEvents, mLogger, mHostContext,
                                                   mNextModLoadOrder, manifest.loadPriority);
    const auto installed = binding->Install();
    if (!installed.isOk()) {
        return Result<void>::err(installed.code,
                                 "mod '" + manifest.id + "': " + installed.message);
    }

    const std::string chunkName = manifest.id + "/" + manifest.entrypoint;
    Result<void> run = runtime->DoString(luaSource, chunkName);
    if (!run.isOk()) {
        // Roll back: the runtime is destroyed here; the mod is never stored.
        return run;
    }

    mLogger.info(manifest.id, "loaded mod '" + manifest.name + "' v" + manifest.version);
    mMods.emplace(manifest.id,
                  LoadedMod{manifest, std::move(runtime), std::move(binding), {}});
    ++mNextModLoadOrder;
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

Result<ModLoadReport> ModHost::LoadModsFromRoot(const std::filesystem::path& root,
                                                const std::filesystem::path& extractionRoot,
                                                const PackageLimits& limits,
                                                std::size_t memoryLimitBytes) {
    auto discovered = DiscoverMods(root);
    if (!discovered.isOk()) {
        return Result<ModLoadReport>::err(discovered.code, discovered.message);
    }

    struct Candidate {
        Manifest manifest;
        std::filesystem::path directory;
        std::filesystem::path ownedDirectory;
    };
    std::vector<Candidate> candidates;
    std::vector<Manifest> manifests;
    ModLoadReport report;
    bool cacheReady = false;

    auto cleanupOwnedDirectories = [&candidates]() {
        for (Candidate& candidate : candidates) {
            if (!candidate.ownedDirectory.empty()) {
                std::error_code ignored;
                std::filesystem::remove_all(candidate.ownedDirectory, ignored);
            }
        }
    };

    for (const DiscoveredMod& source : *discovered.value) {
        std::filesystem::path directory = source.path;
        std::filesystem::path ownedDirectory;
        if (source.kind == ModSourceKind::Package) {
            if (!cacheReady) {
                auto ensured = EnsureDirectory(extractionRoot, "package extraction root");
                if (!ensured.isOk()) {
                    cleanupOwnedDirectories();
                    return Result<ModLoadReport>::err(ensured.code, ensured.message);
                }
                cacheReady = true;
            }
            ownedDirectory = MakePackageDirectory(extractionRoot);
            auto extracted = ExtractShipmod(source.path, ownedDirectory, limits);
            if (!extracted.isOk()) {
                report.rejected[source.path.generic_string()] = extracted.message;
                continue;
            }
            directory = ownedDirectory;
        }

        auto parsed = ParseManifestFile((directory / "manifest.toml").string());
        if (!parsed.isOk()) {
            report.rejected[source.path.generic_string()] = parsed.message;
            if (!ownedDirectory.empty()) {
                std::error_code ignored;
                std::filesystem::remove_all(ownedDirectory, ignored);
            }
            continue;
        }
        auto compatible = ValidateCompatibility(*parsed.value, mHostContext);
        if (!compatible.isOk()) {
            report.rejected[parsed.value->id] = compatible.message;
            if (!ownedDirectory.empty()) {
                std::error_code ignored;
                std::filesystem::remove_all(ownedDirectory, ignored);
            }
            continue;
        }

        manifests.push_back(*parsed.value);
        candidates.push_back({std::move(*parsed.value), std::move(directory),
                              std::move(ownedDirectory)});
    }

    auto resolution = ResolveMods(manifests);
    if (!resolution.isOk()) {
        cleanupOwnedDirectories();
        return Result<ModLoadReport>::err(resolution.code, resolution.message);
    }
    report.rejected.insert(resolution.value->rejected.begin(),
                           resolution.value->rejected.end());

    for (const std::string& id : resolution.value->orderedIds) {
        auto candidate = std::find_if(candidates.begin(), candidates.end(),
                                      [&id](const Candidate& item) {
                                          return item.manifest.id == id;
                                      });
        if (candidate == candidates.end()) {
            report.rejected[id] = "resolved mod source is unavailable";
            continue;
        }

        std::string unavailableDependency;
        for (const auto& [dependencyId, range] : candidate->manifest.dependencies) {
            (void)range;
            if (!IsLoaded(dependencyId)) {
                unavailableDependency = dependencyId;
                break;
            }
        }
        if (!unavailableDependency.empty()) {
            report.rejected[id] = "dependency '" + unavailableDependency + "' failed to load";
            continue;
        }

        auto loaded = LoadModFromDirectory(candidate->directory.string(), memoryLimitBytes);
        if (!loaded.isOk()) {
            report.rejected[id] = loaded.message;
            continue;
        }
        if (!candidate->ownedDirectory.empty()) {
            auto stored = mMods.find(id);
            if (stored == mMods.end()) {
                report.rejected[id] = "loaded mod was not registered by its manifest id";
                continue;
            }
            stored->second.ownedDirectory = std::move(candidate->ownedDirectory);
            candidate->ownedDirectory.clear();
        }
        report.loadedIds.push_back(id);
    }

    cleanupOwnedDirectories();
    return Result<ModLoadReport>::ok(std::move(report));
}

bool ModHost::IsLoaded(const std::string& id) const {
    return mMods.find(id) != mMods.end();
}

std::size_t ModHost::Count() const {
    return mMods.size();
}

std::size_t ModHost::SubscriptionCount() const {
    return mEvents != nullptr ? mEvents->SubscriptionCount() : 0;
}

Result<DispatchOutcome> ModHost::DispatchEvent(const std::string& name,
                                               EventPayload& payload) {
    if (mEvents == nullptr) {
        return Result<DispatchOutcome>::err(ErrorCode::InvalidState,
                                            "mod host has no event dispatcher");
    }
    return mEvents->Dispatch(name, payload);
}

Result<void> ModHost::UnloadMod(const std::string& id) {
    const std::string modId = id;
    auto it = mMods.find(modId);
    if (it == mMods.end()) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "mod '" + modId + "' is not loaded");
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
    mLogger.info(modId, "unloaded mod");
    return Result<void>::ok();
}

void ModHost::UnloadAll() {
    while (!mMods.empty()) {
        const std::string id = mMods.begin()->first;
        UnloadMod(id);
    }
}

LuaRuntime* ModHost::GetRuntime(const std::string& id) {
    auto it = mMods.find(id);
    return (it != mMods.end()) ? it->second.runtime.get() : nullptr;
}

} // namespace ShipLua
