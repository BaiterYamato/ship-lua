# RFC 0001 — Arquitetura do runtime compartilhado

- Status: proposta para revisão
- Data: 2026-07-13
- Tarefas: ARCH-001, ARCH-002

## Motivação

Shipwright e 2Ship2Harkinian expõem hooks semelhantes, mas não possuem nomes, assinaturas ou lifecycle idênticos. Copiar o runtime para os dois forks criaria divergência e obrigaria mods comuns a conhecer estruturas internas de cada jogo.

Esta RFC fixa uma fronteira: o núcleo compartilhado conhece somente interfaces e tipos ShipLua; cada fork possui um adaptador pequeno que traduz o `GameInteractor` e o lifecycle do host.

## Arquitetura

```text
mod Lua
  ↓
API Lua versionada + capabilities
  ↓
runtime, dispatcher, manifesto e serviços compartilhados
  ↓
IGameAdapter
  ├── ShipwrightAdapter
  └── TwoShipAdapter
       ↓
GameInteractor + estruturas internas do host
```

O repositório `ship-lua` é a fonte canônica. Os forks contêm integração CMake, bootstrap, adaptador e hooks estritamente necessários.

## Contrato C++ do adaptador

O contrato inicial deve ser equivalente a:

```cpp
enum class GameId { Oot, Mm };

struct HostInfo {
    GameId game;
    std::string hostVersion;
    std::string hostCommit;
};

class IGameAdapter {
  public:
    virtual ~IGameAdapter() = default;
    virtual HostInfo GetHostInfo() const = 0;
    virtual std::vector<std::string> GetCapabilities() const = 0;
    virtual Result<void> RegisterHooks(IHostEventSink& sink) = 0;
    virtual void UnregisterHooks() noexcept = 0;
};
```

Regras:

- `IGameAdapter` e o núcleo não incluem headers OoT/MM;
- o adaptador é dono dos `HOOK_ID` registrados;
- `UnregisterHooks()` é idempotente;
- callbacks entram no núcleo somente na thread principal;
- exceções nunca atravessam callbacks C/C++ do host;
- ponteiros internos são convertidos antes de chamar `IHostEventSink`.

## Contrato Lua inicial

O primeiro vertical slice expõe somente:

```lua
local ship = require("ship")

ship.game.id()
ship.game.host_version()
ship.runtime.version()
ship.api.version()
ship.capabilities.has(name)
ship.capabilities.list()

ship.events.on("game.ready", function(event) end)
ship.events.off(subscription)

ship.log.debug(message)
ship.log.info(message)
ship.log.warn(message)
ship.log.error(message)
```

`require("ship")` é resolvido por loader interno. O runtime não abre `package`, filesystem nativo ou DLL/SO para implementar esse módulo.

## Eventos do primeiro contrato

| Evento | Payload comum | OoT | MM |
|---|---|---:|---:|
| `game.ready` | `game_id`, versões | sim | sim |
| `game.frame` | contador monotônico do runtime | sim | sim |
| `game.shutdown` | vazio | sim | sim |
| `scene.enter` | `scene_id` | sim | sim |
| `actor.init/update/destroy` | handle + snapshot estável | sim | sim |
| `save.loaded` | slot lógico | sim | sim |
| `text.open` | `text_id` somente leitura | sim | sim |
| `audio.sequence_started` | `player_index`, `sequence_id` | sim | sim |

O contador de frame pertence ao runtime. `scene.enter` não inventa `spawn_id` em OoT. Recursos adicionais de MM são publicados sob `ship.mm.*` e capability correspondente.

## Capacidades

Capacidades comuns iniciais:

```text
scene.events
actor.events
save.events
text.events
audio.sequence.events
```

Capacidades específicas candidatas:

```text
mm.room.events
mm.cycle
mm.owl_save
mm.clock
oot.ocarina
oot.dungeon_keys
oot.equipment
```

Uma capacidade somente é anunciada quando o adaptador implementa e testa integralmente sua semântica. Ausência retorna `false`; não é erro e não ativa simulação.

## Comportamento por host

### Shipwright

- bootstrap depois da criação de `GameInteractor::Instance` em `OTRGlobals.cpp`;
- `game.frame` deriva de `OnGameFrameUpdate`;
- `scene.enter` deriva de `OnSceneInit(sceneNum)`;
- save exige deduplicação entre `OnLoadGame` e `OnLoadFile`;
- callbacks de ator convertem `void*` para snapshot no adaptador OoT.

### 2Ship2Harkinian

- bootstrap depois da criação de `GameInteractor::Instance` em `BenPort.cpp`;
- `game.frame` deriva de `OnGameStateUpdate`;
- `scene.enter` deriva de `OnSceneInit(sceneId, spawnNum)`, mas o payload comum contém apenas cena;
- `OnRoomInit` e eventos de ciclo exigem capabilities MM;
- callbacks de ator convertem `Actor*` para snapshot no adaptador MM.

## Handles e vida útil

Lua nunca recebe ponteiro ou endereço. Um handle público contém, no mínimo:

```cpp
struct ActorHandle {
    std::uint32_t slot;
    std::uint32_t generation;
    GameId game;
};
```

O registry invalida a geração antes de emitir `actor.destroy`. Toda leitura ou escrita valida host, slot, geração e estado atual.

## Ordem e isolamento

O `GameInteractor` transporta o sinal bruto; ele não define a ordem pública ShipLua. O dispatcher compartilhado ordena por:

1. dependências resolvidas;
2. `load.after`/`load.before`;
3. prioridade do mod;
4. prioridade do callback;
5. ID do mod;
6. ID de registro.

Cada callback usa chamada Lua protegida. Falha incrementa contador do mod, preserva outros runtimes e pode desativar somente o mod reincidente.

## Erros

Fronteiras C++ retornam `Result<T>` com `ErrorCode`. Lua recebe erro estável, sem mensagem dependente do layout interno do host.

```text
unsupported
invalid_argument
invalid_handle
invalid_state
permission_denied
resource_limit
host_failure
script_failure
```

Função protegida nunca lança exceção através da ABI C/Lua ou callback do `GameInteractor`.

## Compatibilidade e versões

São independentes: `host_version`, `runtime_version`, `api_version` e `mod_version`.

A API usa SemVer. Recurso compatível incrementa `MINOR`; correção contratualmente compatível incrementa `PATCH`; quebra exige `MAJOR` e RFC. Atualização do host não altera automaticamente `api_version`.

API depreciada permanece funcional por pelo menos um ciclo `MINOR`, recebe substituição documentada e só é removida em `MAJOR`.

## Segurança

- sem `io`, `debug`, `os.execute`, `package.loadlib`, FFI, rede ou subprocessos por padrão;
- nenhum header ou ponteiro do host cruza o adaptador;
- callbacks executam na thread principal;
- pacote e storage bloqueiam absoluto, `..`, symlinks e colisões portáveis;
- limites de memória, callbacks, arquivos e bytes são configuráveis;
- nenhuma telemetria;
- nenhum download ou atualização executável automática.

## Plano de testes

1. testes unitários do núcleo sem ROM;
2. adapter tests com sinks sintéticos para cada assinatura mapeada;
3. golden traces comparando payloads OoT/MM;
4. mod comum `hello-world.shipmod` nos dois hosts;
5. unload comprovando remoção de hooks e callbacks;
6. drift check dos HookTable, bootstrap e CMake em ambos os hosts;
7. matriz Linux e Windows obrigatória; macOS recomendada antes do primeiro release.

## Plano de implementação

1. schemas de API, eventos e capabilities;
2. dispatcher e módulo interno `ship`;
3. `IGameAdapter` e sink sintético no núcleo;
4. bootstrap mínimo OoT e MM;
5. lifecycle/frame, depois cena, atores e save;
6. conformance do primeiro vertical slice;
7. capabilities específicas somente por RFC adicional.

## Depreciação

Alterações na fronteira pública `IGameAdapter`, nomes de eventos ou payloads exigem emenda/RFC, análise de compatibilidade e plano de migração. Detalhes privados do adaptador podem mudar para acompanhar upstream sem alterar a API Lua.

## Alternativas rejeitadas

- duplicar runtime nos forks: cria drift e duas APIs;
- expor todos os headers automaticamente: vaza ABI instável e ponteiros;
- usar LuaJIT FFI: remove isolamento e portabilidade;
- fazer o `GameInteractor` ser o contrato público: nomes e assinaturas já divergem;
- simular room/ciclo no OoT: produz semântica falsa e quebra detecção de capability.
