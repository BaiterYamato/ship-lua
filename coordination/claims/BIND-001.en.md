> English translation of [coordination/claims/BIND-001.md](BIND-001.md). The Portuguese document remains the canonical project record.

#BIND-001

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / MinGW g++ 15.2 + Ninja
- Branch: agent/BIND-001-lua-api
- Started: 2026-07-13T01:46:42-03:00
- Depends on: CODEGEN-001/002/003 (PRs #10 and #12; stacked branch)
- Files:
  - include/shiplua/api/LuaApiBinding.h
  - src/api/LuaApiBinding.cpp
  - include/shiplua/host/ModHost.h
  - src/host/ModHost.cpp
  - tests/unit/LuaApiBindingTests.cpp
  - tests/CMakeLists.txt
  - docs/api/lua-binding.md
  - coordination/claims/BIND-001.md
  - coordination/handoffs/BIND-001.md
  - coordination/STATUS.md
- Goal:
  - Install `require("ship")` without opening `package`, filesystem or native loading.
  - Expose game/versions, capabilities, events and logging of schema 0.1.0.
  - Execute Lua callbacks protected by the dispatcher and isolate them by mod.
  - Remove Lua inscriptions and references on unload.
  - Try the first vertical slice in memory without ROM or specific host.
- Completed: 2026-07-13T01:57:28-03:00
