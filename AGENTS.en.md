> English translation of [AGENTS.md](AGENTS.md). The Portuguese document remains the canonical project record.

# AGENTS.md — ShipLua

> Operating agreement for human and AI agents working on Lua modloader shared between Shipwright and 2Ship2Harkinian.
>
> Provisional name of the project: **ShipLua**.
> Default owner used in this document: **BaiterYamato**.
> Plan base date: **2026-07-12**.

## 1. Purpose of this file

This file defines how any agent should work on the project, regardless of the platform used: Codex, Claude Code, Gemini CLI, Copilot, local agents, CI automations or human collaborators.

Please read this file in its entirety before changing code. In case of conflict:

1. security and integrity of the repository;
2. this `AGENTS.md`;
3. `PLAN.md`;
4. directory-specific documentation;
5. instruction of the assigned task.

No agent should change the public API architecture on their own.

---

## 2. Project objective

Create a modular Lua modloader to:

- `HarbourMasters/Shipwright` — Ocarina of Time;
- `HarbourMasters/2ship2harkinian` — Majora's Mask.

The project must offer:

- a shared Lua runtime;
- a common and versioned API;
- specific adapters for each game;
- manifest, dependencies, permissions and storage per mod;
- installation of mods without recompiling the game;
- error isolation;
- documentation and generated types;
- continuous compatibility with future updates of both ports.

### Central principle

A mod that uses only the common API should run, without changes, in both games.

Unique resources must be exposed by capabilities and namespaces:

```lua
if ship.capabilities.has("mm.cycle") then
    ship.mm.cycle.on_reset(function(event)
        -- Majora's Mask
    end)
end
```

Don't artificially simulate a feature that doesn't exist in the other game.

---

## 3. Repositories and branches

### Repositories

```text
BaiterYamato/ship-lua
BaiterYamato/Shipwright
BaiterYamato/2ship2harkinian
```

The `ship-lua` repository is the canonical source for:

- shared runtime;
- API schema;
- manifest;
- generators;
- documentation;
- conformity tests;
- coordination files.

The two forks only contain:

- CMake integration;
- runtime bootstrap;
- game adapter;
- necessary hooks;
- host specific tests.

### Conceptually protected branches

| Branch | Function |
|---|---|
| `develop` | `upstream/develop` mirror; does not receive project work |
| `lua/main` | stable modloader integration in each fork |
| `agent/<TASK-ID>-<slug>` | isolated work of a task |
| `release/<version>` | version stabilization |
| `hotfix/<TASK-ID>-<slug>` | urgent fix after release |

In the `ship-lua` repository, use `main` as stable integration and `agent/...` branches for tasks.

### Git Rules

- Commits are allowed when the user explicitly authorizes them.
- Even with authorization for commits, do not commit directly to `develop`,
  `lua/main` or `main`, unless the user also explicitly authorizes the
  named integration branch. By default, use a `agent/<TASK-ID>-<slug>` branch.
- Never use `git push --force` in shared branch.
- Do not rebase a branch after another agent depends on it.
- Prefer small, reviewable PR with a single purpose.
- Do not mix unrelated refactoring with functional implementation.
- Do not update submodules without recording the reason.
- Do not merge PR with CI failing.
- Do not include ROM, `.z64`, `.n64`, `.v64`, `oot.o2r`, `mm.o2r`, dumps, personal saves or protected assets.

---

## 4. Recommended workspace

```text
ship-lua-workspace/
├── control/ # BaiterYamato clone/ship-lua
├── shipwright/ # clone of BaiterYamato/Shipwright fork
├── two-ship/ # fork clone BaiterYamato/2ship2harkinian
└── worktrees/
    ├── CORE-001/
    ├── OOT-001/
    └── MM-001/
```

When multiple agents work on the same machine, each must use a separate `git worktree`.

Example:

```bash
git -C control fetch origin
git -C control worktree add \
  ../worktrees/CORE-001 \
  -b agent/CORE-001-runtime-bootstrap \
  origin/main
```

Do not allow two agents to share the same working directory.

---

## 5. Mandatory coordination protocol

Before editing any file, the agent must:

1. read `AGENTS.md`;
2. read `PLAN.md`;
3. read `coordination/STATUS.md`;
4. check tasks and dependencies;
5. create or update a claim in `coordination/claims/`;
6. declare the paths you want to change.

### Claim file

Path:

```text
coordination/claims/<TASK-ID>.md
```

Format:

```markdown
#CORE-001

- Status: claimed
- Agent: codex-windows-01
- Platform: Windows 11 / Codex
- Branch: agent/CORE-001-runtime-bootstrap
- Started: 2026-07-12T22:00:00-03:00
- Depends on: none
- Files:
  - src/runtime/**
  - include/shiplua/runtime/**
  - tests/runtime/**
- Goal:
  - Initialize and terminate an isolated Lua state.
```

### Allowed states

```text
unclaimed
claimed
blocked
review
done
canceled
```

### File exclusivity

An agent must not edit a path claimed by another agent, except when:

- the first agent formally updated the claim;
- the coordinator authorized;
- work takes place in different, non-conflicting files.

Emergency change outside of scope must be recorded in the claim before committing.

---

## 6. Mandatory handoff format

When stopping or completing a task, create:

```text
coordination/handoffs/<TASK-ID>.md
```

Model:

```markdown
# Handoff — CORE-001

## State

review

## Result

- Runtime initializes.
- Allowed libraries have been registered.
- Dangerous libraries remain unavailable.

## Commits

- `abc1234` — Add isolated Lua runtime
- `def5678` — Add runtime lifecycle tests

## Changed files

- `src/runtime/LuaRuntime.cpp`
- `include/shiplua/runtime/LuaRuntime.h`
- `tests/runtime/LuaRuntimeTests.cpp`

## Validation performed

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Validation result

All tests passed on Linux x86_64.

## Outstanding items

- Windows still needs to be tested.
- The memory limit is configurable, but has no stress test yet.

## Risks

- The allocation callback does not yet produce per-mod metrics.

## Recommended next action

Run `CORE-002` after the merge.
```

A handoff cannot just say “done”. It must allow another agent to continue without reconstructing the entire context.

---

## 7. Mandatory architectural division

```text
Lua mod
   ↓
Versioned public API
   ↓
runtime and shared services
   ↓
IGameAdapter
   ├── ShipwrightAdapter
   └── TwoShipAdapter
       ↓
GameInteractor + host
```

### Shared core

The core cannot include OoT or MM headers.

Prohibited in Core:

```cpp
#include <z64.h>
#include "variables.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
```

The core should only depend on its own interfaces and stable types.

### Adapters

Adapters can access internal structures, but must convert them to:

- snapshots;
- handles with generation;
- public enums;
- validated results;
- common events.

### Public API

The public API must not expose:

- pointers;
- addresses;
- struct layouts;
- C++ references with uncontrolled lifetime;
- internal enums without translation;
- arbitrary functions of ABI C;
- FFI.

---

## 8. Compatibility rules

### Separate versions

Always differentiate:

```text
host_version
runtime_version
api_version
mod_version
```

Host update should not automatically change the API version.

### SemVer

The public API uses SemVer:

- `MAJOR`: intentional break in compatibility;
- `MINOR`: new feature supported;
- `PATCH`: correction without contractual change.

### Capabilities

Every non-universal function or event must have detectable capacity:

```lua
if ship.capabilities.has("player.health.write") then
    ship.player.set_health(48)
end
```

### Depreciation

A public API:

1. is marked as deprecated;
2. remains functional for at least one `MINOR` cycle;
3. gets documented replacement;
4. It is only removed in version `MAJOR`.

---

## 9. Security

### Lua Libraries

By default, do not expose:

- `io`;
- `os.execute`;
- `package.loadlib`;
- `debug`;
- FFI;
- DLL/OS loading;
- unrestricted access to the file system;
- network;
- subprocesses.

### Filesystem

Each mod receives its own virtual storage.

Minimum requirements:

- `..` blocking;
- blocking of absolute paths;
- normalization before validating;
- blocking of symlinks;
- file limit;
- byte limit;
- atomic writing;
- namespace by mod ID.

### Execution

Each callback must have:

- protected error handling;
- mod identification;
- stack trace;
- configurable limit;
- failure counter;
- safe deactivation after repeated failures.

Never allow C++ exception to cross the C/Lua border.

---

## 10. Code standards

### C and C++

- Follow the formatting and conventions of the host repository.
- The shared core uses C++20, unless otherwise decided in the RFC.
- Use RAII for runtime resources.
- Avoid global singletons when a dependency can be injected.
- Do not use exceptions at the public border.
- Return `Result<T>` or structured error.
- Avoid allocation per frame on frequent paths.
- Do not log per frame in normal build.
- Every handle must validate generation and host.

### Moon

- Main API in namespace `ship`.
- Don't create implicit globals.
- Use LuaDoc in the examples.
- Example scripts should work on both hosts unless explicitly stated.
- Do not distribute `.luac` bytecode as canonical format.
- A mod must have `manifest.toml` and explicit entrypoint.

### Names

```text
C++ classes: PascalCase
C++ methods: PascalCase or host default
public Lua API: snake_case
events: dotted.snake_case
capabilities: dotted.snake_case
task IDs: PREFIX-NNN
```

---

## 11. Mandatory tests

Each functional PR must include at least one of the following:

- unit test;
- integration testing;
- conformity mod;
- regression testing;
- manifest fixture;
- validation of generated documentation.

### Minimum matrix

| Layer | Linux | Windows | macOS |
|---|---:|---:|---:|
| core `ship-lua` | mandatory | mandatory | initially recommended |
| Shipwright | mandatory | mandatory | recommended |
| 2Ship | mandatory | mandatory | recommended |
| path safety | mandatory | mandatory | mandatory |
| common compliance mod | mandatory | mandatory | mandatory before release |

Local tests that require legitimate assets cannot be run on the public CI with ROM included. Use synthetic fixtures and tests without protected content.

---

## 12. Build and validation

### Shipwright — Linux

```bash
git submodule update --init --recursive
cmake -S . -B build-cmake -GNinja -DSUPPRESS_WARNINGS=0
cmake --build build-cmake --target GenerateSohOtr
cmake --build build-cmake
```

### 2Ship — Linux

```bash
git submodule update --init --recursive
cmake -S . -B build-cmake -GNinja
cmake --build build-cmake --target Generate2ShipOtr
cmake --build build-cmake
```

###Windows

Use Visual Studio 2022, toolset v143, CMake and Python 3. Full commands remain at:

```text
Shipwright/docs/BUILDING.md
2ship2harkinian/docs/BUILDING.md
```

The agent must record in the handoff:

- operating system;
- compiler;
- settings;
- command;
- result;
- tests not performed.

---

## 13. Commit and PR policy

### Commit

Format:

```text
<type>(<scope>): <summary>
```

Examples:

```text
feat(runtime): add isolated Lua state
feat(oot): bridge scene enter hook
fix(storage): reject normalized parent traversal
test(api): add common player health conformance mod
docs(manifest): document optional capabilities
```

Types:

```text
feat fix refactor test docs build ci chore perf security
```

###PR

Every PR must inform:

- task;
- problem;
- solution;
- central files;
- tests;
- platforms;
- risks;
- compatibility;
- screenshots or logs when applicable;
- security checklist.

Adapter PR should not reset common contract without corresponding PR in core.

---

## 14. Criteria for changing the public API

It is mandatory to create an RFC in:

```text
rfcs/NNNN-title.md
```

The RFC must contain:

- motivation;
- Lua contract;
- capabilities;
- behavior in OoT;
- behavior in MM;
- errors;
- compatibility;
- security impact;
- test plan;
- depreciation plan.

Do not implement a new public API before registering its semantics.

---

## 15. Prohibitions for agents

Don't:

- direct access to pointer from Lua;
- LuaJIT FFI;
- API automatically generated from all game headers;
- unrestricted exposure of `gSaveContext`;
- asynchronous execution of game logic outside the main thread;
- silent change of manifest or version;
- automatic download of mods without verification;
- automatic update executable without consent;
- telemetry;
- use of ROM in versioned tests;
- joint change of the two adapters without testing both;
- “quick fix” that duplicates the core in both forks.

---

## 16. Global definition of ready

A task is only ready when:

- compiled code;
- relevant tests performed;
- updated documentation;
- no protected files;
- no duplicate path in the other fork;
- updated claim;
- handoff created;
- Open PR;
- declared known risks;
- integration agent review completed.

Completion of the project requires the same common mod package to be loaded by both hosts without code changes.

---

## 17. Parallel operating model: builder + governance

In this cycle the work is divided between two roles that operate at the same time:

| Paper | Who | Responsibility |
|---|---|---|
| Builder | code agent (codex) | implements tasks in `agent/<TASK-ID>-<slug>` branches, writes tests, updates `coordination/STATUS.md`, claims and handoffs |
| Governance/Integrator | governance agent (Claude) | integrates completed branches into `main`, generates PRs, validates build/tests in isolation, maintains `PLAN.md`, `AGENTS.md` and the integration log |

### Single directory rule

The builder is the owner of the working tree in `D:/Desenvolvimento/2ShipHarkinian-MoonLoader`.
Governance **never** edits files or checks out this directory; operates in a
separate `git worktree` (`../2ShipHarkinian-MoonLoader-gov`). This fulfills §4.

### Exclusivity of files between roles

To avoid merge conflicts, each role only writes to its files:

- **Builder writes:** `src/**`, `include/**`, `tests/**`, `schema/**`, `tools/**`,
  `generated/**`, `docs/**`, `rfcs/**`, `coordination/claims/**`,
  `coordination/handoffs/**`, `coordination/STATUS.md`.
- **Governance writes:** `PLAN.md`, `AGENTS.md`, `coordination/INTEGRATION.md`,
  `.github/**` (CI workflows), and integrates into `main` (fast-forward
  or conflict-free merge of the builder's validated tops, with documentation commits
  and CI on top).

If one role needs to touch another's file, register it in your claim first.
or in the integration log.

### Governance integration protocol

In each cycle, governance:

1. reads the state (`git log`, branches, `STATUS.md`) without changing the builder's working tree;
2. select completed branches `agent/*` (code + handoff, builder has already moved on);
3. validate `cmake --build` + `ctest` in the isolated worktree (with the MinGW runtime in `PATH`);
4. advance `main` by fast-forward to the validated top and push;
5. update `PLAN.md`/`AGENTS.md` and register it in `coordination/INTEGRATION.md`;
6. resolves corresponding PRs (merge/close with note) and cleans up merged branches.

Never integrate a branch with red build or test.
