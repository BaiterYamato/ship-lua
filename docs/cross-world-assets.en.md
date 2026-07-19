# Cross-World Assets — MM assets inside OOT and vice-versa

> Since SDK 0.4 the Link-Span hosts mount the neighbouring game's archive at
> runtime: OOT reads `../MM/mm.o2r` and MM reads `../OOT/oot.o2r`. No asset is
> copied or extracted — mods reference the other game's paths and the host
> resolves them straight from its archive.

## Installation layout

The mechanism relies on the sibling-folder layout created by the launcher:

```text
Link-Span/
├── link-span.exe
├── OOT/
│   ├── soh.exe
│   ├── oot.o2r        ← generated from YOUR ROM on first boot
│   └── mods/
└── MM/
    ├── 2ship.exe
    ├── mm.o2r         ← generated from YOUR ROM on first boot
    └── mods/
```

Each host looks for its sibling at `../MM` / `../OOT` relative to its own
executable. For different layouts use the environment variables
`SHIPLUA_MM_ROOT` (in OOT) and `SHIPLUA_OOT_ROOT` (in MM).

On boot the log confirms the mount:

```text
ShipLua exposed 50496 MM assets under 'mm/' (18585 direct-aliased, ...)
ShipLua mounted the MM mm.o2r in cross-world mode: ...\MM\mm.o2r
```

If the sibling does not exist the game runs normally — only cross-world
assets are unavailable.

## The `mm/` and `oot/` namespaces

Every asset of the neighbouring archive is addressable with a namespace
prefix:

| You are in | Prefix | Example |
|---|---|---|
| OOT | `mm/` | `mm/objects/gameplay_keep/gElegyShellHumanDL` |
| MM | `oot/` | `oot/objects/object_rl/object_rl_Skel_007B38` |

The prefix avoids collisions: both games share thousands of identical path
names (`objects/gameplay_keep/...` exists in both, with different contents).
Roughly 2,500 of MM's ~50,000 paths collide with OOT paths — almost all of
them audio.

## Hash aliasing (why it exists)

Composite resources reference their children by the **hash of the original
path**, baked into the binary: a display list points at its own textures by
the hash of `objects/.../fooTex`, unprefixed. Those hashes cannot be
rewritten — so the host creates an **alias at the original hash** for every
`objects/` and `textures/` entry of the neighbour that does **not** collide
with a local path. That way internal references (DL→texture, skeleton→DL)
resolve on their own.

Entries outside `objects/`/`textures/` deliberately get no alias: systems
such as the audio loader **enumerate** the global index, and foreign entries
in that enumeration crash the host (the audio formats differ). They remain
reachable through the namespace, which enumerators never scan.

## Display-list dialect sanitizer

The display lists serialized by the two ports are not 100% identical in
dialect:

- `G_DL_INDEX` (0x3D) jumps into a setup-DL table by 2ship's internal
  convention — non-existent in the other game's context. It is neutralized
  (turned into a no-op); the DL's own inline material commands remain and the
  host applies its local setup before drawing.
- The shader/interpolation opcodes from 0x43 up have divergent numbering
  between the two LUS builds and are remapped or neutralized at load time.

This happens transparently whenever a DL is loaded through the cross-world
archive — mods do not need to do anything.

## Walkthrough 1 — the Elegy statue in OOT

The OOT host registers the logical actor `oot.mm_elegy_statue`: a lightweight
actor whose draw renders `mm/objects/gameplay_keep/gElegyShellHumanDL` — the
human form of the Elegy of Emptiness statue, read live from `mm.o2r`. The
whole mod:

```lua
local ship = require("ship")
local statue = nil

ship.hotkeys.register("elegy_statue", { default = "K", label = "Elegy Statue (MM)" }, function()
    if statue and ship.actor.exists(statue) then
        ship.actor.destroy(statue)
        statue = nil
        return
    end
    local actor, err = ship.actor.spawn("oot.mm_elegy_statue", {
        position = { x = 0, y = 0, z = 0 }, -- (0,0,0) = in front of the player
        rotation = { x = 0, y = 0, z = 0 },
    })
    if not actor then
        ship.log.warn("failed [" .. err.code .. "]: " .. err.message)
        return
    end
    statue = actor
end)
```

## Walkthrough 2 — Rauru in MM

The reverse, with an upgrade: Rauru is a **skeletal** NPC. The MM host
registers `mm.oot_rauru`, which initializes a SkelAnime with OOT's skeleton
(`oot/objects/object_rl/object_rl_Skel_007B38`) and idle animation
(`object_rl_Anim_000A3C`) — hash aliasing makes the limb display lists and
textures resolve on their own. The Lua mod is identical to the statue one
with the key swapped to `"mm.oot_rauru"`.

## Honest limitations

- **Skeletal assets** depend on resource-format compatibility between the two
  ports (currently compatible; may diverge in future versions).
- **MM setup-DLs** become no-ops in OOT — subtle material differences
  (blend/fog modes) may appear compared to the original look.
- The neighbour's **audio, scenes and sequences** get no alias and are not
  executable on the other engine — only render data truly crosses.
- Hosts do **not** load the neighbouring game: they only read its archive.

---

*(Versão em português: [cross-world-assets.md](cross-world-assets.md))*
