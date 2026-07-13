# Integrando o ShipLua num jogo (host)

> Para **autores de mods**, veja [writing-mods.md](writing-mods.md). Este documento é
> para quem quer **adicionar o ShipLua ao código-fonte** de um Shipwright (OoT) ou
> 2Ship (MM) e compilar o jogo com o modloader embutido.

O ShipLua **não** é um plugin que se solta num executável pronto: o modloader é uma
biblioteca C++ que precisa ser **compilada dentro do jogo** (ela usa ganchos do motor
para eventos, input e atores). O fluxo é: você pega o **source** do jogo, adiciona o
ShipLua e **compila você mesmo**. São ~5 passos e poucos arquivos.

> Referência funcional completa: o fork
> [`BaiterYamato/2ship2harkinian`](https://github.com/BaiterYamato/2ship2harkinian)
> (branch `agent/MM-005-mod-directory`) — integração real do MM.

## 1. Adicionar o ShipLua como submódulo

No source do jogo:

```bash
git submodule add https://github.com/BaiterYamato/ship-lua.git extern/ship-lua
git submodule update --init --recursive
```

O caminho `extern/ship-lua` é o mesmo nos dois jogos.

## 2. Ligar no CMake

No `CMakeLists.txt` do jogo:

```cmake
option(ENABLE_SHIP_LUA "Enable ShipLua modloader" ON)

if(ENABLE_SHIP_LUA)
    add_subdirectory(extern/ship-lua)
    target_link_libraries(<alvo-do-jogo> PRIVATE shiplua)
endif()
```

Troque `<alvo-do-jogo>` pelo alvo real do host (no 2Ship é `2ship`; no Shipwright,
`soh`). O ShipLua baixa Lua 5.4, toml++ e miniz via FetchContent.

## 3. Escrever o adaptador (bootstrap)

Crie um arquivo no jogo, ex.: `<jogo>/ShipLuaBootstrap.cpp` (+ `.h`). Ele:

1. cria um `ShipLua::ModHost` com a identidade do host;
2. descobre e carrega os mods de uma pasta `mods/`;
3. dispara o evento `game.ready`;
4. (opcional) instala bindings específicos do jogo (`ship.mm.*` / `ship.oot.*`).

Esqueleto mínimo:

```cpp
#include <shiplua/host/ModHost.h>
#include <shiplua/generated/ApiBindings.h>
#include <ship/Context.h>          // API do LibUltraship do host
#include <spdlog/spdlog.h>
#include <filesystem>
#include <memory>

namespace ShipLuaHost {
namespace {
std::unique_ptr<ShipLua::ModHost> gModHost;

ShipLua::Logger CreateLogger() {
    return ShipLua::Logger([](ShipLua::LogLevel lvl, const std::string& mod, const std::string& msg) {
        switch (lvl) {
            case ShipLua::LogLevel::Debug: SPDLOG_DEBUG("ShipLua [{}]: {}", mod, msg); break;
            case ShipLua::LogLevel::Info:  SPDLOG_INFO ("ShipLua [{}]: {}", mod, msg); break;
            case ShipLua::LogLevel::Warn:  SPDLOG_WARN ("ShipLua [{}]: {}", mod, msg); break;
            case ShipLua::LogLevel::Error: SPDLOG_ERROR("ShipLua [{}]: {}", mod, msg); break;
        }
    });
}
} // namespace

void Initialize() {
    if (gModHost) return;

    ShipLua::LuaApiHostContext ctx;
    ctx.gameId      = "mm";            // "oot" no Shipwright
    ctx.hostVersion = "4.0.2";         // versão real do host

    gModHost = std::make_unique<ShipLua::ModHost>(ctx, CreateLogger());

    auto shipCtx = Ship::Context::GetInstance();
    auto modsRoot = Ship::Context::GetPathRelativeToAppDirectory("mods", shipCtx->GetShortName());
    std::filesystem::create_directories(modsRoot);

    auto loaded = gModHost->LoadModsFromRoot(modsRoot, modsRoot / ".shiplua-cache");
    if (loaded.isOk()) {
        SPDLOG_INFO("ShipLua carregou {} mod(s)", loaded.value->loadedIds.size());
    }

    ShipLua::EventPayload ready{
        {"game_id", ctx.gameId}, {"host_version", ctx.hostVersion},
        {"runtime_version", ctx.runtimeVersion},
        {"api_version", std::string(ShipLua::Generated::kApiVersion)},
    };
    gModHost->DispatchEvent("game.ready", ready);
}

void Shutdown() { gModHost.reset(); }

} // namespace ShipLuaHost
```

Adicione o `.cpp` às fontes do alvo do jogo (glob ou lista explícita).

## 4. Chamar `Initialize()` / `Shutdown()`

No ponto de inicialização/encerramento do port (no 2Ship é o `BenPort.cpp`; no
Shipwright, o equivalente):

```cpp
#include "ShipLuaBootstrap.h"
// ... durante a inicialização do jogo:
ShipLuaHost::Initialize();
// ... no encerramento:
ShipLuaHost::Shutdown();
```

## 5. Compilar e usar

Compile o jogo normalmente (com sua ROM para gerar os assets — o ShipLua **não** lida
com ROMs). Depois:

```text
<pasta-do-exe>/mods/<seu-mod>/    (manifest.toml + main.lua)  ou  <seu-mod>.shipmod
```

Abra o jogo: os mods carregam e `game.ready` dispara. Veja os logs em
`logs/<jogo>.log`.

---

## Bindings específicos do jogo (`ship.mm.*` / `ship.oot.*`)

O núcleo expõe só a API comum. Recursos exclusivos do jogo o **adaptador** instala
diretamente no `lua_State` de cada mod (o ShipLua expõe os headers do Lua aos
consumidores privilegiados justamente para isso). Padrão:

```cpp
extern "C" { #include "lua.h" }
// ... internals do jogo (Actor_Spawn, gPlayState, etc.)

static int LuaSpawnDog(lua_State* L) {
    // ... chama Actor_Spawn(...) usando os internals do MM ...
    lua_pushboolean(L, /*ok*/ 1);
    return 1;
}

void InstallMmApi(lua_State* L) {          // chame para cada runtime carregado
    lua_getglobal(L, "require"); lua_pushstring(L, "ship");
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) { lua_pop(L, 1); return; }
    lua_newtable(L);                       // ship.mm
    lua_pushcfunction(L, LuaSpawnDog);
    lua_setfield(L, -2, "spawn_dog");      // ship.mm.spawn_dog
    lua_setfield(L, -2, "mm");
    lua_pop(L, 1);
}
```

Para publicar um evento comum a partir do host (ex.: uma tecla), use
`gModHost->DispatchEvent("input.hotkey", payload)`. Veja o exemplo completo
(hotkey + spawn de cachorro) no fork do MM e em
[`examples/dog-spawner`](../examples/dog-spawner/).

## Notas

- Mantenha o commit do submódulo `extern/ship-lua` **fixado** e registre o motivo ao
  atualizá-lo (ver `AGENTS.md`).
- O núcleo do ShipLua **nunca** inclui headers do jogo; só o adaptador (dentro do
  fork) toca internals.
- Distribua o **fork buildável** ou o **source + este guia** — nunca ROMs/assets.
