> English translation of [docs/api/event-dispatcher.md](event-dispatcher.md). The Portuguese document remains the canonical project record.

# Event dispatcher

`EventDispatcher` is the internal contract, independent of Lua and hosts, that
orders and isolates callbacks before public bindings are connected.

## Deterministic order

Registration is carried out by:

1. load order already resolved by the mod graph;
2. mod priority;
3. callback priority;
4. Mod ID;
5. Increasing record ID.

Smaller values ​​run first. The loading order incorporates
dependencies and the constraints `load.after` and `load.before`.

## Event types

- `observe`: observes a copy of the payload; changes are discarded;
- `filter`: returns `Block` to stop propagation without changing the payload;
- `transform`: changes the shared payload and continues propagation;
- `consume`: returns `Consume` to end propagation without changing the payload.

A flow incompatible with the event type is recorded as a callback failure,
but it doesn't bring down the dispatcher.

## Security during dispatch

- exceptions are converted to `CallbackFailure` with signature and mod ID;
- following callbacks continue after a failure;
- a removal takes effect immediately for callbacks not yet called;
- a subscription created during dispatch only participates in the next dispatch;
- events cannot be defined during a dispatch;
- all mutable operations must occur in the thread that created the dispatcher.

The payload accepts `nil`, boolean, 64-bit integer, number, text, array and
recursive object. It does not contain pointers, host references, or file layouts.
internal structures of games.
