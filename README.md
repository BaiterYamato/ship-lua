# ShipLua

**English** · [Português](README.pt-BR.md)

**A shared Lua modloader for [Shipwright](https://github.com/HarbourMasters/Shipwright) (Ocarina of Time) and [2Ship2Harkinian](https://github.com/HarbourMasters/2ship2harkinian) (Majora's Mask).**

Write a Lua mod **once** and run it in both games. Game-exclusive features sit behind capabilities and namespaces (`ship.mm.*`, `ship.oot.*`), so nothing is faked when a feature doesn't exist in the other game.

```lua
local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info("Hello from " .. ship.game.id() .. " host=" .. ship.game.host_version())
end)
```

That same file prints `Hello from mm ...` in 2Ship and `Hello from oot ...` in Shipwright.

---

## Status

| Piece | State |
|---|---|
| Isolated, sandboxed Lua 5.4 runtime per mod | ✅ |
| Manifest, discovery, SemVer, dependency graph | ✅ |
| Event dispatcher (observe/filter/transform/consume) + timers | ✅ |
| `ship.*` API + codegen (C++ bindings + LuaDoc) | ✅ |
| Root loader (`.shipmod`, compatibility, dep ordering, failure isolation) | ✅ |
| Green CI on Linux + Windows | ✅ |
| **2Ship (MM)** adapter loading mods in-game | ✅ (`hello-world`, `dog-spawner`) |
| **Shipwright (OoT)** adapter | in progress |

Integration details in [`coordination/INTEGRATION.md`](coordination/INTEGRATION.md).

---

## Start here

- **Want to write a mod?** → **[Guide: Writing mods](docs/writing-mods.md)**
- **API reference** (generated from the schemas) → [`generated/docs/api-reference.md`](generated/docs/api-reference.md)
- **Examples** → [`examples/`](examples/)
  - [`hello-world`](examples/hello-world/) — the minimal mod (logs the host identity).
  - [`dog-spawner`](examples/dog-spawner/) — an **F** hotkey that spawns a dog in MM (uses `ship.mm.*`).

## Installing a mod in the game

Mods live in a `mods/` folder next to the game executable. Each mod is either:

- a **folder** with `manifest.toml` + `main.lua` (easiest while developing), or
- a **`.shipmod`** file (a ZIP with the same files).

```text
<game-folder>/
└── mods/
    ├── hello-world.shipmod
    └── my-mod/
        ├── manifest.toml
        └── main.lua
```

Launch the game: mods are discovered, validated (game/version/API), loaded in
dependency order, and the `game.ready` event fires. One mod failing never takes
down the others. Logs go to the console and to the game's log file
(e.g. `logs/2 Ship 2 Harkinian.log`).

## Building the core (development)

Requires CMake ≥ 3.20, Ninja and a C++20 compiler. Lua 5.4, toml++ and miniz are
fetched via FetchContent.

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows with MinGW, make sure the runtime is on `PATH` (otherwise the test
executables hang on a missing-DLL dialog):

```powershell
$env:Path = "C:\ProgramData\mingw64\mingw64\bin;$env:Path"
```

## Architecture (overview)

```text
Lua mod
   ↓
versioned public API (ship.*)     ← generated from schema/*.yml
   ↓
shared runtime + services (this repository)
   ↓
IGameAdapter
   ├── ShipwrightAdapter (OoT)
   └── TwoShipAdapter (MM)   ← loads mods, dispatches events, exposes ship.mm.*
```

The core **never** includes game headers. Each adapter (inside each game's fork)
translates internal structures into snapshots/handles/events and may install
host-specific bindings (`ship.mm.*`, `ship.oot.*`).

## Contributing

Read [`AGENTS.md`](AGENTS.md) (working contract) and [`PLAN.md`](PLAN.md) (roadmap).
Changes to the public API require an RFC in [`rfcs/`](rfcs/). Coordination state
lives in [`coordination/`](coordination/).

## License

The ShipLua core is under the repository license. **Never** commit ROMs,
`.z64`/`.n64`/`.o2r` files, or any copyrighted assets.
