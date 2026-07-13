> English translation of [coordination/claims/EVENT-001.md](EVENT-001.md). The Portuguese document remains the canonical project record.

# EVENT-001 / EVENT-002 / EVENT-003

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / MinGW g++ 15.2 + Ninja
- Branch: agent/EVENT-001-dispatcher
- Started: 2026-07-13T00:00:00-03:00
- Depends on: API-002 (PR #7; stacked branch)
- Files:
  - include/shiplua/events/EventDispatcher.h
  - src/events/EventDispatcher.cpp
  - tests/unit/EventDispatcherTests.cpp
  - tests/CMakeLists.txt
  - docs/api/event-dispatcher.md
  - coordination/claims/EVENT-001.md
  - coordination/handoffs/EVENT-001.md
  - coordination/STATUS.md
- Goal:
  - Dispatch events in deterministic order without depending on the host or Lua.
  - Support unsubscribe and changes during dispatch without invalidating iteration.
  - Implement `observe`, `filter`, `transform` and `consume`.
  - Isolate callback exception and preserve healthy callbacks/mods.
  - Restrict the dispatcher to the owning thread.
- Completed: 2026-07-13T01:27:06-03:00
