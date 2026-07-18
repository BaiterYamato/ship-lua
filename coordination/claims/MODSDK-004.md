# MODSDK-004

- Status: review
- Agent: kimi-subagent-modsdk-004
- Platform: Windows 11 / Kimi Work / CMake + MSVC 2022
- Repository: BaiterYamato/ship-lua (link-span)
- Branch: agent/MODSDK-004-mock-runtime
- Started: 2026-07-18T02:51:39-03:00
- Depends on: none (plan-sdk.md §16 'Mock runtime', §22 'Testes', §29)
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
  - Providers de teste (mock) para as capabilities centrais: eventos, timers, log,
    storage e capabilities, permitindo rodar mods Lua sem jogo/ROM.
  - API Lua `ship.timer.after/every/cancel` e `ship.storage.get/set/delete/clear`
    (plan-sdk.md §8.1/§8.18) injetadas por providers no host context.
  - Capabilities `core.events`, `core.timers`, `core.storage`, `core.input` no
    registry (plan-sdk.md §7 Core).
  - `MockRuntime` C++: emite eventos de lifecycle, avança frames/timers,
    simula input (hotkeys), captura logs e expõe storage inspecionável.
  - Mod test runner: biblioteca `ModTestRunner` + CLI `shiplua_mod_test_runner`
    que carregam um diretório de mod ou `.shipmod` no mock e executam os testes
    do mod (`tests/*.lua` com DSL `describe`/`it`/`assert`/`mock`).
  - Exemplo `frame-counter` com testes rodando no mock; testes unitários e alvos
    ctest seguindo as convenções do repo.
