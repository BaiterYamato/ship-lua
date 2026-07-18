# MODSDK-001

- Status: review
- Agent: kimi-subagent-modsdk-001
- Platform: Windows 11 / Kimi Work / CMake + Ninja (MinGW g++)
- Repository: BaiterYamato/link-span (ship-lua)
- Branch: agent/MODSDK-001-capability-registry
- Started: 2026-07-13T00:00:00-03:00
- Depends on: BIND-001, API-003
- Files:
  - rfcs/0008-capability-registry.md
  - include/shiplua/capability/**
  - src/capability/**
  - include/shiplua/api/LuaApiBinding.h
  - src/api/LuaApiBinding.cpp
  - tests/unit/CapabilityRegistryTests.cpp
  - tests/unit/LuaApiBindingTests.cpp
  - tests/CMakeLists.txt
  - coordination/claims/MODSDK-001.md
  - coordination/handoffs/MODSDK-001.md
  - coordination/STATUS.md
- Goal:
  - Registry consultável de capabilities com descritores (id, version, provider,
    games, stability, permissions, limits) conforme plan-sdk.md §3.5/§7/§8.19/§15.
  - Suporte a múltiplos providers por capability com seleção determinística.
  - API Lua `ship.capabilities.has(id, range)`, `list(filter)`, `info(id)` e
    `providers(id)` com feature detection por range SemVer.
  - Compatibilidade retroativa com o contexto de host legado (lista plana de
    capabilities sintetiza descritores a partir de `Generated::kCapabilities`).
