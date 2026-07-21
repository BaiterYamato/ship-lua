<!-- Gerado por tools/generate_api_docs.py. Não edite manualmente. -->
# Referência da API ShipLua

Versão da API: `0.4.0`. Versão do schema: `1`.

## Tipos

| Nome | Tipo | Contrato | Descrição |
|---|---|---|---|
| `game_id` | `enum` | `oot`, `mm` | Identificador público do jogo host. |
| `subscription` | `opaque` | Lua `integer` | Identificador opaco de inscrição em evento. |
| `timer_handle` | `opaque` | Lua `integer` | Identificador opaco de timer em frames. |
| `event_options` | `object` | `priority: integer?` | Opções determinísticas de inscrição. |
| `actor_handle` | `object` | `kind: string`, `slot: integer`, `generation: integer`, `scene_generation: integer` | Handle opaco validado por kind, slot, geração e cena. |
| `actor_position` | `object` | `x: number`, `y: number`, `z: number` | Posição absoluta finita em coordenadas do mundo. |
| `actor_rotation` | `object` | `x: number`, `y: number`, `z: number` | Rotação XYZ em graus. |
| `actor_spawn_options` | `object` | `position: actor_position`, `rotation: actor_rotation?` | Transform seguro para criação de ator. |
| `operation_error` | `object` | `code: string`, `message: string` | Erro estruturado retornado sem lançar lua_error. |
| `actor_snapshot` | `object` | `handle: actor_handle`, `actor_id: integer`, `category: integer` | Snapshot mínimo e estável de ator. |
| `hotkey_options` | `object` | `default: string?`, `label: string?` | Opções de registro de hotkey (tecla default e rótulo). |

## Funções

| Função | Argumentos | Retorno | Disponibilidade | Estabilidade | Desde | Capability | Erros |
|---|---|---|---|---|---|---|---|
| `ship.game.id` | — | `game_id` | `common` | `stable` | `0.1.0` | — | — |
| `ship.game.host_version` | — | `string` | `common` | `stable` | `0.1.0` | — | — |
| `ship.runtime.version` | — | `string` | `common` | `stable` | `0.1.0` | — | — |
| `ship.api.version` | — | `string` | `common` | `stable` | `0.1.0` | — | — |
| `ship.capabilities.has` | `name: string` | `boolean` | `common` | `stable` | `0.1.0` | — | `invalid_argument` |
| `ship.capabilities.list` | — | `array<string>` | `common` | `stable` | `0.1.0` | — | — |
| `ship.events.on` | `event: string`, `options_or_callback: any`, `callback: callback?` | `subscription` | `common` | `stable` | `0.1.0` | — | `invalid_argument`, `unsupported` |
| `ship.events.off` | `subscription: subscription` | `boolean` | `common` | `stable` | `0.1.0` | — | `invalid_handle` |
| `ship.hooks.result` | `value: any` | `boolean` | `common` | `experimental` | `0.4.0` | — | `invalid_argument` |
| `ship.hotkeys.register` | `id: string`, `options: hotkey_options?`, `callback: callback` | `boolean` | `common` | `preview` | `0.2.0` | — | `invalid_argument`, `unsupported` |
| `ship.actor.spawn` | `actor_type: string`, `options: actor_spawn_options` | `actor_handle, operation_error?` | `common` | `experimental` | `0.4.0` | `actor.spawn` | `invalid_argument`, `unsupported`, `permission_denied`, `invalid_state`, `resource_limit`, `host_failure` |
| `ship.actor.destroy` | `handle: actor_handle` | `boolean, operation_error?` | `common` | `experimental` | `0.4.0` | `actor.destroy` | `invalid_argument`, `unsupported`, `permission_denied`, `invalid_handle`, `host_failure` |
| `ship.actor.exists` | `handle: actor_handle` | `boolean, operation_error?` | `common` | `experimental` | `0.4.0` | `actor.exists` | `invalid_argument`, `unsupported`, `permission_denied`, `host_failure` |
| `ship.world.travel` | `world: game_id`, `destination: string` | `boolean` | `common` | `experimental` | `0.3.0` | `world.travel` | `invalid_argument`, `unsupported`, `invalid_state`, `host_failure` |
| `ship.mm.player.jump` | — | `boolean` | `mm` | `experimental` | `0.2.0` | `mm.player.jump` | — |
| `ship.mm.spawn_dog` | — | `boolean` | `mm` | `experimental` | `0.3.0` | `mm.spawn_dog` | — |
| `ship.mm.player.set_sword_skin` | `skin: string` | `boolean` | `mm` | `experimental` | `0.4.0` | `mm.player.sword_skin` | — |
| `ship.oot.player.jump` | — | `boolean` | `oot` | `experimental` | `0.3.0` | `oot.player.jump` | — |
| `ship.oot.player.set_bunny_hood` | `equipped: boolean` | `boolean` | `oot` | `experimental` | `0.4.0` | `oot.player.bunny_hood` | — |
| `ship.oot.player.set_mask` | `mask: string` | `boolean` | `oot` | `experimental` | `0.4.0` | `oot.player.mask` | — |
| `ship.player.set_speed_multiplier` | `factor: number` | `boolean` | `common` | `experimental` | `0.4.0` | `player.speed` | — |
| `ship.player.get` | `field: string` | `any` | `oot` | `experimental` | `0.4.0` | `player.fields` | — |
| `ship.player.set` | `field: string`, `value: number` | `boolean` | `oot` | `experimental` | `0.4.0` | `player.fields` | — |
| `ship.oot.player.attach_model` | `slot: string`, `path: string` | `boolean` | `oot` | `experimental` | `0.4.0` | `oot.player.attach_model` | — |
| `ship.oot.player.set_damage_immunity` | `kind: string`, `enabled: boolean` | `boolean` | `oot` | `experimental` | `0.4.0` | `oot.player.immunity` | — |
| `ship.oot.player.set_weight` | `weight: string` | `boolean` | `oot` | `experimental` | `0.4.0` | `oot.player.weight` | — |
| `ship.oot.player.set_roll_mode` | `mode: string` | `boolean` | `oot` | `experimental` | `0.4.0` | `oot.player.roll` | — |
| `ship.oot.spawn_dog` | — | `boolean` | `oot` | `experimental` | `0.3.0` | `oot.spawn_dog` | — |
| `ship.log.debug` | `message: string` | `nil` | `common` | `stable` | `0.1.0` | — | `invalid_argument` |
| `ship.log.info` | `message: string` | `nil` | `common` | `stable` | `0.1.0` | — | `invalid_argument` |
| `ship.log.warn` | `message: string` | `nil` | `common` | `stable` | `0.1.0` | — | `invalid_argument` |
| `ship.log.error` | `message: string` | `nil` | `common` | `stable` | `0.1.0` | — | `invalid_argument` |
| `ship.timer.after` | `frames: integer`, `callback: callback` | `timer_handle` | `common` | `experimental` | `0.3.0` | `core.timers` | `invalid_argument`, `resource_limit`, `unsupported` |
| `ship.timer.every` | `frames: integer`, `callback: callback` | `timer_handle` | `common` | `experimental` | `0.3.0` | `core.timers` | `invalid_argument`, `resource_limit`, `unsupported` |
| `ship.timer.cancel` | `handle: timer_handle` | `boolean` | `common` | `experimental` | `0.3.0` | `core.timers` | `invalid_argument`, `invalid_handle` |
| `ship.storage.get` | `key: string`, `default: any?` | `any` | `common` | `experimental` | `0.3.0` | `core.storage` | `invalid_argument`, `unsupported` |
| `ship.storage.set` | `key: string`, `value: any` | `boolean` | `common` | `experimental` | `0.3.0` | `core.storage` | `invalid_argument`, `resource_limit`, `unsupported` |
| `ship.storage.delete` | `key: string` | `boolean` | `common` | `experimental` | `0.3.0` | `core.storage` | `invalid_argument`, `unsupported` |
| `ship.storage.clear` | — | `integer` | `common` | `experimental` | `0.3.0` | `core.storage` | `unsupported` |

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
| `hook.oot.player.speed.run` | `transform` | `hook_bridge` | `oot` | não | `hooks.bridge` | `speed: number` |
| `hook.oot.player.fall_damage` | `transform` | `hook_bridge` | `oot` | não | `hooks.bridge` | — |
| `hook.oot.item.receive` | `observe` | `hook_bridge` | `oot` | não | `hooks.bridge` | `item_id: integer`, `get_item_id: integer` |
| `hook.oot.player.health_change` | `observe` | `hook_bridge` | `oot` | não | `hooks.bridge` | `amount: integer` |
| `hook.oot.player.bonk` | `observe` | `hook_bridge` | `oot` | não | `hooks.bridge` | — |
| `hook.mm.player.speed.walk` | `transform` | `hook_bridge` | `mm` | não | `hooks.bridge` | `speed: number` |
| `hook.mm.player.goron_roll.consume_magic` | `transform` | `hook_bridge` | `mm` | não | `hooks.bridge` | — |
| `hook.mm.player.goron_roll.disable_spike_mode` | `transform` | `hook_bridge` | `mm` | não | `hooks.bridge` | — |
| `hook.mm.player.goron_roll.increase_spike_level` | `transform` | `hook_bridge` | `mm` | não | `hooks.bridge` | — |
| `hook.mm.item.give` | `observe` | `hook_bridge` | `mm` | não | `hooks.bridge` | `item: integer` |

## Capabilities

| Capability | Estado | Hosts | Descrição |
|---|---|---|---|
| `core.events` | `contract` | `oot`, `mm` | Eventos e lifecycle centrais do host. |
| `hooks.bridge` | `contract` | `oot`, `mm` | Ponte genérica para os pontos de instrumentação nativos (VB_*/On* do GameInteractor) — eventos hook.* assináveis com ship.events.on e decididos com ship.hooks.result, sem precisar de função nativa dedicada por habilidade. |
| `core.timers` | `contract` | `oot`, `mm` | Timers por frame com ownership por mod. |
| `core.input` | `contract` | `oot`, `mm` | Registro de hotkeys e eventos de input. |
| `core.storage` | `contract` | `oot`, `mm` | Armazenamento chave-valor com namespace por mod. |
| `scene.events` | `contract` | `oot`, `mm` | Eventos comuns de cena. |
| `actor.events` | `contract` | `oot`, `mm` | Eventos comuns de ator com handles e snapshots. |
| `actor.spawn` | `contract` | `oot`, `mm` | Cria um ator allowlisted com ownership e handle seguro. |
| `actor.destroy` | `contract` | `oot`, `mm` | Destrói um ator pertencente ao mod chamador. |
| `actor.exists` | `contract` | `oot`, `mm` | Consulta a validade de um handle de ator pertencente ao mod. |
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
| `mm.player.sword_skin` | `contract` | `mm` | Alterna o visual da espada Kokiri empunhada entre o modelo de Majora's Mask e o de Ocarina of Time (lido do oot.o2r vizinho). |
| `oot.player.jump` | `contract` | `oot` | Aplica um impulso vertical validado ao jogador de OoT quando ele está no chão. |
| `oot.spawn_dog` | `contract` | `oot` | Spawna um cachorro (En_Dog) perto do jogador de OoT. |
| `oot.player.bunny_hood` | `contract` | `oot` | Veste a Bunny Hood em OoT com o comportamento de Majora's Mask (corrida mais rápida e pulo maior). |
| `oot.player.mask` | `contract` | `oot` | Equipa qualquer máscara de OoT pelo nome lógico, sem ocupar um botão C. |
| `player.speed` | `contract` | `oot`, `mm` | Multiplica a velocidade de movimento do jogador por um fator validado (0.1–5.0); 1.0 restaura. |
| `player.fields` | `contract` | `oot` | Lê e escreve campos nomeados do jogador (vida, magia, rupees, posição, velocidade) com validação de faixa. |
| `oot.player.attach_model` | `contract` | `oot` | Desenha uma display list arbitrária no jogador por caminho de resource, incluindo assets de mod e do jogo vizinho. |
| `mod.assets` | `contract` | `oot` | Archives (.o2r/.otr) na pasta de mods ficam endereçáveis sob mod/<nome>/, permitindo que um mod traga conteúdo próprio. |
| `oot.player.immunity` | `contract` | `oot` | Concede imunidade a um tipo de dano (hoje: fogo). |
| `oot.player.weight` | `contract` | `oot` | Alterna o peso do jogador entre normal e pesado (afunda na água, resiste a empurrão). |
| `oot.player.roll` | `contract` | `oot` | Ativa rolamento contínuo e dirigível, encadeado indefinidamente. |
| `oot.ocarina` | `planned` | `oot` | Eventos e estado de ocarina de OoT. |
| `oot.dungeon_keys` | `planned` | `oot` | Estado de chaves de dungeon de OoT. |
| `oot.equipment` | `planned` | `oot` | Equipamento específico de OoT. |

## Códigos de erro

`unsupported`, `invalid_argument`, `invalid_handle`, `invalid_state`, `permission_denied`, `resource_limit`, `host_failure`, `script_failure`.
