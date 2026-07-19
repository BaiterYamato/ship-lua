> English translation of [coordination/STATUS.md](STATUS.md). The Portuguese document remains the canonical project record.

# STATUS.md

## Project

ShipLua

## Global status

stable core on `main`; generic `ship.actor` API 0.4.0 completed and validated on a review branch, pending OoT provider integration

## Repositories

| Repository | Status | Integration branch | Baseline |
|---|---|---|---|
| BaiterYamato/ship-lua | created + push (origin) | main | — |
| BaiterYamato/Shipwright-HyliaFoundry (HarbourMasters/Shipwright fork) | OOT-001 to OOT-005 in review (PRs #5 to #9) | lua/main | `fb3259f7` |
| BaiterYamato/2ship2harkinian (fork of HarbourMasters/2ship2harkinian) | MM-001 to MM-005 in review (PRs #1 to #5) | lua/main | `b3cc3662` |

> Note: Shipwright fork has been renamed to `Shipwright-HyliaFoundry`.
> Both forks have default `develop` and integration branch `lua/main` already created.

## Recommended active task

Check and integrate, in order, the batteries OOT-001 → OOT-005 and MM-001 → MM-005;
in parallel, review the EXAMPLE-001 common package.

## Progress

- `GOV-001` — done (forks already existed; `lua/main` present in both).
- `GOV-002` — done (BaiterYamato/ship-lua created and pushed).
- `CORE-001` — review (isolated Lua 5.4 runtime + sandbox; green ctest on Windows/MinGW).
- `MOD-001` — review (C++ struct + JSON Schema from manifest).
- `MOD-002` — review (TOML parser with structured errors).
- `CORE-002` — review (isolated multi-mod ModHost, rollback and unload).
- `TEST-002` — review (valid and invalid TOML fixtures).
- `MOD-003` — review (discovery + limited/transactional ZIP extraction + ModHost upload).
- `MOD-004` — review (SemVer 2.0.0, conjunctive ranges and validation in the manifest).
- `MOD-005` — review (graph and dependency validation).
- `MOD-006` — review (deterministic order by graph, priority and ID).
- `MOD-007` — review (cycles, incompatibilities and duplicate IDs).
- `TOOL-001` — review (CLI validator for manifest, directory and `.shipmod`).
- `ARCH-001` — review (inventory of hooks in current commits from both hosts).
- `ARCH-002` — review (RFC 0001 of the core/adapters architecture).
- `API-001` — review (`schema/api.yml` with types, functions and errors).
- `API-002` — review (`schema/events.yml` with payloads, support and phases).
- `API-003` — review (`schema/capabilities.yml` with status and hosts).
- `EVENT-001` — review (host/Lua independent dispatcher and proprietary thread).
- `EVENT-002` — review (order by load, priorities, mod and registry).
- `EVENT-003` — review (`observe`, `filter`, `transform`, `consume` and fault isolation).
- `TIMER-001` — review (`after`/`every` per frame, cancellation and fault isolation).
- `CODEGEN-001` — review (types and C++ metadata generated from schemas with drift gate).
- `CODEGEN-002` — review (LuaDoc generated with types, signatures and event names).
- `CODEGEN-003` — review (Markdown reference in pt-BR generated from schemas).
- `BIND-001` — review (`require("ship")`, game/versions, capabilities, events and log).
- `OOT-001` — review (submodule/CMake; `soh.exe` generated on Windows/MSVC; PR #5).
- `MM-001` — review (submodule/CMake; `2ship.exe` generated on Windows/MSVC; PR #1).
- `OOT-002` — review (idempotent bootstrap + logger + static CRT; `soh.exe` generated; PR #6).
- `MM-002` — review (idempotent bootstrap + logger + static CRT; `2ship.exe` generated; PR #2).
- `OOT-003` — review (idempotent shutdown before host services; `soh.exe` generated; PR #7).
- `MM-003` — review (idempotent shutdown before host services; `2ship.exe` generated; PR #3).
- `OOT-004` — review (`oot`, version 9.1.2 and real commit injected/detected; PR #8).
- `MM-004` — review (`mm`, version 4.0.2 and real commit injected/detected; PR #4).
- `MOD-008` — done (root discovery, compatibility, dependencies, isolation and cache of `.shipmod`; integrated into `main`).
- `OOT-005` — review (writable folder `mods`, common load and `game.ready`; `soh.exe` generated; PR #9).
- `MM-005` — review (writable folder `mods`, common load and `game.ready`; `2ship.exe` generated; PR #5).
- `EXAMPLE-001` — review (reproducible common package, OoT/MM compliance and CI artifact; PR #20).
- `MODSDK-005` — review (RFC 0009, `ActorProvider`, generic spawn/destroy/exists,
  manifest permissions/limits, ROM-free mock and OoT/MM examples; MSVC Release
  56/56 and schema/codegen 30/30 green).

## Locks

None registered.

## Latest integration

ship-lua `main` at `fff5785`; MODSDK-005 is based on that published baseline
and published for review as link-span PR #41.

## Next action

Integrate MODSDK-005 into the OoT provider: update `extern/ship-lua`, implement
the public provider interface, inject `LuaApiHostContext::actors`, and rebuild
`soh.exe`. A legitimate-assets smoke and the MM provider remain outstanding.
