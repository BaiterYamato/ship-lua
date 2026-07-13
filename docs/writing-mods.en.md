> English translation of [docs/writing-mods.md](writing-mods.md). The Portuguese document remains the canonical project record.

# Writing mods for ShipLua

This guide shows how to create a Lua mod that runs on Shipwright (OoT) and 2Ship (MM).

> Complete and always updated API reference (generated from schemas):
> [`generated/docs/api-reference.en.md`](../generated/docs/api-reference.en.md).

## 1. Anatomy of a mod

A mod is a folder (or a `.shipmod`, which is just that ZIP folder) with at least:

```text
my-mod/
├── manifest.toml # metadata: id, version, compatibility, dependencies
└── main.lua # entry point
```

Optionally: `scripts/`, `assets/`, `locale/`, `README.md`.

## 2. The manifest (`manifest.toml`)

```toml
id = "community.my_mod"       # Required, unique, reverse-domain style
name = "My Mod"               # Required
version = "0.1.0"             # Required, SemVer
api = ">=0.1 <0.2"            # Required API range supported by the mod
entrypoint = "main.lua"       # Required
description = "What it does."
authors = ["Your Name"]

games = ["oot", "mm"]         # Games it runs on (empty = both)

[host]                         # Optional host version ranges
shipwright = ">=9.0"
two_ship = ">=4.0"

[capabilities]                 # Optional resources the mod needs or uses
required = ["scene.events"]   # The mod will not load if missing from the host
optional = ["mm.cycle"]       # Detect at runtime with ship.capabilities.has

[dependencies]                 # Optional dependencies on other mods
"community.core_utils" = ">=1.0 <2.0"

[load]                         # Optional load order
priority = 50
after = ["community.core_utils"]

[permissions]                  # Optional
storage = true
network = false
```

Only `id`, `name`, `version`, `api` and `entrypoint` are required. The loader validates
`games`, `host` and `api` against the current game and **refuses** the mod (without taking down the others)
if it is incompatible.

## 3. The entry point (`main.lua`)

`main.lua` runs once when the mod loads. Here you register event callbacks.

```lua
local ship = require("ship")

ship.log.info("my-mod loaded")

ship.events.on("game.ready", function()
    ship.log.info("Running on " .. ship.game.id() .. " host=" .. ship.game.host_version())
end)
```

Each mod runs in an **isolated Lua state** with a sandbox: `io`, `os.execute`,
`debug`, `package`, `dofile`/`loadfile` are **not** available. An error in a mod
does not affect others.

## 4. The `ship` API

### Log
```lua
ship.log.debug(msg)   ship.log.info(msg)
ship.log.warn(msg)    ship.log.error(msg)
```

### Identity and versions
```lua
ship.game.id()            -- "oot" or "mm"
ship.game.host_version()  -- Game version, e.g. "4.0.2"
ship.runtime.version()    -- ShipLua runtime version
ship.api.version()        -- API version
```

### Capabilities (discoverable resources)
```lua
if ship.capabilities.has("mm.cycle") then
    -- Only runs where this capability exists
end
for _, cap in ipairs(ship.capabilities.list()) do ... end
```

### Events
```lua
local sub = ship.events.on("game.ready", function(payload) ... end)
ship.events.off(sub)   -- Cancel the subscription

-- With options (lower priority runs first):
ship.events.on("game.frame", { priority = 10 }, function(payload)
    -- payload.frame
end)
```

The callback receives the **payload** of the event as a Lua table (fields as per the
schema of each event).

### Common events available

| Event | Payload | Note |
|---|---|---|
| `game.ready` | `game_id, host_version, runtime_version, api_version` | triggered at boot, after loading the mods |
| `game.frame` | `frame` | per frame (when host publishes) |
| `game.shutdown` | — | when closing |
| `input.hotkey` | `action, key` | host-configured keyboard shortcut |
| `scene.enter` | `scene_id` | requires capability `scene.events` |
| `actor.init` / `actor.update` / `actor.destroy` | `actor` / `handle` | requires `actor.events` |
| `save.loaded` | `slot` | requires `save.events` |
| `text.open` | `text_id` | requires `text.events` |
| `audio.sequence_started` | `player_index, sequence_id` | requires `audio.sequence.events` |

Not every host publishes all events — use `ship.capabilities.has(...)` for those
conditionals.

### Game-specific namespaces

Exclusive features are in `ship.mm.*` (Majora's Mask) and `ship.oot.*` (Ocarina).
They are installed by the host adapter, so **check for them** before using:

```lua
if ship.mm ~= nil and ship.mm.spawn_dog ~= nil then
    ship.mm.spawn_dog()   -- Spawn a Clock Town dog (MM)
end
```

## 5. Packaging and Installing

During development, copy the mod **folder** directly to `mods/`:

```text
<game-folder>/mods/my-mod/{manifest.toml, main.lua}
```

To distribute, package as `.shipmod` (ZIP with `manifest.toml` + `main.lua` in
file root). Example with Python:

```python
import zipfile
with zipfile.ZipFile("my-mod.shipmod", "w", zipfile.ZIP_DEFLATED) as z:
    z.write("my-mod/manifest.toml", "manifest.toml")
    z.write("my-mod/main.lua", "main.lua")
```

Place the `.shipmod` (or the folder) in `mods/` and open the game. Check the loading
in the game log (`logs/...log`). If a mod is rejected, the log tells you why
(game, version or API incompatibility).

## 6. Complete example: hotkey that spawns a dog (MM)

See [`examples/dog-spawner`](../examples/dog-spawner/). Host publishes `input.hotkey`
when the configured key (default **F**) is pressed; the mod reacts and calls
`ship.mm.spawn_dog()`:

```lua
local ship = require("ship")

ship.events.on("input.hotkey", function(event)
    if event.action == "spawn_dog" and ship.mm and ship.mm.spawn_dog then
        ship.mm.spawn_dog()
    end
end)
```

This shows the recommended pattern: **common events** (`input.hotkey`) core +
a **game-specific function** (`ship.mm.*`) provided by the adapter.

## 7. Validating the manifest (optional)

The core brings a CLI validator:

```bash
shiplua_manifest_validator path/to/my-mod/manifest.toml
shiplua_manifest_validator path/to/my-mod          # Folder
shiplua_manifest_validator my-mod.shipmod          # Package
```

## Good practices

- Check `ship.capabilities.has(...)` before using non-universal features.
- Don't assume that `ship.mm`/`ship.oot` exist — check with `~= nil`.
- A mod that uses only the common API should run in both games without changes.
- Prefer explicit `priority` if the order between callbacks matters.
