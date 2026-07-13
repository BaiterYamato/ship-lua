> English translation of [docs/api/lua-binding.md](lua-binding.md). The Portuguese document remains the canonical project record.

# Minimal Lua Binding

`LuaApiBinding` installs the internal module `ship` in each `LuaRuntime`. The binding
is created before entrypoint and destroyed before `lua_State`.

## Secure import

The sandbox is still missing the `package` library. A restricted `require` accepts
only `require("ship")` and returns the same table by module identity.
There is no searching for disk, `package.loadlib`, DLL/OS or external modules.

## Available surface

- `ship.game.id()` and `ship.game.host_version()` use host-injected context;
- `ship.runtime.version()` and `ship.api.version()` maintain separate versions;
- `ship.capabilities.has/list` use an ordered list without duplicates;
- `ship.events.on/off` connect callbacks to `EventDispatcher`;
- `ship.log.debug/info/warn/error` preserve the mod's proprietary ID.

If bootstrap has not yet provided game or host version, only these functions
fail in a protected manner; the rest of the module remains available.
Capabilities that are unknown, yet planned, or incompatible with the game are
refused during binding installation.

## Events and lifecycle

Payloads are recursively converted to Lua values: nil, boolean, integer,
number, text, array and object. No pointers or internal layout are exposed.

Callbacks use `lua_pcall` with traceback. A fault is returned to the dispatcher and
does not interrupt other mods. After three consecutive failures, registration is
removed. `ship.events.off` and unload release both registration and
reference in the Lua registry.

`ModHost` defines the generated schema events, installs the binding and offers
`DispatchEvent`. The same script `hello-world` is exercised in memory with
OoT and MM contexts, without ROM.
