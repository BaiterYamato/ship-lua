# Handoff — MODSDK-005

## Status

review

## Result

- RFC 0009 raises the public API to `0.4.0`.
- `ship.actor.spawn`, `destroy`, and `exists` use an injected `ActorProvider`;
  core contains no OoT/MM headers, native ids, or pointers.
- Lua handles match `HandleRegistry`: kind, slot, generation, and scene
  generation, with ownership and ABA/scene protection.
- Expected failures return `nil, { code, message }`; IDL/codegen declares
  `error_mode = return`, avoiding `lua_error` on these MSVC paths.
- Manifests support generic permission grants and actor limits (default 16,
  accepted range 0–256), checked independently from capabilities.
- The ROM-free `MockActorProvider`, unload cleanup, ownership tests, and the
  `examples/actor-spawn` mod run on both mock hosts.

## Validation

- Visual Studio 2022 / MSVC 19.44 Release: 56/56 CTest tests passed.
- Schema/codegen Python suite: 30/30 passed.
- OoT and MM actor-spawn example tests passed without a ROM.
- Generated artifacts and `git diff --check` are clean.

## Next integration

Update the OoT host submodule, make `OotActorProvider` implement the public
interface, inject it through `LuaApiHostContext::actors`, rebuild `soh.exe`, and
run the adapter contract test. A real-assets smoke remains required later.

