> English translation of [coordination/INTEGRATION.md](INTEGRATION.md). The Portuguese document remains the canonical project record.

# INTEGRATION.md — Integration log (governance)

> Maintained only by the governance agent. Records what was integrated into `main`,
> with build/test validation. See `AGENTS.md` §17 for the operating model.

## Status of `main`

- Integrated tip: `af85476` (core + EXAMPLE-001; CI Linux+Windows green).
- Build/test: **24/24 green** (clean rebuild; includes hello-world compliance).
- **CI Linux + Windows + package-examples: green** (criterion v0.1.0 §16 closed).
- Lua v5.4.7 + toml++ v3.4.0 + miniz 3.1.2 (FetchContent).

### Phase 4 (bootstrap of hosts in forks) — coordination integrated in `main`

| Task | Status | Where is the code |
|---|---|---|
| OOT-001 / MM-001 (submodule + CMake) | review | forks (PRs #5 / #1) |
| OOT-002 / MM-002 (bootstrap) | review | forks |
| OOT-003 / MM-003 (shutdown) | review | forks |
| OOT-004 / MM-004 (identity) | review | forks |
| OOT-005 / MM-005 (find mods dir) | review | forks |

## PRs opened in forks (Phase 4) — pending owner decision

| PR | Repo | What does it do | Status |
|---|---|---|---|
| #5 | BaiterYamato/Shipwright-HyliaFoundry | `build(oot)`: submodule `extern/ship-lua` + CMake, base `lua/main` | validated by the builder (MSVC Release x64); **governance NOT merged** — not verifiable in this environment (full game build) |
| #1 | BaiterYamato/2ship2harkinian | `build(mm)`: submodule `extern/ship-lua` + CMake, base `lua/main` | ditto |

- Submodule pin in forks: `ad5aad6` (BIND-001 code) — already contains all the
  core (runtime, manifest, events, timers, codegen, binding). Suggestion: before
  of the merge in the forks, pin bump to the tip of `main` to include the docs of
  governance (optional — does not affect the build).

## Criteria v0.1.0 (§16) — governance overview

| Criterion | Status |
|---|---|
| Shipwright and 2Ship compile with ShipLua | partial — submodule in forks validated in MSVC (PRs #5/#1), awaiting merge from owner |
| hello-world / scene-logger / player run on both | partial — hello-world proven by conformity in the core; pending scene-logger/player |
| manifest and dependencies work | ✅ |
| runtime isolates mods | ✅ |
| storage blocks traversal | partial — root loader validates entrypoint; VFS/shares (STORE-*) outstanding |
| hot reload does not duplicate hooks | pending (RELOAD-*) |
| documentation is generated (codegen) | ✅ |
| API with SemVer | ✅ |
| CI Linux and Windows green | ✅ (core-linux + core-windows + package-examples) |
| no protected ROM/asset | ✅ (.gitignore + scan; core without assets) |
| release with checksums | pending (no release yet) |
| documented known limitations | partial (INTEGRATION.md + handoffs) |

**Summary:** the core Lua modloader API is completed and proven
(vertical slice §14, 24 tests, green Linux+Windows CI). The rest of v0.1.0 is
in-game (adapters in forks) and later phases (STORE/RELOAD/more examples/
release), depending on the builder and the owner's decision about PRs #5/#1.

## History

### 2026-07-13 (9) — Green Windows CI (§16 closed)

- `core-windows` fixed (VS generator did not find VS → `ilammy/msvc-dev-cmd`
  + Ninja): run **success**. Linux + Windows now green.

### 2026-07-13 (8) — CI Windows/MSVC (builder idle)

- Added `.github/workflows/core-windows.yml` (governance): windows-latest,
  generator VS 2022 x64, `--config Release`, `ctest --timeout 120`. No actions
  third parties. De-risk: CMake does not use GCC-specific flags and the core has already compiled
  under MSVC 19.44 in the integration of forks (handoff OOT-001).
- Objective: close criterion §16 "CI Windows green". Result of the 1st run is
  checked in the next cycle.
- No builder to integrate in this cycle (idle; `main` already up to date).

### 2026-07-13 (7) — EXAMPLE-001 hello-world + vertical slice (§14) proven in core

- Fast-forward from `main` (`1adebb2` → `e6dcf6c`): EXAMPLE-001.
  - `examples/hello-world/` (manifest.toml + main.lua identical to §14 of PLAN).
  - `tests/conformance/HelloWorldConformanceTests.cpp` + Python packaging:
    packs `hello-world.shipmod`, loads via `LoadModsFromRoot`, fires
    `game.ready`, check log "Hello from <game> host=<version>" and unload without
    leaky callback — **for oot and mm**.
  - `.github/workflows/package-examples.yml` — packaging workflow
    examples added by the builder (new file, does not collide with `core-linux.yml`;
    CI of examples is now co-maintained — see note below).
- Validation: **clean rebuild, 24/24 green**. Milestone: The Vertical API Agreement
  slice (§14) is proven end-to-end in the core (actual in-game part depends
  of the hosts' adapters in the forks).
- Ownership note: `.github/**` was governance territory (§17); the builder
  contributed `package-examples.yml`. Accepted (no file collision); IC of
  core (`core-linux.yml`) remains with governance.

### 2026-07-13 (6) — HOST-004 + MOD-008 root loader (code)

- Merge `--no-ff` from `agent/MOD-008-root-loader` (`ee0004f` → `265a200`), bringing:
  - HOST-004 (OOT-004/MM-004 identity, fork-side coordination);
  - **MOD-008** (code): `ModHost::LoadModsFromRoot` — discover mods from one
    root directory, validates host/API/game compatibility, resolves order of
    dependencies and isolates failures per mod (returns `ModLoadReport` with
    `loadedIds`/`rejected`); `LoadModFromPackage` (.shipmod); `DispatchEvent`
    connecting the EventDispatcher to the host.
- Validation: **clean rebuild**, 21/21 green (new `mod_root_loader_tests`:
  deterministic order, mismatch rejection, fault isolation,
  ownership/package cache). No game headers.
- Operational note: a global `taskkill cmake.exe` (to unlock reconfigure)
  may have hit a builder cmake — avoid kill by name; use PID.

### 2026-07-13 (5) — Green Linux CI + HOST-002

- CI `core-linux` (run 29228265961): **success**. `main` checked on Ubuntu.
- Merge `--no-ff` from `agent/HOST-002-bootstrap` (`1a1856a` → `18c9397`):
  OOT-002/MM-002 coordination (host bootstrap, under review on forks). No code.
- PR ship-lua#15 closed (merged) by push.
- HOST-003 (shutdown OOT-003/MM-003) still in claim — not integrated.

### 2026-07-13 (4) — Core Linux CI

- Added `.github/workflows/core-linux.yml` (governance/Build-CI).
  Mirrors verified build/test: Ubuntu + CMake/Ninja + FetchContent
  (Lua 5.4.7, toml++ 3.4.0, miniz 3.1.2) + ctest, `--timeout 120`.
  Triggers on push/PR for `main` and for `workflow_dispatch`.
- Python tests: stdlib only; CMake uses `find_package(Python3)` + `Python3_EXECUTABLE`
  (portable, without `python` hardcoded). No `pip install` required.
- Windows/macOS CI are a next step after Linux goes green.
- Nothing from the builder's in-progress branch (HOST-002, claims OOT-002/MM-002) was
  integrated: still no handoff/code.

### 2026-07-13 (3) — HOST-001 coordination (fork bootstrap)

- Merge `--no-ff` from `agent/HOST-001-submodules` into `main` (`8a702c1` → `c92d0e7`).
  Actual content: coordination only (STATUS + claims/handoffs from OOT-001 and MM-001).
  No ship-lua code → build unchanged. Merges clean.
- PR ship-lua#14 (branch HOST-001) closed as merged by push.
- Fork PRs #5 (Shipwright) and #1 (2ship) opened by the builder — see table above.
  Not integrated by governance (not verifiable here).

### 2026-07-13 (2) — Codegen + Lua binding

- Merge `--no-ff` from `agent/BIND-001-lua-api` into `main` (`a62a0de` → `64a76fa`),
  bringing the chain CODEGEN-001 (C++ bindings), CODEGEN-002/003 (LuaDoc + Markdown)
  and BIND-001 (binding the Lua runtime to the `ship.*` services).
- Clean merge — no file overlap with governance docs
  (the exclusivity of §17 worked).
- Core checked: no game headers on `src/**` or `include/**`.
- Isolated build/test: 20/20 green.
- Marked in PLAN §13: CODEGEN-001/002/003.

### 2026-07-13 — Phase 2/3 Integration + timers

- Fast-forward from `main` `abbd687` → `82a6d55`: integrated with linear stack of PRs #1–#7
  (MOD-001/CORE-002, MOD-003 discovery+extraction, MOD-004 SemVer, MOD-005
  dependencies, TOOL-001 validator, ARCH-001 hook inventory, ARCH-002 RFC 0001,
  API-001/002/003 schemas).
  - PR#1 marked MERGED automatically; PR#2–#7 closed with note (empty diff against
    `main` after fast-forward — commits already present). Integrated branches removed.
- Fast-forward from `main` `82a6d55` → `a16df3b`: EVENT-001 (deterministic dispatcher)
  and TIMER-001 (frame scheduler). No associated PR; integrated directly after validation.
- Isolated build/test: 14/14 green.

## Robustness notes (CI backlog)

- MinGW test executables depend on `libstdc++-6.dll`/`libgcc_s_seh-1.dll`/
  `libwinpthread-1.dll`. Without them in `PATH`, the test **hangs in a Windows dialog**
  instead of failing. Recommendation for CI/GOV: static link
  (`-static -static-libgcc -static-libstdc++`) on test targets, or ensure the
  runtime at `PATH`/`--timeout` in ctest. Applied `--timeout 60` as a safeguard.

## Integration pending

- CODEGEN-001 (C++ bindings) in progress by builder — integrate when completed.
- CI-001..004 (game builds) and Phase 4 (submodule `extern/ship-lua` in forks)
  they depend on forks and are for later.
