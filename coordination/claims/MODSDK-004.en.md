# MODSDK-004

- Status: review
- Agent: kimi-subagent-modsdk-004
- Platform: Windows 11 / Kimi Work / CMake + MSVC 2022
- Repository: BaiterYamato/ship-lua (link-span)
- Branch: agent/MODSDK-004-mock-runtime
- Started: 2026-07-18T02:51:39-03:00
- Depends on: none (plan-sdk.md §16 'Mock runtime', §22 'Tests', §29)
- Files:
  - coordination/claims/MODSDK-004.md
  - coordination/claims/MODSDK-004.en.md
  - include/shiplua/api/LuaApiBinding.h
  - include/shiplua/storage/KeyValueStorage.h
  - include/shiplua/mock/MockRuntime.h
  - include/shiplua/testing/ModTestRunner.h
  - src/api/LuaApiBinding.cpp
  - src/storage/KeyValueStorage.cpp
  - src/mock/MockRuntime.cpp
  - src/testing/ModTestRunner.cpp
  - tools/mod_test_runner.cpp
  - schema/api.yml
  - schema/capabilities.yml
  - generated/include/shiplua/generated/ApiBindings.h
  - generated/lua/shiplua.lua
  - generated/docs/api-reference.md
  - generated/docs/api-reference.en.md
  - examples/frame-counter/**
  - tests/CMakeLists.txt
  - tests/unit/KeyValueStorageTests.cpp
  - tests/unit/MockRuntimeTests.cpp
  - tests/unit/GeneratedApiBindingsTests.cpp
  - tests/unit/ModTestRunnerTests.cpp
  - tests/fixtures/mod-runner/**
- Goal:
  - Test providers (mock) for the core capabilities: events, timers, log,
    storage and capabilities, allowing Lua mods to run without game/ROM.
  - Lua API `ship.timer.after/every/cancel` and `ship.storage.get/set/delete/clear`
    (plan-sdk.md §8.1/§8.18) injected via providers in the host context.
  - Capabilities `core.events`, `core.timers`, `core.storage`, `core.input` in
    the registry (plan-sdk.md §7 Core).
  - C++ `MockRuntime`: emits lifecycle events, advances frames/timers,
    simulates input (hotkeys), captures logs and exposes inspectable storage.
  - Mod test runner: `ModTestRunner` library + `shiplua_mod_test_runner` CLI
    that load a mod directory or `.shipmod` into the mock and execute the mod's
    tests (`tests/*.lua` with the `describe`/`it`/`assert`/`mock` DSL).
  - `frame-counter` example with tests running on the mock; unit tests and
    ctest targets following repo conventions.
