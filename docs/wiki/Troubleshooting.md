# Troubleshooting

## My mod doesn't load

Check the game log (`logs/*.log` next to the executable). Every rejection is
logged with a reason:

- **`api range ">=0.1 <0.2" does not include ShipLua API 0.4.0`** — the mod
  was packaged for an old SDK. Update `manifest.toml` to `api = ">=0.4 <0.5"`
  (or a range covering 0.4) and repack.
- **`capability … missing`** — the mod requires a capability this host does
  not offer. Use `optional` + `ship.capabilities.has(...)` when a feature is
  nice-to-have.
- **Nothing about the mod at all** — the `.shipmod` must contain
  `manifest.toml` and `main.lua` at the ZIP **root** (not in a subfolder),
  and must sit in the game's `mods/` folder.

## A key does nothing

- Two mods may share the same default key (both callbacks fire) — rebind in
  the ShipLua settings.
- `spawn failed [invalid_state]: required object is not loaded` — the
  current scene doesn't have that actor's model bank. Try the scene the mod
  suggests (e.g. dogs: OoT Market / MM Clock Town).
- `unsupported` — unknown actor key: use a catalog name or the
  `oot.id.<n>` / `mm.id.<n>` form.

## `ship.storage` raises "unsupported"

`core.storage` (and `core.timers`) are currently implemented in the **mock
runtime only**, not in the real hosts. Feature-detect:

```lua
local hasStorage = ship.capabilities.has("core.storage")
```

## Cross-world assets unavailable

The boot log says where it looked:

```text
ShipLua: mm.o2r não encontrado em '…' — assets do MM indisponíveis no OOT
```

Keep the sibling `OOT/` and `MM/` folders next to each other (the launcher
layout), or point `SHIPLUA_MM_ROOT` / `SHIPLUA_OOT_ROOT` at the right place.
Both games must have their archives generated (first boot with your ROM).

## The game crashed

Crash logs are appended to `logs/*.log` with a stack trace (`CrashHandler`).
When reporting a bug, attach:

1. the tail of the log (including the `Traceback:` block);
2. your `mods/` folder listing;
3. game versions (first `Starting …` line of the log).

Open issues at <https://github.com/BaiterYamato/link-span/issues>.

---

**Resumo (pt-BR):** o log em `logs/*.log` explica toda rejeição de mod
(faixa de api antiga, capability ausente, zip com estrutura errada), falha de
spawn (objeto da cena ausente, chave desconhecida) e traz o stack trace de
crashes. `ship.storage`/`ship.timer` só existem no mock — use
`ship.capabilities.has`. Cross-world exige as pastas irmãs `OOT/` e `MM/`.
