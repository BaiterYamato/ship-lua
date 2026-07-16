<!-- Gerado por tools/generate_api_docs.py. Não edite manualmente. -->
# Referência da API ShipLua

Versão da API: `0.3.0`. Versão do schema: `1`.

## Tipos

| Nome | Tipo | Contrato | Descrição |
|---|---|---|---|
| `game_id` | `enum` | `oot`, `mm` | Identificador público do jogo host. |
| `subscription` | `opaque` | Lua `integer` | Identificador opaco de inscrição em evento. |
| `event_options` | `object` | `priority: integer?` | Opções determinísticas de inscrição. |
| `actor_handle` | `object` | `slot: integer`, `generation: integer`, `game: game_id` | Handle validado por slot, geração e host. |
| `actor_snapshot` | `object` | `handle: actor_handle`, `actor_id: integer`, `category: integer` | Snapshot mínimo e estável de ator. |
| `hotkey_options` | `object` | `default: string?`, `label: string?` | Opções de registro de hotkey (tecla default e rótulo). |

## Funções

| Função | Argumentos | Retorno | Disponibilidade | Capability | Erros |
|---|---|---|---|---|---|
| `ship.game.id` | — | `game_id` | `common` | — | — |
| `ship.game.host_version` | — | `string` | `common` | — | — |
| `ship.runtime.version` | — | `string` | `common` | — | — |
| `ship.api.version` | — | `string` | `common` | — | — |
| `ship.capabilities.has` | `name: string` | `boolean` | `common` | — | `invalid_argument` |
| `ship.capabilities.list` | — | `array<string>` | `common` | — | — |
| `ship.events.on` | `event: string`, `options_or_callback: any`, `callback: callback?` | `subscription` | `common` | — | `invalid_argument`, `unsupported` |
| `ship.events.off` | `subscription: subscription` | `boolean` | `common` | — | `invalid_handle` |
| `ship.hotkeys.register` | `id: string`, `options: hotkey_options?`, `callback: callback` | `boolean` | `common` | — | `invalid_argument`, `unsupported` |
| `ship.world.travel` | `world: game_id`, `destination: string` | `boolean` | `common` | `world.travel` | `invalid_argument`, `unsupported`, `invalid_state`, `host_failure` |
| `ship.mm.player.jump` | — | `boolean` | `mm` | `mm.player.jump` | — |
| `ship.mm.spawn_dog` | — | `boolean` | `mm` | `mm.spawn_dog` | — |
| `ship.oot.player.jump` | — | `boolean` | `oot` | `oot.player.jump` | — |
| `ship.oot.spawn_dog` | — | `boolean` | `oot` | `oot.spawn_dog` | — |
| `ship.log.debug` | `message: string` | `nil` | `common` | — | `invalid_argument` |
| `ship.log.info` | `message: string` | `nil` | `common` | — | `invalid_argument` |
| `ship.log.warn` | `message: string` | `nil` | `common` | — | `invalid_argument` |
| `ship.log.error` | `message: string` | `nil` | `common` | — | `invalid_argument` |

## Eventos

| Evento | Tipo | Fase | Hosts | Cancelável | Capability | Payload |
|---|---|---|---|---:|---|---|
| `game.ready` | `observe` | `mvp` | `oot`, `mm` | não | — | `game_id: game_id`, `host_version: string`, `runtime_version: string`, `api_version: string` |
| `game.frame` | `observe` | `mvp` | `oot`, `mm` | não | — | `frame: integer` |
| `game.shutdown` | `observe` | `mvp` | `oot`, `mm` | não | — | — |
| `scene.enter` | `observe` | `host_bridge` | `oot`, `mm` | não | `scene.events` | `scene_id: integer` |
| `actor.init` | `observe` | `host_bridge` | `oot`, `mm` | não | `actor.events` | `actor: actor_snapshot` |
| `actor.update` | `observe` | `host_bridge` | `oot`, `mm` | não | `actor.events` | `actor: actor_snapshot` |
| `actor.destroy` | `observe` | `host_bridge` | `oot`, `mm` | não | `actor.events` | `handle: actor_handle` |
| `save.loaded` | `observe` | `host_bridge` | `oot`, `mm` | não | `save.events` | `slot: integer` |
| `text.open` | `observe` | `host_bridge` | `oot`, `mm` | não | `text.events` | `text_id: integer` |
| `audio.sequence_started` | `observe` | `host_bridge` | `oot`, `mm` | não | `audio.sequence.events` | `player_index: integer`, `sequence_id: integer` |
| `input.hotkey` | `observe` | `host_bridge` | `oot`, `mm` | não | — | `action: string`, `key: string` |

## Capabilities

| Capability | Estado | Hosts | Descrição |
|---|---|---|---|
| `scene.events` | `contract` | `oot`, `mm` | Eventos comuns de cena. |
| `actor.events` | `contract` | `oot`, `mm` | Eventos comuns de ator com handles e snapshots. |
| `save.events` | `contract` | `oot`, `mm` | Eventos comuns de carregamento de save. |
| `text.events` | `contract` | `oot`, `mm` | Eventos comuns de texto somente leitura. |
| `audio.sequence.events` | `contract` | `oot`, `mm` | Eventos de início de sequência de áudio. |
| `world.travel` | `contract` | `oot`, `mm` | Solicita handoff autenticado para um destino lógico no outro host. |
| `mm.room.events` | `planned` | `mm` | Eventos de room exclusivos de Majora's Mask. |
| `mm.cycle` | `planned` | `mm` | Lifecycle do ciclo de três dias. |
| `mm.owl_save` | `planned` | `mm` | Semântica de owl save. |
| `mm.clock` | `planned` | `mm` | Leitura estável do relógio de MM. |
| `mm.player.jump` | `contract` | `mm` | Aplica um impulso vertical validado ao jogador de Majora's Mask quando ele está no chão. |
| `mm.spawn_dog` | `contract` | `mm` | Spawna o cachorro de Clock Town (En_Dg) perto do jogador de Majora's Mask. |
| `oot.player.jump` | `contract` | `oot` | Aplica um impulso vertical validado ao jogador de OoT quando ele está no chão. |
| `oot.spawn_dog` | `contract` | `oot` | Spawna um cachorro (En_Dog) perto do jogador de OoT. |
| `oot.ocarina` | `planned` | `oot` | Eventos e estado de ocarina de OoT. |
| `oot.dungeon_keys` | `planned` | `oot` | Estado de chaves de dungeon de OoT. |
| `oot.equipment` | `planned` | `oot` | Equipamento específico de OoT. |

## Códigos de erro

`unsupported`, `invalid_argument`, `invalid_handle`, `invalid_state`, `permission_denied`, `resource_limit`, `host_failure`, `script_failure`.
