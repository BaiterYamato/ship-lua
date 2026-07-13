> English translation of [coordination/handoffs/CORE-001.md](CORE-001.md). The Portuguese document remains the canonical project record.

# Handoff — CORE-001

## State

review

## Result

- `ship-lua` repository started locally (git, branch `main`, no remote yet).
- Lua 5.4 runtime isolated per mod (`LuaRuntime`): one `lua_State` per instance,
  own allocator with configurable memory limit, RAII + move semantics.
- Initial sandbox (partial CORE-003): opens only base/coroutine/table/string/
  math/utf8 + `os` sanitized. `io`, `debug`, `package` never opened;
  `dofile`, `loadfile` and `os.execute/exit/getenv/remove/rename/tmpname/setlocale`
  removed.
- Structured logging by mod (CORE-004) via `Logger`/`LogSink`.
- Protected errors (CORE-005): `DoString` uses `lua_pcall` + `luaL_traceback`;
  no C++ exception or Lua error crosses the boundary; runtime remains usable after error.
- Memory limit per state (CORE-006) implemented in the allocator and tested.
- Lifecycle init/close (basic CORE-007) via constructor/destructor.

## Commits

- `b301723` — chore(repo): initialize ShipLua workspace scaffolding
- `3d56f20` — feat(runtime): add isolated Lua 5.4 runtime with sandbox (CORE-001)

## Changed files

- `CMakeLists.txt`
- `include/shiplua/runtime/Result.h`
- `include/shiplua/runtime/Logger.h`
- `include/shiplua/runtime/LuaRuntime.h`
- `src/runtime/LuaRuntime.cpp`
- `tests/CMakeLists.txt`
- `tests/unit/RuntimeTests.cpp`
- `.gitignore`

## Validation performed

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
```

## Validation result

Windows 11 / MinGW g++ 15.2 / Ninja / Lua v5.4.7 (FetchContent).
`100% tests passed, 0 tests failed out of 1` (20 internal checks pass:
isolation, dangerous libs missing, safe libs present, error protected +
reuse, lifecycle, memory limit).

## Pending

- No remote/push (waiting for authorized GitHub bootstrap GOV-001/GOV-002).
- Lacks build/test on Linux and macOS (AGENTS matrix §11).
- Sandbox does not yet expose `require("ship")` or custom loader (API phase).
- CORE-002 (one status per mod managed by a Registry/ModHost) not yet done —
  Today each `LuaRuntime` is already isolated, but the multi-mod manager is missing.

## Risks

- Lua's `FetchContent` requires networking in CMake configuration (offline build fails).
- Allocator does not expose usage metrics per mod to the outside (only internal limit).

## Recommended next action

Commit bootstrap GitHub (GOV-001/GOV-002) OR follow local with CORE-002
(multi-mod manager) and MOD-001/MOD-002 (schema + TOML manifest parser).
