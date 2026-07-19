# RFC 0009 — Minimal generic actor API

- Status: accepted for implementation
- Target API: 0.4.0
- Task: MODSDK-005
- Dependencies: MODSDK-001, MODSDK-002, MODSDK-003, MODSDK-004, OOT-MODSDK-001

## Motivation

The host-specific dog helpers prove the native bridge, but do not provide a
reusable SDK. Mods need stable logical actor names without native IDs,
pointers, engine structures, or game-specific units.

This first slice covers only spawn, destroy, and liveness. Transform updates,
configuration, animation, behavior, and native parameters require later RFCs.

## Lua contract

```lua
local actor, err = ship.actor.spawn("oot.en_dog", {
    position = { x = 10, y = 20, z = 30 },
    rotation = { x = 0, y = 90, z = 0 }, -- degrees; optional
})

local alive, exists_err = ship.actor.exists(actor)
local destroyed, destroy_err = ship.actor.destroy(actor)
```

All three functions return two values: `value, nil` on success or
`nil, { code = "...", message = "..." }` for expected failures.

`spawn` requires an allowlisted logical type and a finite absolute position.
Rotation is optional, expressed in degrees, and defaults to zero. `destroy`
only accepts an actor owned by the caller. `exists` returns `false, nil` for an
owned stale handle and an error for malformed or foreign handles.

Lua sees only the opaque session-scoped value:

```lua
{ kind = "actor", slot = 12, generation = 4, scene_generation = 9 }
```

## Native provider

Core defines an abstract `ActorProvider`, injected through
`LuaApiHostContext::actors`, with `Spawn`, `Destroy`, `Exists`, and `ReleaseMod`.
The request contains a logical actor name, finite absolute position, and
rotation in degrees. The provider runs on the game thread and owns allowlist,
object dependencies, scene checks, native ownership, and cleanup. Core never
includes OoT/MM headers.

## Capabilities, permissions, and limits

- `actor.spawn` requires `world.entities.create`;
- `actor.destroy` requires `world.entities.destroy`;
- `actor.exists` requires `world.entities.read`.

Availability and authorization are separate and both are checked per call.
Manifests request generic grants in `permissions.grants` and may set
`limits.actors`. The default is 16, zero disables spawn, and the parser maximum
is 256. A manifest may only reduce the provider limit.

## Lifecycle and errors

Handles validate kind, slot, generation, scene, and owner. Unload invokes
`ReleaseMod` before the Lua state closes; host scene transitions invalidate
native records; failed spawns roll back partial reservations.

Expected failures use structured `invalid_argument`, `unsupported`,
`permission_denied`, `invalid_handle`, `invalid_state`, `resource_limit`, and
`host_failure` errors. They do not invoke `lua_error`, so MSVC never longjmps
over live C++ objects.

## Compatibility and tests

The additive contract raises API 0.3.0 to 0.4.0 (MINOR). Existing host-specific
helpers remain available. A ROM-free `MockActorProvider`, schema/codegen tests,
permission/limit tests, ownership/stale-handle tests, unload cleanup, rollback,
and an `actor-spawn` example provide the acceptance proof.
