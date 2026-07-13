# Inventário de hooks dos hosts

## Escopo observado

| Host | Repositório observado | Commit |
|---|---|---|
| Shipwright | `BaiterYamato/Shipwright-HyliaFoundry` | `3bed8cc2f3c1fe67b9fca15e6434551fcec57d0c` |
| 2Ship2Harkinian | `BaiterYamato/2ship2harkinian` | `b3cc366288c0a3e7583810be816d5fed3cd54ac2` |

As assinaturas devem ser revalidadas quando qualquer um desses commits mudar.

## Arquivos proprietários dos hooks

### Shipwright

- declaração: `soh/soh/Enhancements/game-interactor/GameInteractor_HookTable.h`;
- API de registro: `soh/soh/Enhancements/game-interactor/GameInteractor.h`;
- ponte C e execução: `soh/soh/Enhancements/game-interactor/GameInteractor_Hooks.h/.cpp`;
- bootstrap: `soh/soh/OTRGlobals.cpp`.

`GameInteractor::Instance` é criado em `OTRGlobals.cpp:1541`. O bootstrap ShipLua deve ocorrer depois dessa linha e antes do início efetivo do gameplay.

### 2Ship2Harkinian

- declaração: `mm/2s2h/GameInteractor/GameInteractor_HookTable.h`;
- API de registro e ponte C: `mm/2s2h/GameInteractor/GameInteractor.h`;
- execução: `mm/2s2h/GameInteractor/GameInteractor.cpp`;
- bootstrap: `mm/2s2h/BenPort.cpp`.

`GameInteractor::Instance` é criado em `BenPort.cpp:960`; `RegisterOwnHooks()` ocorre em `BenPort.cpp:968`. O bootstrap ShipLua deve ser inserido depois da criação da instância e coordenado com o registro dos hooks nativos.

Nenhum bootstrap observado destrói explicitamente `GameInteractor::Instance`. O adaptador ShipLua deve ter lifecycle próprio e encerrar antes da desmontagem do contexto e do logging do host.

## Matriz comum inicial

| Evento ShipLua | Shipwright | 2Ship | Normalização obrigatória |
|---|---|---|---|
| `game.frame` | `OnGameFrameUpdate()` | `OnGameStateUpdate()` | payload vazio; thread principal |
| `scene.enter` | `OnSceneInit(int16_t sceneNum)` | `OnSceneInit(s8 sceneId, s8 spawnNum)` | expor apenas `scene_id` no evento comum |
| `actor.init` | `OnActorInit(void* actor)` | `OnActorInit(Actor* actor)` | converter imediatamente para handle/snapshot |
| `actor.update` | `OnActorUpdate(void* actor)` | `OnActorUpdate(Actor* actor)` | nunca reter nem expor ponteiro |
| `actor.destroy` | `OnActorDestroy(void* actor)` | `OnActorDestroy(Actor* actor)` | invalidar geração antes de notificar Lua |
| `save.loaded` | `OnLoadGame(int32_t)` e `OnLoadFile(int32_t)` | `OnSaveLoad(s16)` | adaptador define uma emissão por carga lógica |
| `text.open` | `OnOpenText(uint16_t*, bool*)` | `OnOpenText(u16*, bool*)` | snapshot do ID; mutação exige capacidade futura |
| `audio.sequence_started` | `OnSeqPlayerInit(int32_t, int32_t)` | `OnSeqPlayerInit(s32, s32)` | índices inteiros estáveis |
| `game.shutdown` | integração de bootstrap | integração de bootstrap | evento do runtime, não hook bruto |

## Hooks semelhantes com semântica divergente

| Tema | Shipwright | 2Ship | Decisão |
|---|---|---|---|
| frame | `OnGameFrameUpdate` | `OnGameStateUpdate` | um evento comum `game.frame` |
| save | `OnLoadGame`/`OnLoadFile` | `OnSaveLoad`/`OnFileSelectSaveLoad` | deduplicar no adaptador; não mapear nomes 1:1 |
| cena | `sceneNum` | `sceneId + spawnNum` | `spawnNum` fica em namespace/capacidade MM |
| boss | `OnBossDefeat(void*)` | `OnBossDefeated(s16 actorId)` | snapshot comum futuro, não ponte direto |
| vanilla | `OnVanillaBehavior` | `ShouldVanillaBehavior` | filtro exige RFC específica |
| bottle | `OnPlayerBottleUpdate(int16_t)` | `OnBottleContentsUpdate(u8)` | traduzir valor antes de API comum |

## Superfícies exclusivas relevantes

### 2Ship2Harkinian

- `OnRoomInit` e `AfterRoomSceneCommands`;
- `BeforeEndOfCycleSave`, `AfterEndOfCycleSave` e `BeforeMoonCrash`;
- `OnFileSelectSaveLoad(..., isOwlSave, SaveContext*)`;
- `BeforeInterfaceClockDraw` e `AfterInterfaceClockDraw`.

Capacidades candidatas: `mm.room.events`, `mm.cycle`, `mm.owl_save` e `mm.clock`.

### Shipwright

- `OnOcarinaSongAction` e `OnOcarinaNote`;
- `OnDungeonKeyUsed`;
- `OnLinkEquipmentChange`;
- `OnPlayerHealthChange` e `OnPlayerUseItem`;
- `OnRandoEntranceDiscovered`.

Capacidades candidatas: `oot.ocarina`, `oot.dungeon_keys`, `oot.equipment` e `oot.player.age` após confirmação adicional.

## Regras para implementação dos adaptadores

1. Registrar hooks somente depois de `GameInteractor::Instance` existir.
2. Guardar todo `HOOK_ID` retornado e cancelar cada registro no shutdown.
3. Executar callbacks ShipLua somente na thread principal.
4. Converter tipos internos no adaptador; o núcleo não inclui headers do jogo.
5. Não transportar `Actor*`, `PlayState*`, `SaveContext*` ou ponteiros de texto para o núcleo.
6. Deduplicar eventos de save por transição lógica, não pelo nome do hook.
7. Tratar payload adicional de um host como capacidade, nunca como campo comum fictício.
8. Falhar build/teste de drift quando hook, assinatura ou bootstrap observado mudar.

## Pontos primários de patch futuros

| Trilha | Arquivo primário | Responsabilidade |
|---|---|---|
| OoT bootstrap | `soh/soh/OTRGlobals.cpp` | criar e encerrar integração ShipLua |
| OoT adapter | novo diretório sob `soh/soh/Enhancements/shiplua/` | traduzir hooks/tipos OoT |
| MM bootstrap | `mm/2s2h/BenPort.cpp` | criar e encerrar integração ShipLua |
| MM adapter | novo diretório sob `mm/2s2h/ShipLua/` | traduzir hooks/tipos MM |
| núcleo | repositório `ship-lua` | runtime, schemas, dispatcher e serviços sem headers dos hosts |

Esses caminhos são propostos. Cada patch deve seguir as convenções CMake vigentes no commit usado para implementação.
