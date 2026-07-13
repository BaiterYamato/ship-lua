> English translation of [rfcs/0001-shiplua-architecture.md](0001-shiplua-architecture.md). The Portuguese document remains the canonical project record.

# RFC 0001 — Shared Runtime Architecture

- Status: proposal for review
- Date: 2026-07-13
- Tasks: ARCH-001, ARCH-002

## Motivation

Shipwright and 2Ship2Harkinian expose similar hooks, but they do not have identical names, signatures, or lifecycles. Copying the runtime for both forks would create divergence and force common mods to know the internal structures of each game.

This RFC sets a boundary: the shared core knows only ShipLua interfaces and types; each fork has a small adapter that translates the `GameInteractor` and the host lifecycle.

## Architecture

```text
Lua mod
  ↓
Versioned Lua API + capabilities
  ↓
runtime, dispatcher, manifest and shared services
  ↓
IGameAdapter
  ├── ShipwrightAdapter
  └── TwoShipAdapter
       ↓
GameInteractor + host internal frameworks
```

The `ship-lua` repository is the canonical source. Forks contain strictly necessary CMake integration, bootstrap, adapter and hooks.

## Adapter C++ contract

The initial contract must be equivalent to:

```cpp
enum class GameId { Oot, Mm };

struct HostInfo {
    GameId game;
    std::string hostVersion;
    std::string hostCommit;
};

class IGameAdapter {
  public:
    virtual ~IGameAdapter() = default;
    virtual HostInfo GetHostInfo() const = 0;
    virtual std::vector<std::string> GetCapabilities() const = 0;
    virtual Result<void> RegisterHooks(IHostEventSink& sink) = 0;
    virtual void UnregisterHooks() noexcept = 0;
};
```

Rules:

- `IGameAdapter` and core do not include OoT/MM headers;
- the adapter owns the registered `HOOK_ID`;
- `UnregisterHooks()` is idempotent;
- callbacks enter the core only on the main thread;
- exceptions never traverse host C/C++ callbacks;
- internal pointers are converted before calling `IHostEventSink`.

## Initial Lua Contract

The first vertical slice only exposes:

```lua
local ship = require("ship")

ship.game.id()
ship.game.host_version()
ship.runtime.version()
ship.api.version()
ship.capabilities.has(name)
ship.capabilities.list()

ship.events.on("game.ready", function(event) end)
ship.events.off(subscription)

ship.log.debug(message)
ship.log.info(message)
ship.log.warn(message)
ship.log.error(message)
```

`require("ship")` is resolved by internal loader. The runtime does not open `package`, native filesystem, or DLL/OS to implement this module.

## Events of the first contract

| Event | Common Payload | OoT | MM |
|---|---|---:|---:|
| `game.ready` | `game_id`, versions | yes | yes |
| `game.frame` | runtime monotonic counter | yes | yes |
| `game.shutdown` | empty | yes | yes |
| `scene.enter` | `scene_id` | yes | yes |
| `actor.init/update/destroy` | handle + stable snapshot | yes | yes |
| `save.loaded` | logical slot | yes | yes |
| `text.open` | `text_id` read-only | yes | yes |
| `audio.sequence_started` | `player_index`, `sequence_id` | yes | yes |

The frame counter belongs to the runtime. `scene.enter` does not invent `spawn_id` in OoT. Additional MM resources are published under `ship.mm.*` and corresponding capability.

## Capabilities

Initial common capabilities:

```text
scene.events
actor.events
save.events
text.events
audio.sequence.events
```

Specific candidate capabilities:

```text
mm.room.events
mm.cycle
mm.owl_save
mm.clock
oot.ocarina
oot.dungeon_keys
oot.equipment
```

A capability is only announced when the adapter fully implements and tests its semantics. Absence returns `false`; It is not an error and does not activate simulation.

## Behavior by host

### Shipwright

- bootstrap after creating `GameInteractor::Instance` in `OTRGlobals.cpp`;
- `game.frame` derives from `OnGameFrameUpdate`;
- `scene.enter` derives from `OnSceneInit(sceneNum)`;
- save requires deduplication between `OnLoadGame` and `OnLoadFile`;
- actor callbacks convert `void*` to snapshot on OoT adapter.

### 2Ship2Harkinian

- bootstrap after creating `GameInteractor::Instance` in `BenPort.cpp`;
- `game.frame` derives from `OnGameStateUpdate`;
- `scene.enter` derives from `OnSceneInit(sceneId, spawnNum)`, but the common payload contains only scene;
- `OnRoomInit` and cycle events require MM capabilities;
- actor callbacks convert `Actor*` to snapshot on MM adapter.

## Handles and service life

Lua never receives a pointer or address. A public handle contains, at a minimum:

```cpp
struct ActorHandle {
    std::uint32_t slot;
    std::uint32_t generation;
    GameId game;
};
```

The registry invalidates the generation before issuing `actor.destroy`. Every read or write validates host, slot, generation and current state.

## Order and isolation

The `GameInteractor` carries the raw signal; it does not define ShipLua public order. The shared dispatcher sorts by:

1. dependencies resolved;
2. `load.after`/`load.before`;
3. mod priority;
4. callback priority;
5. Mod ID;
6. Registration ID.

Each callback uses protected Lua calls. Failure increments the mod's counter, preserves other runtimes and can only disable the repeating mod.

## Errors

C++ borders returns `Result<T>` with `ErrorCode`. Lua receives stable error, no message dependent on internal host layout.

```text
unsupported
invalid_argument
invalid_handle
invalid_state
permission_denied
resource_limit
host_failure
script_failure
```

Protected function never throws exception through the C/Lua ABI or `GameInteractor` callback.

## Compatibility and versions

They are independent: `host_version`, `runtime_version`, `api_version` and `mod_version`.

The API uses SemVer. Compatible resource increments `MINOR`; contractually compatible fix increments `PATCH`; breaking requires `MAJOR` and RFC. Host update does not automatically change `api_version`.

Deprecated API remains functional for at least one `MINOR` cycle, receives documented deprecation, and is only removed in `MAJOR`.

## Security

- no `io`, `debug`, `os.execute`, `package.loadlib`, FFI, network or subprocesses by default;
- no host headers or pointers cross the adapter;
- callbacks execute on the main thread;
- package and storage blocks absolute, `..`, symlinks and portable collisions;
- memory limits, callbacks, files and bytes are configurable;
- no telemetry;
- no automatic executable downloads or updates.

## Test plan

1. core unit testing without ROM;
2. adapter tests with synthetic sinks for each mapped signature;
3. golden traces comparing OoT/MM payloads;
4. common mod `hello-world.shipmod` on both hosts;
5. unload proving removal of hooks and callbacks;
6. drift check of HookTable, bootstrap and CMake on both hosts;
7. Mandatory Linux and Windows matrix; macOS recommended before first release.

## Implementation plan

1. API schemas, events and capabilities;
2. dispatcher and internal module `ship`;
3. `IGameAdapter` and synthetic sink in the core;
4. minimal OoT and MM bootstrap;
5. lifecycle/frame, then scene, actors and save;
6. conformance of the first vertical slice;
7. specific capabilities only by additional RFC.

## Depreciation

Changes to the `IGameAdapter` public boundary, event names, or payloads require amendment/RFC, compatibility analysis, and migration plan. Private adapter details can change to keep upstream without changing the Lua API.

## Rejected alternatives

- duplicate runtime in forks: creates drift and two APIs;
- expose all headers automatically: leaks unstable ABI and pointers;
- use LuaJIT FFI: removes isolation and portability;
- make `GameInteractor` the public contract: names and signatures already differ;
- simulate room/cycle in OoT: produces false semantics and breaks capability detection.
