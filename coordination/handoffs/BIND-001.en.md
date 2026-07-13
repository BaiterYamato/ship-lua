> English translation of [coordination/handoffs/BIND-001.md](BIND-001.md). The Portuguese document remains the canonical project record.

# Handoff — BIND-001

## State

review

## Result

- Internal `require("ship")` works without opening `package` or external modules.
- Game, versions, capabilities, events and logging follow schema 0.1.0.
- The same `hello-world` runs in memory with OoT and MM contexts.
- Recursive payloads are converted without pointers or game layouts.
- Lua callbacks use protected call, traceback and deactivation after three failures.
- `off`, rollback and unload release registry registrations and references.
- Module installation occurs within `lua_pcall`, including under low memory.
- `UnloadAll` copies IDs before deleting the map, eliminating dangling references.

## Commits

- `ad5aad6` — `feat(api): bind Lua runtime services`

## Changed files

- `include/shiplua/api/LuaApiBinding.h`
- `src/api/LuaApiBinding.cpp`
- `include/shiplua/host/ModHost.h`
- `src/host/ModHost.cpp`
- `tests/unit/LuaApiBindingTests.cpp`
- `tests/CMakeLists.txt`
- `docs/api/lua-binding.md`
- `coordination/claims/BIND-001.md`
- `coordination/handoffs/BIND-001.md`
- `coordination/STATUS.md`

## Validation performed

```powershell
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk cmake -S . -B build-warnings -G Ninja `
  "-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
rtk cmake --build build-warnings --target lua_api_binding_tests manifest_host_tests
rtk ctest --test-dir build-warnings `
  -R "lua_api_binding_tests|manifest_host_tests" --output-on-failure
rtk git diff --check
```

## Validation result

- Windows 11, MinGW g++ 15.2, Ninja, C++20.
- Full normal build completed.
- 20/20 CTest tests passed.
- Build with expanded warnings compiled the binding without new warnings.
- 2/2 focal tests also passed the build with warnings.
- No protected assets or game headers/globals were introduced.

## Tests not executed

- Linux and macOS.
- Real Shipwright and 2Ship; the current slice uses synthetic ROM-free contexts.
- Limit of instructions per callback; remains the responsibility of the runtime.

## Pending

- Connect `LuaApiHostContext` to real adapters.
- Publish `game.ready`, `game.frame` and `game.shutdown` via hosts bootstrap.
- Integrate `FrameTimerScheduler` into the frame event.
- Package the `hello-world.shipmod` example for conformance on both ports.

## Risks

- The current binding treats contracted events as observations; Lua semantics
  filter/transform/consume still require a specific contract in the schema.
- The build with warnings also pointed out pre-existing warnings in
  `PackageExtractor.cpp` and miniz headers, outside this claim.
- Empty host context keeps API/version/log/events available, but
  `ship.game.*` fails until bootstrap provides identity.

## Recommended next action

After integrating the codegen and binding PRs, start `OOT-001` and `MM-001` to
add ShipLua as submodule and connect minimal bootstrap on both hosts.
