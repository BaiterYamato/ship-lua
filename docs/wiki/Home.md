# Link-Span Wiki

**A shared Lua modloader for Ocarina of Time (Shipwright) and Majora's Mask
(2Ship2Harkinian)** — write a mod once, run it in both games, and reach
across worlds: MM assets inside OoT and OoT assets inside MM, live.

## Features

- **One Lua API for both games** (`ship.*`), isolated sandbox per mod,
  failure isolation (one broken mod never takes the others down).
- **Rebindable hotkeys** (`ship.hotkeys`).
- **Generic actor API** with safe handles — spawn by curated name
  (`oot.en_dog`) or by raw id (`mm.id.0x021`), no host recompilation.
- **Cross-world assets** — OoT mounts `../MM/mm.o2r` as `mm/…`, MM mounts
  `../OOT/oot.o2r` as `oot/…`. Demos: MM's Elegy statue in OoT, OoT's Rauru
  in MM.
- **Launcher** (`link-span.exe`) with game chooser and in-game world switch.
- **Dev CLI + mock runtime** — validate and test mods without a ROM.

## Pages

- [Getting Started](Getting-Started) — install, ROM setup, folder layout.
- [Writing Your First Mod](Writing-Your-First-Mod) — hello-world to hotkeys.
- [Cross-World Assets](Cross-World-Assets) — the `mm/` / `oot/` namespaces.
- [Example Mods](Example-Mods) — the 8 shipped examples and their keys.
- [Troubleshooting](Troubleshooting) — rejections, keys, crashes, logs.

Full docs live in the repository:
[docs/](https://github.com/BaiterYamato/link-span/tree/main/docs) ·
[API reference](https://github.com/BaiterYamato/link-span/blob/main/generated/docs/api-reference.en.md)

---

**Resumo (pt-BR):** Link-Span é um modloader Lua compartilhado para OoT e MM
com API única, hotkeys rebindáveis, API genérica de atores (por nome ou id) e
assets cross-world (assets do MM dentro do OOT e vice-versa). Comece por
[Getting Started](Getting-Started); as docs completas ficam na pasta
`docs/` do repositório (em português e inglês).
