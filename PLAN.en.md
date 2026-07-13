> English translation of [PLAN.md](PLAN.md). The Portuguese document remains the canonical project record.

# PLAN.md — ShipLua

> Master plan for forking Shipwright and 2Ship2Harkinian and implementing a shared Lua modloader.
>
> Default GitHub owner: `BaiterYamato`
> Tentative name: `ShipLua`
> Upstream branches observed in preparation: `develop`
> Base date: `2026-07-12`

## 1. Expected result

At the end of this plan there will be:

```text
BaiterYamato/ship-lua
BaiterYamato/Shipwright
BaiterYamato/2ship2harkinian
```

With:

- Lua runtime isolated by mod;
- modloader with manifest;
- API shared between OoT and MM;
- adapters based on `GameInteractor`;
- sandboxed storage;
- events, timers and commands;
- specific abilities per game;
- development hot reload;
- documentation and definitions for Lua Language Server;
- CI for both forks;
- common mod package executable in both games;
- synchronization process with future upstream updates.

---

## 2. Decision: use `PLAN.md` and `AGENTS.md`

The files have different functions:

- `AGENTS.md`: permanent rules for all agents;
- `PLAN.md`: script, dependencies, phases and acceptance criteria.

Both must exist in the root of the `ship-lua` repository. A short copy or reference to the file must also exist in the forks.

---

## 3. Scope

### Included in the first cycle

- upstream forks and remotes;
- shared repository;
- CMake integration;
- Lua 5.4 PUC or equivalent decision documented by RFC;
- one Lua state per mod;
- TOML manifest;
- discovery and loading of mods;
- event bus;
- Capabilities API;
- OoT and MM adapters;
- initial common API;
- console commands;
- timers;
- storage;
- logging;
- error isolation;
- hot reload in dev mode;
- documentation codegen and types;
- conformity tests;
- packaging `.shipmod`.

### Outside the first cycle

- multiplayer;
- mod marketplace within the game;
- automatic download;
- native code execution;
- FFI;
- scripts with unrestricted network access;
- complete replacement of assets;
- complete ABI custom actor;
- compatibility with Lua mods from sm64coopdx without port;
- console support before the runtime is stable on desktop.

---

## 4. Bootstrap from repositories

### 4.1 Prerequisites

```bash
git --version
gh --version
cmake --version
python3 --version
gh auth status
```

The coordinating agent must confirm that the GitHub CLI is authenticated as the desired owner.

### 4.2 Create forks

```bash
export OWNER="${OWNER:-BaiterYamato}"

gh repo fork HarbourMasters/Shipwright \
  --clone=false

gh repo fork HarbourMasters/2ship2harkinian \
  --clone=false
```

To check:

```bash
gh repo view "$OWNER/Shipwright"
gh repo view "$OWNER/2ship2harkinian"
```

If the fork already exists, do not recreate it. Just validate owner, visibility and default branch.

### 4.3 Create the shared repository

```bash
gh repo create "$OWNER/ship-lua" \
  --public \
  --description "Modular Lua modloader and shared API for Shipwright and 2Ship2Harkinian" \
  --clone
```

In the clone:

```bash
cd ship-lua
mkdir -p \
  cmake \
  coordination/claims \
  coordination/handoffs \
  docs \
  examples \
  include/shiplua \
  rfcs \
  schema \
  src \
  tests/conformance \
  tools
touch coordination/STATUS.md
git add .
git commit -m "chore(repo): initialize ShipLua workspace"
git push origin main
```

### 4.4 Clone forks

```bash
mkdir ship-lua-workspace
cd ship-lua-workspace

git clone --recurse-submodules \
  "https://github.com/$OWNER/ship-lua.git" control

git clone --recurse-submodules \
  "https://github.com/$OWNER/Shipwright.git" shipwright

git clone --recurse-submodules \
  "https://github.com/$OWNER/2ship2harkinian.git" two-ship
```

### 4.5 Configure remotes

```bash
git -C shipwright remote rename origin fork
git -C shipwright remote add origin "https://github.com/$OWNER/Shipwright.git"
git -C shipwright remote add upstream "https://github.com/HarbourMasters/Shipwright.git"

git -C two-ship remote rename origin fork
git -C two-ship remote add origin "https://github.com/$OWNER/2ship2harkinian.git"
git -C two-ship remote add upstream "https://github.com/HarbourMasters/2ship2harkinian.git"
```

If the clone already has the correct `origin`, use the simplest configuration:

```bash
git -C shipwright remote add upstream \
  https://github.com/HarbourMasters/Shipwright.git

git -C two-ship remote add upstream \
  https://github.com/HarbourMasters/2ship2harkinian.git
```

The coordinator must record the actual result in `coordination/STATUS.md`. Don't blindly run both blocks.

### 4.6 Create integration branches

In each fork:

```bash
git fetch origin upstream

git checkout develop
git merge --ff-only upstream/develop
git push origin develop

git checkout -b lua/main
git push -u origin lua/main
```

Configure `lua/main` as a protected branch in the fork when possible.

---

## 5. Architectural structure

### 5.1 `ship-lua` Repository

```text
ship-lua/
├── AGENTS.md
├── PLAN.md
├── CMakeLists.txt
├── cmake/
│ └── ShipLuaConfig.cmake
├── coordination/
│ ├── STATUS.md
│ ├── claims/
│ └── handoffs/
├── include/shiplua/
│ ├── api/
│ ├── events/
│ ├── host/
│ ├── manifest/
│ ├── runtime/
│ ├── security/
│ └── storage/
├── src/
│ ├── api/
│ ├── events/
│ ├── manifest/
│ ├── runtime/
│ ├── security/
│ └── storage/
├── schema/
│ ├── api.yml
│ ├── capabilities.yml
│ ├── events.yml
│ └── manifest.schema.json
├── tools/
│ ├── api_codegen.py
│ ├── docs_codegen.py
│ ├── manifest_validator.py
│ └── package_mod.py
├── generated/
│ ├── moon/
│ ├── cpp/
│ └── docs/
├── examples/
│ ├── hello-world/
│ ├── low-gravity/
│ └── scene-logger/
├── tests/
│ ├── unit/
│ ├── security/
│ └── conformance/
└── rfcs/
```

### 5.2 Integration in forks

Add core as pinned submodule:

```bash
git submodule add \
  https://github.com/BaiterYamato/ship-lua.git \
  extern/ship-lua
```

Identical path in both forks:

```text
extern/ship-lua
```

CMake option:

```cmake
option(ENABLE_SHIP_LUA "Enable ShipLua modloader" ON)

if(ENABLE_SHIP_LUA)
    add_subdirectory(extern/ship-lua)
    target_link_libraries(<host-target> PRIVATE shiplua)
endif()
```

The real target must be identified by each adapter agent. Don't come up with the name without validating the host's CMake.

### 5.3 Adapters

Shipwright:

```text
soh/soh/ShipLua/
├── ShipwrightAdapter.cpp
├── ShipwrightAdapter.h
├── ShipwrightBootstrap.cpp
├── ShipwrightBindings.cpp
├── ShipwrightCapabilities.cpp
└── ShipwrightEventBridge.cpp
```

2Ship:

```text
mm/2s2h/ShipLua/
├── TwoShipAdapter.cpp
├── TwoShipAdapter.h
├── TwoShipBootstrap.cpp
├── TwoShipBindings.cpp
├── TwoShipCapabilities.cpp
└── TwoShipEventBridge.cpp
```

These paths can be adjusted to follow the host's actual organization, but must remain clearly isolated.

---

## 6. Essential interfaces

### 6.1 Host

```cpp
namespace ShipLua {

enum class GameId {
    OcarinaOfTime,
    MajorasMask,
};

struct HostInfo {
    GameId game;
    std::string hostVersion;
    std::string hostCommit;
    std::string runtimeVersion;
    std::string apiVersion;
};

class IGameAdapter {
  public:
    virtual ~IGameAdapter() = default;

    virtual HostInfo GetHostInfo() const = 0;
    virtual CapabilitySet GetCapabilities() const = 0;
    virtual Result<PlayerSnapshot> GetPlayer() const = 0;
    virtual Result<void> SetPlayerHealth(int32_t value) = 0;
    virtual Result<ActorHandle> SpawnActor(const ActorSpawnRequest& request) = 0;
    virtual Result<SceneSnapshot> GetCurrentScene() const = 0;
    virtual void RegisterEvents(EventDispatcher& dispatcher) = 0;
};

}
```

### 6.2 Handles

```cpp
struct ActorHandle {
    uint32_t slot;
    uint32_t generation;
    GameId game;
};
```

No pointer should cross to Lua.

### 6.3 Results

```cpp
enum class ErrorCode {
    Ok,
    Unsupported,
    InvalidArgument,
    InvalidHandle,
    InvalidState,
    PermissionDenied,
    ResourceLimit,
    HostFailure,
    ScriptFailure,
};

template <typename T>
struct Result {
    ErrorCode code;
    std::string message;
    std::optional<T> value;
};
```

---

## 7. Initial Lua API

### 7.1 Initialization

```lua
local ship = require("ship")

ship.log.info("Loaded on " .. ship.game.id())
```

### 7.2 Identification and capabilities

```lua
ship.game.id()
ship.game.host_version()
ship.runtime.version()
ship.api.version()
ship.capabilities.has("scene.events")
ship.capabilities.list()
```

### 7.3 Common events

```text
game.ready
game.frame
game.shutdown

save.loaded
save.before_write
save.after_write

scene.enter
scene.ready
scene.leave

room.enter
room.leave

actor.spawn
actor.init
actor.update
actor.destroy

player.update
player.health_changed
player.item_received

text.open
input.update
audio.sequence_started
```

Not all will be implemented in the initial MVP. The schema must declare:

- host support;
- availability phase;
- payload;
- possibility of cancellation;
- order;
- priority.

### 7.4 Common MVP Functions

```lua
ship.events.on(name, options_or_callback)
ship.events.off(subscription)

ship.game.id()
ship.game.host_version()

ship.player.get()
ship.player.set_health(value)
ship.player.damage(amount)
ship.player.heal(amount)
ship.player.teleport(destination)

ship.actor.get(handle)
ship.actor.find(query)
ship.actor.spawn(spec)

ship.scene.current()

ship.console.register(name, options, callback)
ship.timer.after(frames, callback)
ship.timer.every(frames, callback)
ship.storage.open()
ship.log.debug(message)
ship.log.info(message)
ship.log.warn(message)
ship.log.error(message)
```

### 7.5 Specific namespaces

```lua
ship.oot.*
ship.mm.*
```

Examples of capabilities:

```text
oot.player.age
oot.ocarina
oot.master_quest

mm.cycle
mm.masks
mm.owl_save
mm.clock
```

---

## 8. Manifest

Required file:

```text
manifest.toml
```

Example:

```toml
id = "community.low_gravity"
name = "Low Gravity"
version = "0.1.0"
api = ">=0.1 <1.0"
entrypoint = "main.lua"
description = "Limits downward velocity."
authors = ["Example Author"]

games = ["oot", "mm"]

[host]
shipwright = ">=9.0"
two_ship = ">=4.0"

[capabilities]
required = ["player.velocity.read", "player.velocity.write"]
optional = ["mm.cycle", "oot.player.age"]

[dependencies]
"community.core_utils" = ">=1.0 <2.0"

[load]
priority = 50
after = ["community.core_utils"]

[permissions]
storage = true
network = false
clipboard = false
```

Package:

```text
low-gravity.shipmod
├── manifest.toml
├── main.lua
├── scripts/
├──assets/
├── locale/
└── README.md
```

The physical format must be ZIP with its own extension, but the logic must not depend solely on the extension.

---

## 9. Event system

Types:

| Type | Behavior |
|---|---|
| `observe` | does not modify result |
| `filter` | allow or block |
| `transform` | transforms value |
| `consume` | ends propagation |

Deterministic ordering:

1. dependencies;
2. `load.after` and `load.before`;
3. mod priority;
4. callback priority;
5. Mod ID;
6. Registration ID.

The host's `GameInteractor` does not provide enough stable public order to be the API contract. The order must be controlled in `EventDispatcher`.

---

## 10. Execution phases

## Phase 0 — Governance and bootstrap

### Objective

Create the three repositories, branches, coordination structure and baseline builds.

### Tasks

| ID | Task | It depends | Parallel |
|---|---|---|---|
| GOV-001 | Create forks and remotes | — | no |
| GOV-002 | Create `ship-lua` | GOV-001 | no |
| GOV-003 | Add `AGENTS.md`, `PLAN.md`, status and templates | GOV-002 | no |
| CI-001 | Compile Shipwright baseline | GOV-001 | yes |
| CI-002 | Compile 2Ship baseline | GOV-001 | yes |
| ARCH-001 | Inventory common and unique hooks | GOV-002 | yes |
| ARCH-002 | Write Architectural RFC | ARCH-001 | no |

### Accept

- three repositories exist;
- configured upstream;
- baseline compiles;
- registered baseline hashes;
- no protected files were committed;
- tasks can be claimed without conflict.

---

## Phase 1 — Runtime Core

### Objective

Run an isolated Lua script with no game dependency.

### Tasks

| ID | Task | It depends |
|---|---|---|
| CORE-001 | Integrate Lua and create `LuaRuntime` | ARCH-002 |
| CORE-002 | One state per mod | CORE-001 |
| CORE-003 | allowed libraries and initial sandbox | CORE-001 |
| CORE-004 | logging structured by mod | CORE-002 |
| CORE-005 | protected error handling | CORE-002 |
| CORE-006 | memory limit per state | CORE-002 |
| CORE-007 | lifecycle init/load/unload | CORE-002 |
| TEST-001 | runtime unit tests | CORE-001 |

### Accept

- two mods have isolated states;
- one error does not end the other;
- `io`, `debug`, `os.execute` and DLL/SO are not accessible;
- unload releases resources;
- tests run without ROM.

---

## Phase 2 — Manifest, discovery and dependencies

### Objective

Find, validate, sort and load mods.

### Tasks

| ID | Task | It depends |
|---|---|---|
| MOD-001 | manifest schema | ARCH-002 |
| MOD-002 | TOML parser | MOD-001 |
| MOD-003 | directory and package discovery | MOD-002 |
| MOD-004 | SemVer resolution | MOD-002 |
| MOD-005 | dependency graph | MOD-004 |
| MOD-006 | deterministic order | MOD-005 |
| MOD-007 | cycle and conflict detection | MOD-005 |
| MOD-008 | Orchestrate Root Discovery, Compatibility, and Load | MOD-003, MOD-007, BIND-001 |
| TOOL-001 | CLI validator | MOD-001 |
| TEST-002 | valid and invalid fixtures | MOD-002 |

### Accept

- manifest errors are readable;
- missing dependency only prevents the affected mod;
- cycles are detected;
- order is reproducible;
- package cannot escape the extraction directory.

---

## Phase 3 — Event bus and base API

### Objective

Define common contract before deeply onboarding hosts.

### Tasks

| ID | Task | It depends |
|---|---|---|
| API-001 | `schema/api.yml` | ARCH-002 |
| API-002 | `schema/events.yml` | API-001 |
| API-003 | capabilities | API-001 |
| EVENT-001 | dispatcher | API-002 |
| EVENT-002 | priorities | EVENT-001 |
| EVENT-003 | observe/filter/transform/consume | EVENT-001 |
| TIMER-001 | timers per frame | EVENT-001 |
| CODEGEN-001 | generate C++ bindings | API-001 |
| CODEGEN-002 | generate LuaDoc | API-001 |
| CODEGEN-003 | generate Markdown | API-001 |

### Accept

- ordered events;
- unsubscribe works;
- invalid callback is isolated;
- documentation and bindings derive from the same source;
- API does not have pointers.

---

## Phase 4 — Bootstrap on both hosts

### Objective

Boot the same core in both games.

### Parallel trails

#### Shipwright

| ID | Task |
|---|---|
| OOT-001 | add submodule and CMake |
| OOT-002 | bootstrap on startup |
| OOT-003 | secure shutdown |
| OOT-004 | detect version and commit |
| OOT-005 | find mod directory |
| OOT-006 | minimal menu/console |

#### 2Ship

| ID | Task |
|---|---|
| MM-001 | add submodule and CMake |
| MM-002 | bootstrap on startup |
| MM-003 | secure shutdown |
| MM-004 | detect version and commit |
| MM-005 | find mod directory |
| MM-006 | minimal menu/console |

### Accept

The same `hello-world.shipmod` records a message on both hosts.

---

## Phase 5 — Bridging Common Events

### Objective

Map `GameInteractor` to stable events.

### Initial matrix

| Public event | Shipwright | 2Ship |
|---|---|---|
| `game.frame` | `OnGameFrameUpdate` | `OnGameStateUpdate` |
| `scene.enter` | `OnSceneInit` | `OnSceneInit` |
| `actor.init` | `OnActorInit` | `OnActorInit` |
| `actor.update` | `OnActorUpdate` | `OnActorUpdate` |
| `actor.destroy` | `OnActorDestroy` | `OnActorDestroy` |
| `save.loaded` | `OnLoadFile`/`OnLoadGame` | `OnSaveLoad` |
| `text.open` | `OnOpenText` | `OnOpenText` |
| `audio.sequence_started` | `OnSeqPlayerInit` | `OnSeqPlayerInit` |

The actual signature must be committed to the current commit before deployment.

### Tasks

| ID | Task | Trail |
|---|---|---|
| OOT-EVT-001 | lifecycle and frame | OoT |
| MM-EVT-001 | lifecycle and frame | MM |
| OOT-EVT-002 | scenes | OoT |
| MM-EVT-002 | scenes and rooms | MM |
| OOT-EVT-003 | actors | OoT |
| MM-EVT-003 | actors | MM |
| OOT-EVT-004 | save and text | OoT |
| MM-EVT-004 | save and text | MM |
| CONF-001 | compare payloads | common |
| CONF-002 | golden event traces | common |

### Accept

A single `scene-logger` mod receives a compatible payload in both games.

---

## Phase 6 — Player, actors and handles

### Objective

Provide secure reading and writing.

### Tasks

| ID | Task |
|---|---|
| HANDLE-001 | handle registry |
| HANDLE-002 | generation and invalidation |
| PLAYER-001 | common snapshot |
| PLAYER-002 | health |
| PLAYER-003 | position and speed |
| PLAYER-004 | teleport validated |
| ACTOR-001 | common snapshot |
| ACTOR-002 | query by ID/category |
| ACTOR-003 | spawn validated |
| ACTOR-004 | destroy with permission |
| SECURITY-001 | prevent use-after-free |
| CONF-003 | common health mod |
| CONF-004 | common actor logger mod |

### Accept

- handle destroyed returns `invalid_handle`;
- Lua does not receive an address;
- limits are validated;
- Common API works in both games;
- divergences are specific capabilities.

---

## Phase 7 — Storage, console and hot reload

### Tasks

| ID | Task |
|---|---|
| STORE-001 | VFS by mod |
| STORE-002 | quota of bytes and files |
| STORE-003 | atomic writing |
| STORE-004 | path traversal blocks |
| CONSOLE-001 | command registration |
| CONSOLE-002 | removal on unload |
| RELOAD-001 | dev file watch |
| RELOAD-002 | transactional unload/reload |
| RELOAD-003 | optional state preservation |
| SECURITY-002 | path fuzz |
| SECURITY-003 | malicious ZIP package |

### Accept

- mod does not access files from another mod;
- reload does not duplicate callbacks;
- command disappears when unloading;
- malicious packet is refused.

---

## Phase 8 — Specific Resources

### OoT

```text
oot.player.age
oot.ocarina
oot.master_quest
oot.equipment
oot.dungeon_keys
```

###MM

```text
mm.cycle
mm.clock
mm.masks
mm.owl_save
mm.room
```

Each group requires its own RFC and corresponding capability.

---

## Phase 9 — DX, documentation and community

### Tasks

| ID | Task |
|---|---|
| DOC-001 | website/API reference |
| DOC-002 | tutorial first mod |
| DOC-003 | C++ porting guide |
| DOC-004 | OoT/MM differences guide |
| TOOL-002 | `shipmod init` |
| TOOL-003 | `shipmod validate` |
| TOOL-004 | `shipmod pack` |
| TOOL-005 | Lua Language Server settings |
| EXAMPLE-001 | hello world |
| EXAMPLE-002 | low gravity |
| EXAMPLE-003 | scene logger |
| EXAMPLE-004 | capability-aware mod |

### Accept

A developer can create, validate and package a mod without reading the C++ code.

---

## Phase 10 — CI, future releases and updates

### Workflows

In `ship-lua`:

```text
core-linux.yml
core-windows.yml
core-macos.yml
security.yml
codegen-diff.yml
package-examples.yml
```

In forks:

```text
shiplua-host-build.yml
shiplua-conformance.yml
upstream-drift.yml
```

### Integration matrix

```text
Shipwright/develop + ship-lua/main
2ship2harkinian/develop + ship-lua/main
```

### Drift detection

Automation should observe changes in:

```text
GameInteractor.h
GameInteractor.cpp
GameInteractor_HookTable.h
GameInteractor_Hooks.h
CMakeLists.txt
bootstrap/global initialization
libultraship submodule
```

When there is a break:

1. IC fails;
2. report lists missing symbols;
3. issue is opened;
4. adapter is fixed;
5. Public API remains unchanged when possible.

---

## 11. Synchronization with upstream

Run separately on each fork.

```bash
git fetch upstream origin

git checkout develop
git merge --ff-only upstream/develop
git push origin develop

git checkout lua/main
git merge --no-ff develop \
  -m "chore(upstream): merge upstream develop"
git push origin lua/main
```

After:

```bash
git submodule update --init --recursive
cmake --build <build-dir>
ctest --test-dir <build-dir> --output-on-failure
```

Do not rewrite the history of `lua/main`.

---

## 12. Multi-agent strategy

### Roles

| Paper | Responsibility |
|---|---|
| Integrator | architecture, merges and releases |
| Core Runtime | Moon, lifecycle and errors |
| API/Codegen | schemas, bindings and docs |
| OoT Adapter | Shipwright |
| MM Adapter | 2Ship |
| Security | sandbox, package and storage |
| Build/CI | CMake and workflows |
| QA | conformance and regression |
| Docs/DX | guides, examples and tools |

An agent can occupy more than one role, but a task can only have one main person responsible.

### Secure side work

After Phase 3:

```text
┌─ OoT Adapter ─┐
Core/API ───────┤ ├── Conformance
                └─ MM Adapter ──┘
```

Do not start adapters before freezing the first minimum event and capacity contract.

### Periodic integration

For each small group of tasks:

1. merge into `ship-lua/main`;
2. update submodule commit in both forks;
3. perform conformance;
4. record result in `coordination/STATUS.md`.

---

## 13. Agent-ready initial backlog

### Coordination

- [x] `GOV-001` — create forks;
- [x] `GOV-002` — create repository `ship-lua`;
- [x] `GOV-003` — install coordination files;
- [x] `ARCH-001` — generate hook inventory;
- [x] `ARCH-002` — Architecture RFC 0001.

### Baseline

- [ ] `CI-001` — Shipwright Linux;
- [ ] `CI-002` — 2Ship Linux;
- [ ] `CI-003` — Shipwright Windows;
- [ ] `CI-004` — 2Ship Windows.

### Core

- [x] `CORE-001` — runtime;
- [x] `CORE-002` — insulation;
- [x] `CORE-003` — sandbox;
- [x] `CORE-004` — logging;
- [x] `CORE-005` — errors;
- [x] `CORE-006` — memory;
- [x] `CORE-007` — lifecycle.

###Modloader

- [x] `MOD-001` — manifest;
- [x] `MOD-002` — parser;
- [x] `MOD-003` — discovery;
- [x] `MOD-004` — SemVer;
- [x] `MOD-005` — dependencies;
- [x] `MOD-006` — load order;
- [x] `MOD-007` — conflicts.

###API

- [x] `API-001` — functions;
- [x] `API-002` — events;
- [x] `API-003` — capabilities;
- [x] `EVENT-001` — dispatcher;
- [x] `CODEGEN-001` — C++;
- [x] `CODEGEN-002` — LuaDoc;
- [x] `CODEGEN-003` — docs.

### Hosts

- [ ] `OOT-001` to `OOT-006`;
- [ ] `MM-001` to `MM-006`;
- [ ] `OOT-EVT-001` to `OOT-EVT-004`;
- [ ] `MM-EVT-001` to `MM-EVT-004`.

### Security and DX

- [ ] `STORE-001` to `STORE-004`;
- [ ] `SECURITY-001` to `SECURITY-003`;
- [ ] `RELOAD-001` to `RELOAD-003`;
- [x] `TOOL-001` to `TOOL-005`;
- [ ] `DOC-001` to `DOC-004`.

---

## 14. First vertical slice

Before extending the API, deliver this complete flow:

1. install `hello-world.shipmod`;
2. host discovers manifest;
3. runtime creates isolated state;
4. script imports `ship`;
5. registers `game.ready`;
6. receives event;
7. write log with game and version;
8. unload without leaving callback;
9. Same file works in OoT and MM.

### Test Mod

`manifest.toml`:

```toml
id = "example.hello_world"
name = "Hello World"
version = "0.1.0"
api = ">=0.1 <0.2"
entrypoint = "main.lua"
games = ["oot", "mm"]
```

`main.lua`:

```lua
local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info(
        "Hello from " ..
        ship.game.id() ..
        " host=" ..
        ship.game.host_version()
    )
end)
```

Do not advance to physics, actors or assets before this slice is green on both hosts.

---

## 15. Risks and mitigation

| Risk | Mitigation |
|---|---|
| divergence of `GameInteractor` | adapters and capabilities |
| changing internal structs | snapshots and handles |
| duplication between forks | shared core |
| non-deterministic order | own dispatcher |
| mod crashes the game | limits and deactivation |
| arbitrary filesystem | VFS by mod |
| upstream update breaks build | Drift IC |
| conflict between agents | claims, worktrees and handoffs |
| API grows without coherence | RFC and codegen |
| assets protected in releases | scanners and strict policy |
| submodule outdated | pinned update and commit bot |
| official core not accepted upstream | forks remain functional; modular integration |

---

## 16. Version 0.1.0 Criteria

Version `0.1.0` can be published when:

- Shipwright and 2Ship compile with ShipLua;
- `hello-world`, `scene-logger` and a common player mod run on both;
- manifest and dependencies work;
- runtime isolates mods;
- storage blocks traversal;
- hot reload does not duplicate hooks;
- documentation is generated;
- API has SemVer;
- CI Linux and Windows is green;
- no ROM or protected asset exists in the repositories;
- release contains checksums;
- known limitations are documented.

---

## 17. Final project criteria

The project is considered functional and community usable when:

1. mods are installed without recompiling;
2. the same common mod runs in both games;
3. Unique features are discoverable;
4. Upstream updates require changes focused on adapters;
5. errors in one mod do not take down the others;
6. no mods receive arbitrary native access;
7. the documentation derives from the same schema as the bindings;
8. any agent can continue a task from the coordination files.
