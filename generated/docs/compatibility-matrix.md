<!-- Gerado por tools/generate_api_contracts.py. Não edite manualmente. -->
# Matriz de compatibilidade — ShipLua Runtime API

Versão da API: `0.3.0` (schema `1`).
Derivada dos schemas canônicos em `schema/` (IDL); regenere com
`tools/generate_api_contracts.py`.

## Funções

| Função | Desde | Estabilidade | OoT | MM | Capability | Erros |
|---|---|---|---|---|---|---|
| `ship.game.id` | `0.1.0` | `stable` | sim | sim | — | — |
| `ship.game.host_version` | `0.1.0` | `stable` | sim | sim | — | — |
| `ship.runtime.version` | `0.1.0` | `stable` | sim | sim | — | — |
| `ship.api.version` | `0.1.0` | `stable` | sim | sim | — | — |
| `ship.capabilities.has` | `0.1.0` | `stable` | sim | sim | — | `invalid_argument` |
| `ship.capabilities.list` | `0.1.0` | `stable` | sim | sim | — | — |
| `ship.events.on` | `0.1.0` | `stable` | sim | sim | — | `invalid_argument`, `unsupported` |
| `ship.events.off` | `0.1.0` | `stable` | sim | sim | — | `invalid_handle` |
| `ship.hotkeys.register` | `0.2.0` | `preview` | sim | sim | — | `invalid_argument`, `unsupported` |
| `ship.mm.player.jump` | `0.2.0` | `experimental` | — | sim | `mm.player.jump` | — |
| `ship.mm.spawn_dog` | `0.3.0` | `experimental` | — | sim | `mm.spawn_dog` | — |
| `ship.oot.player.jump` | `0.3.0` | `experimental` | sim | — | `oot.player.jump` | — |
| `ship.oot.spawn_dog` | `0.3.0` | `experimental` | sim | — | `oot.spawn_dog` | — |
| `ship.log.debug` | `0.1.0` | `stable` | sim | sim | — | `invalid_argument` |
| `ship.log.info` | `0.1.0` | `stable` | sim | sim | — | `invalid_argument` |
| `ship.log.warn` | `0.1.0` | `stable` | sim | sim | — | `invalid_argument` |
| `ship.log.error` | `0.1.0` | `stable` | sim | sim | — | `invalid_argument` |
| `ship.timer.after` | `0.3.0` | `experimental` | sim | sim | `core.timers` | `invalid_argument`, `resource_limit`, `unsupported` |
| `ship.timer.every` | `0.3.0` | `experimental` | sim | sim | `core.timers` | `invalid_argument`, `resource_limit`, `unsupported` |
| `ship.timer.cancel` | `0.3.0` | `experimental` | sim | sim | `core.timers` | `invalid_argument`, `invalid_handle` |
| `ship.storage.get` | `0.3.0` | `experimental` | sim | sim | `core.storage` | `invalid_argument`, `unsupported` |
| `ship.storage.set` | `0.3.0` | `experimental` | sim | sim | `core.storage` | `invalid_argument`, `resource_limit`, `unsupported` |
| `ship.storage.delete` | `0.3.0` | `experimental` | sim | sim | `core.storage` | `invalid_argument`, `unsupported` |
| `ship.storage.clear` | `0.3.0` | `experimental` | sim | sim | `core.storage` | `unsupported` |

Funções com disponibilidade específica (`oot`/`mm`) são instaladas pelo
adaptador do host quando a capability correspondente é anunciada; o núcleo
nunca registra `ship.oot.*` ou `ship.mm.*` (RFC 0001).

## Eventos

| Evento | Fase | Cancelável | OoT | MM | Capability |
|---|---|---:|---|---|---|
| `game.ready` | `mvp` | não | sim | sim | — |
| `game.frame` | `mvp` | não | sim | sim | — |
| `game.shutdown` | `mvp` | não | sim | sim | — |
| `scene.enter` | `host_bridge` | não | sim | sim | `scene.events` |
| `actor.init` | `host_bridge` | não | sim | sim | `actor.events` |
| `actor.update` | `host_bridge` | não | sim | sim | `actor.events` |
| `actor.destroy` | `host_bridge` | não | sim | sim | `actor.events` |
| `save.loaded` | `host_bridge` | não | sim | sim | `save.events` |
| `text.open` | `host_bridge` | não | sim | sim | `text.events` |
| `audio.sequence_started` | `host_bridge` | não | sim | sim | `audio.sequence.events` |
| `input.hotkey` | `host_bridge` | não | sim | sim | — |

## Capabilities

| Capability | Status | OoT | MM |
|---|---|---|---|
| `core.events` | `contract` | sim | sim |
| `core.timers` | `contract` | sim | sim |
| `core.input` | `contract` | sim | sim |
| `core.storage` | `contract` | sim | sim |
| `scene.events` | `contract` | sim | sim |
| `actor.events` | `contract` | sim | sim |
| `save.events` | `contract` | sim | sim |
| `text.events` | `contract` | sim | sim |
| `audio.sequence.events` | `contract` | sim | sim |
| `mm.room.events` | `planned` | — | sim |
| `mm.cycle` | `planned` | — | sim |
| `mm.owl_save` | `planned` | — | sim |
| `mm.clock` | `planned` | — | sim |
| `mm.player.jump` | `contract` | — | sim |
| `mm.spawn_dog` | `contract` | — | sim |
| `oot.player.jump` | `contract` | sim | — |
| `oot.spawn_dog` | `contract` | sim | — |
| `oot.ocarina` | `planned` | sim | — |
| `oot.dungeon_keys` | `planned` | sim | — |
| `oot.equipment` | `planned` | sim | — |

## Estabilidade das funções

| Estabilidade | Funções |
|---|---:|
| `stable` | 12 |
| `preview` | 1 |
| `experimental` | 11 |

