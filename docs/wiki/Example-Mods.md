# Example Mods

All examples ship in the release `mods/` folders and live in
[`examples/`](https://github.com/BaiterYamato/link-span/tree/main/examples).

| Mod | Key | Game(s) | What it shows |
|---|---|---|---|
| `hello-world` | — | both | Minimal mod: logs the host identity on `game.ready`. |
| `frame-counter` | **C** | both | Events, timers and storage with feature detection (falls back to memory-only when `core.storage` is absent). |
| `dog-spawner` | **F** | both | Game-specific capabilities (`oot.spawn_dog` / `mm.spawn_dog`). Needs a scene with the dog object (OoT Market / Clock Town). |
| `jump` | **J** | both | Player-action capabilities (`oot.player.jump` / `mm.player.jump`). |
| `actor-spawn` | **K** | both | The generic actor API with safe handles. |
| `kafei-puppet` | **H** | OoT | Young-Link skeleton puppet; drop a `.otr` model replacer (e.g. Kafei) into `mods/` and the puppet wears it. |
| `elegy-statue` | **K** | OoT | **Cross-world**: MM's Elegy of Emptiness statue read live from `mm.o2r`. |
| `rauru` | **K** | MM | **Cross-world**: OoT's Rauru (skeleton + animation) read live from `oot.o2r`. |

Notes:

- All keys are **rebindable** in the ShipLua settings; two mods can share a
  default key (both fire) — rebind to taste.
- Actors spawn **in front of the player** when position `{0,0,0}` is passed.
- Spawns fail with a clean log line when the current scene lacks the actor's
  object — try the suggested scenes.

---

**Resumo (pt-BR):** tabela dos 8 mods de exemplo com suas teclas. Teclas são
rebindáveis; spawns aparecem na frente do player; se a cena não tem o objeto
do ator, o log explica.
