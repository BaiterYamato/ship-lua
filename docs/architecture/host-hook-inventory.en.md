> English translation of [docs/architecture/host-hook-inventory.md](host-hook-inventory.md). The Portuguese document remains the canonical project record.

# Host hook inventory

## Scope observed

| Host | Observed repository | Commit |
|---|---|---|
| Shipwright | `BaiterYamato/Shipwright-HyliaFoundry` | `3bed8cc2f3c1fe67b9fca15e6434551fcec57d0c` |
| 2Ship2Harkinian | `BaiterYamato/2ship2harkinian` | `b3cc366288c0a3e7583810be816d5fed3cd54ac2` |

Signatures must be revalidated when any of these commits change.

## Proprietary hook files

### Shipwright

- declaration: `soh/soh/Enhancements/game-interactor/GameInteractor_HookTable.h`;
- Registration API: `soh/soh/Enhancements/game-interactor/GameInteractor.h`;
- C bridge and execution: `soh/soh/Enhancements/game-interactor/GameInteractor_Hooks.h/.cpp`;
- bootstrap: `soh/soh/OTRGlobals.cpp`.

`GameInteractor::Instance` is created in `OTRGlobals.cpp:1541`. The ShipLua bootstrap must occur after this line and before the actual start of gameplay.

### 2Ship2Harkinian

- declaration: `mm/2s2h/GameInteractor/GameInteractor_HookTable.h`;
- Registration API and C bridge: `mm/2s2h/GameInteractor/GameInteractor.h`;
- execution: `mm/2s2h/GameInteractor/GameInteractor.cpp`;
- bootstrap: `mm/2s2h/BenPort.cpp`.

`GameInteractor::Instance` is created in `BenPort.cpp:960`; `RegisterOwnHooks()` occurs in `BenPort.cpp:968`. The ShipLua bootstrap must be inserted after instance creation and coordinated with the native hooks registration.

No observed bootstrap explicitly destroys `GameInteractor::Instance`. The ShipLua adapter must have its own lifecycle and terminate before context disassembly and host logging.

## Initial common matrix

| ShipLua Event | Shipwright | 2Ship | Mandatory standardization |
|---|---|---|---|
| `game.frame` | `OnGameFrameUpdate()` | `OnGameStateUpdate()` | empty payload; main thread |
| `scene.enter` | `OnSceneInit(int16_t sceneNum)` | `OnSceneInit(s8 sceneId, s8 spawnNum)` | expose only `scene_id` in common event |
| `actor.init` | `OnActorInit(void* actor)` | `OnActorInit(Actor* actor)` | immediately convert to handle/snapshot |
| `actor.update` | `OnActorUpdate(void* actor)` | `OnActorUpdate(Actor* actor)` | never retain nor expose pointer |
| `actor.destroy` | `OnActorDestroy(void* actor)` | `OnActorDestroy(Actor* actor)` | invalidate generation before notifying Lua |
| `save.loaded` | `OnLoadGame(int32_t)` and `OnLoadFile(int32_t)` | `OnSaveLoad(s16)` | adapter defines one emission per logical load |
| `text.open` | `OnOpenText(uint16_t*, bool*)` | `OnOpenText(u16*, bool*)` | ID snapshot; mutation requires future capacity |
| `audio.sequence_started` | `OnSeqPlayerInit(int32_t, int32_t)` | `OnSeqPlayerInit(s32, s32)` | stable integer indexes |
| `game.shutdown` | bootstrap integration | bootstrap integration | runtime event, not raw hook |

## Similar hooks with divergent semantics

| Theme | Shipwright | 2Ship | Decision |
|---|---|---|---|
| frame | `OnGameFrameUpdate` | `OnGameStateUpdate` | a common event `game.frame` |
| save | `OnLoadGame`/`OnLoadFile` | `OnSaveLoad`/`OnFileSelectSaveLoad` | deduplicate on adapter; do not map names 1:1 |
| scene | `sceneNum` | `sceneId + spawnNum` | `spawnNum` is in namespace/capacity MM |
| boss | `OnBossDefeat(void*)` | `OnBossDefeated(s16 actorId)` | future common snapshot, not direct bridge |
| vanilla | `OnVanillaBehavior` | `ShouldVanillaBehavior` | filter requires specific RFC |
| bottles | `OnPlayerBottleUpdate(int16_t)` | `OnBottleContentsUpdate(u8)` | translate value before common API |

## Relevant unique surfaces

### 2Ship2Harkinian

- `OnRoomInit` and `AfterRoomSceneCommands`;
- `BeforeEndOfCycleSave`, `AfterEndOfCycleSave` and `BeforeMoonCrash`;
- `OnFileSelectSaveLoad(..., isOwlSave, SaveContext*)`;
- `BeforeInterfaceClockDraw` and `AfterInterfaceClockDraw`.

Candidate capabilities: `mm.room.events`, `mm.cycle`, `mm.owl_save` and `mm.clock`.

### Shipwright

- `OnOcarinaSongAction` and `OnOcarinaNote`;
- `OnDungeonKeyUsed`;
- `OnLinkEquipmentChange`;
- `OnPlayerHealthChange` and `OnPlayerUseItem`;
- `OnRandoEntranceDiscovered`.

Candidate capabilities: `oot.ocarina`, `oot.dungeon_keys`, `oot.equipment` and `oot.player.age` upon further confirmation.

## Rules for implementing adapters

1. Register hooks only after `GameInteractor::Instance` exists.
2. Save all returned `HOOK_ID` and cancel each record at shutdown.
3. Execute ShipLua callbacks only on the main thread.
4. Convert internal types in the adapter; core does not include game headers.
5. Do not carry `Actor*`, `PlayState*`, `SaveContext*` or text pointers to the core.
6. Deduplicate save events by logical transition, not by hook name.
7. Treat additional payload from a host as capacity, never as a fictitious common field.
8. Drift build/test fails when observed hook, signature or bootstrap changes.

## Future primary patch points

| Trail | Primary file | Responsibility |
|---|---|---|
| OoT bootstrap | `soh/soh/OTRGlobals.cpp` | create and close ShipLua integration |
| OoT adapter | new directory under `soh/soh/Enhancements/shiplua/` | translate OoT hooks/types |
| MM bootstrap | `mm/2s2h/BenPort.cpp` | create and close ShipLua integration |
| MM adapter | new directory under `mm/2s2h/ShipLua/` | translate MM hooks/types |
| core | repository `ship-lua` | runtime, schemas, dispatcher and services without host headers |

These paths are proposed. Each patch must follow the CMake conventions in force in the commit used for implementation.
