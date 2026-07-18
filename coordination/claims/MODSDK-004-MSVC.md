# MODSDK-004-MSVC

- Status: review
- Agent: Codex
- Platform: Windows 11 / MSVC 2022 / CMake + Ninja
- Repository: BaiterYamato/link-span
- Branch: agent/MODSDK-004-msvc-crash-fix
- Started: 2026-07-18
- Depends on: MODSDK-004 / PR 39
- Files:
  - docs/agents/Plans.md
  - coordination/claims/MODSDK-004-MSVC.md
  - include/shiplua/api/LuaApiBinding.h
  - src/api/LuaApiBinding.cpp
  - tests/unit/LuaApiBindingTests.cpp
  - tests/contract/ApiContractTests.cpp
- Goal:
  - Reproduzir e eliminar o crash MSVC `0xc0000409` dos testes de contrato.
  - Manter erros Lua protegidos sem `longjmp` atravessando objetos C++ vivos.
  - Validar a correção em Release/MSVC e na suíte CTest completa antes do merge.
