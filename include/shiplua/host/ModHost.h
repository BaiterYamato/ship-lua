#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "shiplua/api/LuaApiBinding.h"
#include "shiplua/events/EventDispatcher.h"
#include "shiplua/manifest/Manifest.h"
#include "shiplua/manifest/PackageExtractor.h"
#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/LuaRuntime.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

// Owns a collection of loaded mods, each running in its own isolated
// LuaRuntime (separate lua_State per mod). Keyed by manifest id.
//
// Dependency/order resolution (MOD-004..007) is out of scope: dependencies
// declared in a manifest are stored but not resolved.
class ModHost {
  public:
    explicit ModHost(Logger logger = {});
    ModHost(LuaApiHostContext hostContext, Logger logger = {});
    ~ModHost();

    ModHost(const ModHost&) = delete;
    ModHost& operator=(const ModHost&) = delete;
    ModHost(ModHost&& other) noexcept;
    ModHost& operator=(ModHost&& other) noexcept;

    // Loads a mod from `dir`: parses <dir>/manifest.toml, reads the
    // entrypoint file and runs it in a fresh isolated runtime. On any
    // failure nothing stays loaded (rollback). `memoryLimitBytes == 0`
    // means no per-mod Lua memory cap.
    Result<void> LoadModFromDirectory(const std::string& dir, std::size_t memoryLimitBytes = 0);

    // Lower-level variant (no filesystem): registers `manifest` and runs
    // `luaSource` as its entrypoint in a fresh isolated runtime.
    Result<void> LoadModFromManifestAndSource(const Manifest& manifest,
                                              const std::string& luaSource,
                                              std::size_t memoryLimitBytes = 0);

    // Safely extracts a .shipmod into an owned cache directory, loads it, and
    // removes the extracted files on unload or failure.
    Result<void> LoadModFromPackage(const std::string& package,
                                    const std::string& extractionRoot,
                                    const PackageLimits& limits = {},
                                    std::size_t memoryLimitBytes = 0);

    bool IsLoaded(const std::string& id) const;
    std::size_t Count() const;
    std::size_t SubscriptionCount() const;

    Result<DispatchOutcome> DispatchEvent(const std::string& name, EventPayload& payload);

    // Destroys the mod's LuaRuntime (releasing all of its resources) and
    // removes it. ErrorCode::InvalidHandle if no such mod is loaded.
    Result<void> UnloadMod(const std::string& id);

    void UnloadAll();

    // Internal accessor for tests/integration layers; nullptr if absent.
    LuaRuntime* GetRuntime(const std::string& id);

  private:
    struct LoadedMod {
        Manifest manifest;
        std::unique_ptr<LuaRuntime> runtime;
        std::unique_ptr<LuaApiBinding> binding;
        std::filesystem::path ownedDirectory;
    };

    Logger mLogger;
    LuaApiHostContext mHostContext;
    std::unique_ptr<EventDispatcher> mEvents;
    std::size_t mNextModLoadOrder = 0;
    std::map<std::string, LoadedMod> mMods;
};

} // namespace ShipLua
