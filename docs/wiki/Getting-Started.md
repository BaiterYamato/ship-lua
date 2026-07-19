# Getting Started

## 1. Download

Grab the latest `Link-Span-Windows-x64.zip` from the
[Releases page](https://github.com/BaiterYamato/link-span/releases) and
extract it into a new folder. The package contains **no ROM** and no
ROM-derived archives — you provide your own, legally obtained game.

## 2. Folder layout

```text
Link-Span/
├── link-span.exe        ← the launcher
├── soh.o2r, 2ship.o2r   ← redistributable port assets (included)
├── mods/                ← shared mods (.shipmod)
└── hosts/
    ├── oot/
    │   ├── soh.exe
    │   └── mods/        ← OoT-only mods
    └── mm/
        ├── 2ship.exe
        └── mods/        ← MM-only mods
```

The sibling `hosts/oot` and `hosts/mm` folders are what makes **cross-world
assets** work: each game finds the neighbour next to itself (any sibling
folder named `oot`/`OOT` and `mm`/`MM` works — Windows paths are
case-insensitive).

## 3. Provide your game

Place your legally obtained ROM (`.z64`/`.n64`/`.v64`) — or an already
extracted `oot.o2r` / `mm.o2r` — next to `link-span.exe` or next to the game
executable (inside `hosts/oot` or `hosts/mm`). On first boot the game's extraction wizard detects the ROM and
generates the game archive. Supported OoT versions include NTSC-U 1.0–1.2 and
GC releases; 2Ship supports MM USA.

## 4. Run

Run `link-span.exe`:

- one available game starts automatically;
- both available → an OoT/MM chooser appears;
- a dual-game mod (`requires_both_games = true`) warns when one half is
  missing.

You can also start `OOT/soh.exe` or `MM/2ship.exe` directly.

## 5. Install mods

Drop `.shipmod` files (or unpacked mod folders) into the game's `mods/`
folder. On boot, the log (`logs/*.log`) shows what loaded:

```text
ShipLua carregou 5 mod(s) de './mods'
```

Try the shipped examples first — see [Example Mods](Example-Mods).

---

**Resumo (pt-BR):** baixe o ZIP da release, extraia, coloque sua ROM legal
dentro de `OOT/` ou `MM/` (a extração do primeiro boot gera o archive do
jogo), rode `link-span.exe` e ponha mods `.shipmod` na pasta `mods/` de cada
jogo. As pastas irmãs `OOT/` e `MM/` habilitam os assets cross-world.
