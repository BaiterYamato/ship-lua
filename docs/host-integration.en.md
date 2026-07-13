> English translation of [docs/host-integration.md](host-integration.md). The Portuguese document remains the canonical project record.

# Integrating ShipLua into a game (host)

> For **mod authors**, see [writing-mods.en.md](writing-mods.en.md). This document is
> for those who want to **add ShipLua to the source code** of a Shipwright (OoT) or
> 2Ship (MM) and compile the game with the built-in modloader.

ShipLua is **not** a plugin that drops into a ready-made executable: the modloader is a
C++ library that needs to be **compiled within the game** (it uses engine hooks
for events, input and actors). The flow is: you take the **source** of the game, add the
ShipLua and **compile it yourself**. There are ~5 steps and few files.

> Complete functional reference: the fork
> [`BaiterYamato/2ship2harkinian`](https://github.com/BaiterYamato/2ship2harkinian)
> (branch `agent/MM-005-mod-directory`) — real MM integration.

## 1. Add ShipLua as a submodule

From the game source:

```bash
git submodule add https://github.com/BaiterYamato/ship-lua.git extern/ship-lua
git submodule update --init --recursive
```

The path `extern/ship-lua` is the same in both games.

## 2. Call in CMake

In game `CMakeLists.txt`:

```cmake
option(ENABLE_SHIP_LUA "Enable ShipLua modloader" ON)

if(ENABLE_SHIP_LUA)
    add_subdirectory(extern/ship-lua)
    target_link_libraries(<game-target> PRIVATE shiplua)
endif()
```

Replace `<game-target>` with the actual host target (on 2Ship it is `2ship`; on Shipwright,
`soh`). ShipLua downloads Lua 5.4, toml++ and miniz via FetchContent.

## 3. Write the adapter (bootstrap)

Create a file in the game, e.g. `<game>/ShipLuaBootstrap.cpp` (plus `.h`). It:

1. creates a `ShipLua::ModHost` with the host identity;
2. discovers and loads mods from a `mods/` folder;
3. triggers the `game.ready` event;
4. optionally installs game-specific bindings (`ship.mm.*` / `ship.oot.*`).

Minimal skeleton:

```cpp
#include <shiplua/host/ModHost.h>
#include <shiplua/generated/ApiBindings.h>
#include <ship/Context.h>          // Host LibUltraShip API
#include <spdlog/spdlog.h>
#include <filesystem>
#include <memory>

namespace ShipLuaHost {
namespace {
std::unique_ptr<ShipLua::ModHost> gModHost;

ShipLua::Logger CreateLogger() {
    return ShipLua::Logger([](ShipLua::LogLevel lvl, const std::string& mod, const std::string& msg) {
        switch (lvl) {
            case ShipLua::LogLevel::Debug: SPDLOG_DEBUG("ShipLua [{}]: {}", mod, msg); break;
            case ShipLua::LogLevel::Info:  SPDLOG_INFO ("ShipLua [{}]: {}", mod, msg); break;
            case ShipLua::LogLevel::Warn:  SPDLOG_WARN ("ShipLua [{}]: {}", mod, msg); break;
            case ShipLua::LogLevel::Error: SPDLOG_ERROR("ShipLua [{}]: {}", mod, msg); break;
        }
    });
}
} // namespace

void Initialize() {
    if (gModHost) return;

    ShipLua::LuaApiHostContext ctx;
    ctx.gameId      = "mm";            // "oot" on Shipwright
    ctx.hostVersion = "4.0.2";         // Actual host version

    gModHost = std::make_unique<ShipLua::ModHost>(ctx, CreateLogger());

    auto shipCtx = Ship::Context::GetInstance();
    auto modsRoot = Ship::Context::GetPathRelativeToAppDirectory("mods", shipCtx->GetShortName());
    std::filesystem::create_directories(modsRoot);

    auto loaded = gModHost->LoadModsFromRoot(modsRoot, modsRoot / ".shiplua-cache");
    if (loaded.isOk()) {
        SPDLOG_INFO("ShipLua loaded {} mod(s)", loaded.value->loadedIds.size());
    }

    ShipLua::EventPayload ready{
        {"game_id", ctx.gameId}, {"host_version", ctx.hostVersion},
        {"runtime_version", ctx.runtimeVersion},
        {"api_version", std::string(ShipLua::Generated::kApiVersion)},
    };
    gModHost->DispatchEvent("game.ready", ready);
}

void Shutdown() { gModHost.reset(); }

} // namespace ShipLuaHost
```

Add `.cpp` to game target sources (glob or explicit list).

## 4. Call `Initialize()` / `Shutdown()`

At the port startup/shutdown point (in 2Ship it is `BenPort.cpp`; in
Shipwright, the equivalent):

```cpp
#include "ShipLuaBootstrap.h"
// ... during game startup:
ShipLuaHost::Initialize();
// ... during shutdown:
ShipLuaHost::Shutdown();
```

## 5. Compile and use

Compile the game normally (with your ROM to generate the assets — ShipLua **does not** handle
ROMs). Then use:

```text
<exe-folder>/mods/<your-mod>/ (manifest.toml + main.lua) or <your-mod>.shipmod
```

Open the game: the mods load and `game.ready` fires. See the logs at
`logs/<game>.log`.

---

## Game specific bindings (`ship.mm.*` / `ship.oot.*`)

The core only exposes the common API. The **adapter** installs exclusive game features
directly in each mod's `lua_State` (ShipLua exposes Lua headers to
privileged consumers precisely for this). Standard:

```cpp
extern "C" { #include "lua.h" }
// ... game internals (Actor_Spawn, gPlayState, etc.)

static int LuaSpawnDog(lua_State* L) {
    // ... call Actor_Spawn(...) using MM internals ...
    lua_pushboolean(L, /*ok*/ 1);
    return 1;
}

void InstallMmApi(lua_State* L) {          // Call for each loaded runtime
    lua_getglobal(L, "require"); lua_pushstring(L, "ship");
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) { lua_pop(L, 1); return; }
    lua_newtable(L);                       // ship.mm
    lua_pushcfunction(L, LuaSpawnDog);
    lua_setfield(L, -2, "spawn_dog");      // ship.mm.spawn_dog
    lua_setfield(L, -2, "mm");
    lua_pop(L, 1);
}
```

To publish a common event from the host (e.g. a keystroke), use
`gModHost->DispatchEvent("input.hotkey", payload)`. See the full example
(hotkey + dog spawn) in the MM fork and in
[`examples/dog-spawner`](../examples/dog-spawner/).

## Notes

- Keep the `extern/ship-lua` submodule commit **pinned** and record the reason when
  updating it (see `AGENTS.en.md`).
- ShipLua core **never** includes game headers; only the adapter (inside the
  fork) accesses internals.
- Distribute the **buildable fork** or **source + this guide** — never ROMs/assets.
