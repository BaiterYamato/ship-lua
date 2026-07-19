# Cross-World Assets

Since SDK 0.4 each host mounts the neighbouring game's archive at runtime:

| You are in | It mounts | Assets appear as |
|---|---|---|
| OoT | `../MM/mm.o2r` | `mm/objects/...`, `mm/textures/...` |
| MM | `../OOT/oot.o2r` | `oot/objects/...`, `oot/textures/...` |

No asset is copied — the host reads the neighbour's archive live. The boot
log confirms it:

```text
ShipLua expôs 50496 assets do MM sob 'mm/' (…)
ShipLua montou o mm.o2r do MM em modo cross-world: …\MM\mm.o2r
```

## What you can do with it

- **Spawn the Elegy of Emptiness statue in OoT** — the shipped
  `elegy-statue` mod (**K** in OoT) draws
  `mm/objects/gameplay_keep/gElegyShellHumanDL` straight from `mm.o2r`.
- **Summon Rauru in MM** — the shipped `rauru` mod (**K** in MM) builds a
  SkelAnime from `oot/objects/object_rl/…` (skeleton + idle animation).

From Lua, cross-world actors are just catalog keys:

```lua
ship.actor.spawn("oot.mm_elegy_statue", { position = { x = 0, y = 0, z = 0 } })
ship.actor.spawn("mm.oot_rauru",        { position = { x = 0, y = 0, z = 0 } })
```

## How it works (short version)

- Everything is addressable under the `mm/` / `oot/` **namespace**;
- `objects/` and `textures/` entries that don't collide with local paths
  also get an **alias at their original hash**, so internal references
  (display list → textures, skeleton → limb DLs) resolve automatically;
- audio and other enumerated data get **no alias** on purpose (foreign
  formats would crash the host's scanners) — namespace-only;
- a **dialect sanitizer** translates the two ports' display-list encodings
  at load time.

Override the sibling location with `SHIPLUA_MM_ROOT` (in OoT) /
`SHIPLUA_OOT_ROOT` (in MM).

Deep dive: [docs/cross-world-assets.en.md](https://github.com/BaiterYamato/link-span/blob/main/docs/cross-world-assets.en.md)

---

**Resumo (pt-BR):** cada jogo monta o archive do vizinho em runtime
(`mm/…` no OOT, `oot/…` no MM), com alias de hash para dados de render e
sanitizador de dialeto de display lists. Demos: estátua da Elegia no OOT e
Rauru no MM, ambos no K. Detalhes em `docs/cross-world-assets.md`.
