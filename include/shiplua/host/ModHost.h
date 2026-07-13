#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>

#include "shiplua/manifest/Manifest.h"
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

    ModHost(const ModHost&) = delete;
    ModHost& operator=(const ModHost&) = delete;
    ModHost(ModHost&&) noexcept = default;
    ModHost& operator=(ModHost&&) noexcept = default;

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

    bool IsLoaded(const std::string& id) const;
    std::size_t Count() const;

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
    };

    Logger mLogger;
    std::map<std::string, LoadedMod> mMods;
};

} // namespace ShipLua
