> English translation of [docs/api/frame-timers.md](frame-timers.md). The Portuguese document remains the canonical project record.

# Timers per frame

`FrameTimerScheduler` implements the internal basis of `ship.timer.after` and
`ship.timer.every`. He doesn't know Lua or game structures; each host
advances the scheduler once when publishing its common event `game.frame`.

## Semantics

- `after(mod, frames, callback)` executes once;
- `every(mod, frames, callback)` runs again in the same period;
- `frames` must be greater than zero;
- `frames = 1` means the next tick;
- the recurring period starts from the previous maturity, avoiding drift;
- cancellation by handle and cancellation of all timers in a mod are
  idempotent in the lifecycle, but an already inactive handle returns
  `InvalidHandle` when canceled again.

## Order

Timers expiring on the same tick are ordered by:

1. due date frame;
2. mod load order;
3. mod priority;
4. Mod ID;
5. Increasing timer ID.

Smaller values ​​run first. A timer created during a tick never enters
in the list already collected for that tick.

## Faults and thread

Exceptions do not traverse the scheduler. The crash logs handle, mod ID and
message; a failed one-time timer is removed and a recurring timer is disabled
after three consecutive failures by default. A successful execution resets the
consecutive counter.

The scheduler belongs to the thread in which it was created. `Tick`, scheduling and
cancellation in another thread return `InvalidState`; re-entrant ticks too
are refused.
