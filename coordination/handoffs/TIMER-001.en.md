> English translation of [coordination/handoffs/TIMER-001.md](TIMER-001.md). The Portuguese document remains the canonical project record.

# Handoff — TIMER-001

## State

review

## Result

- C++20 Scheduler independent of Lua and hosts.
- Single and recurring timers measured in frames.
- Deterministic order by expiration, load, priority, mod and registration.
- Cancellation by handle and by owner, including during callback.
- Timers created during a tick only appear in future ticks.
- Disabled isolated and recurring exceptions after consecutive failures.
- Proprietary thread and re-entrant tick rejection.

## Commits

- `b2ce9df` — `feat(timer): add frame scheduler`

## Changed files

- `include/shiplua/timer/FrameTimerScheduler.h`
- `src/timer/FrameTimerScheduler.cpp`
- `tests/unit/FrameTimerSchedulerTests.cpp`
- `tests/CMakeLists.txt`
- `docs/api/frame-timers.md`
- `coordination/claims/TIMER-001.md`
- `coordination/handoffs/TIMER-001.md`
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
- 14/14 CTest tests passed.
- No protected assets, pending markers, or dependencies on hosts found.

## Tests not executed

- Linux and macOS.
- Integration with the hosts' `game.frame` event.
- Actual Lua callback and runtime instruction limits.
- Single compilation with warnings via proxy `rtk g++`; the wrapper did not translate
  Windows paths, while the CMake target compiled normally.

## Pending

- Binding of `ship.timer.after` and `ship.timer.every`.
- Advance the scheduler from the common event `game.frame` in bootstrap.
- Cancel all timers on mod unload for the host lifecycle.

## Risks

- The maximum of three consecutive failures is configurable in C++, but not yet
  has public configuration.
- Lower priorities execute first, as in the dispatcher.

## Recommended next action

Start `CODEGEN-001` to generate C++ contracts from the canonical schemas,
without manually duplicating the public API in future bindings.
