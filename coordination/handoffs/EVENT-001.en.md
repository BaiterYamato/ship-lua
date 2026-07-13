> English translation of [coordination/handoffs/EVENT-001.md](EVENT-001.md). The Portuguese document remains the canonical project record.

# Handoff — EVENT-001 / EVENT-002 / EVENT-003

## State

review

## Result

- C++20 Dispatcher independent of Lua and hosts.
- Recursive payload without pointers or internal game layouts.
- Deterministic order by load, mod priority, callback priority,
  Mod ID and Registration ID.
- Registrations during dispatch are deferred; removals take effect in
  current iteration without invalidating it.
- `observe`, `filter`, `transform` and `consume` modes implemented.
- Exceptions and incompatible flows are isolated and identified by mod.
- Mutable operations are restricted to the owning thread.

## Commits

- `8f11491` — `feat(events): add deterministic dispatcher`

## Changed files

- `include/shiplua/events/EventDispatcher.h`
- `src/events/EventDispatcher.cpp`
- `tests/unit/EventDispatcherTests.cpp`
- `tests/CMakeLists.txt`
- `docs/api/event-dispatcher.md`
- `coordination/claims/EVENT-001.md`
- `coordination/handoffs/EVENT-001.md`
- `coordination/STATUS.md`

## Validation performed

```powershell
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk git diff --check
```

## Validation result

- Windows 11, MinGW g++ 15.2, Ninja, C++20.
- Full build completed.
- 13/13 CTest tests passed.
- No protected assets or host framework dependencies were found.

## Tests not executed

- Linux and macOS.
- Integration into Shipwright and 2Ship, scheduled for Phase 4.
- Real Lua callback, which depends on the hosts' bindings and bootstrap.

## Pending

- Convert protected Lua callbacks to contract `EventCallback`.
- Connect resolved order from `DependencyResolver` to registration options.
- Implement `TIMER-001` on events per frame.

## Risks

- The configurable execution limit per callback continues to belong to the
  Lua runtime and is not yet connected to the dispatcher.
- Lower priorities run first; bindings and generated documentation must
  preserve this convention.

## Recommended next action

Implement `TIMER-001` on the dispatcher before starting bindings or
host adapters.
