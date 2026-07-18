# MODSDK-001-MSVC

- Status: review
- Agent: Codex
- Platform: Windows 11 / MSVC 2022 / CMake + Ninja
- Repository: BaiterYamato/link-span
- Branch: agent/MODSDK-001-msvc-crash-fix
- Started: 2026-07-18
- Depends on: MODSDK-001 / PR 36 e PR 39 integrado
- Files:
  - docs/agents/Plans.md
  - coordination/claims/MODSDK-001-MSVC.md
  - include/shiplua/api/LuaApiBinding.h
  - src/api/LuaApiBinding.cpp
  - tests/unit/LuaApiBindingTests.cpp
- Goal:
  - Resolver conflitos do capability registry com o runtime de timers/storage.
  - Eliminar os crashes MSVC dos caminhos de erro da API de capabilities.
  - Validar a pilha combinada antes do merge do PR 36.

- Validation:
  - Release/MSVC build: passed.
  - CTest: 46/46 passed.
  - AddressSanitizer contract probe: passed on OoT and MM.
