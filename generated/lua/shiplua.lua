---@meta ShipLua
-- Gerado por tools/generate_api_docs.py. Não edite manualmente.
-- API 0.4.0 / schema 1

--- Identificador público do jogo host.
---@alias ShipLuaGameId "oot"|"mm"

--- Identificador opaco de inscrição em evento.
---@alias ShipLuaSubscription integer

--- Identificador opaco de timer em frames.
---@alias ShipLuaTimerHandle integer

--- Opções determinísticas de inscrição.
---@class ShipLuaEventOptions
---@field priority? integer

--- Handle opaco validado por kind, slot, geração e cena.
---@class ShipLuaActorHandle
---@field kind string
---@field slot integer
---@field generation integer
---@field scene_generation integer

--- Posição absoluta finita em coordenadas do mundo.
---@class ShipLuaActorPosition
---@field x number
---@field y number
---@field z number

--- Rotação XYZ em graus.
---@class ShipLuaActorRotation
---@field x number
---@field y number
---@field z number

--- Transform seguro para criação de ator.
---@class ShipLuaActorSpawnOptions
---@field position ShipLuaActorPosition
---@field rotation? ShipLuaActorRotation

--- Erro estruturado retornado sem lançar lua_error.
---@class ShipLuaOperationError
---@field code string
---@field message string

--- Snapshot mínimo e estável de ator.
---@class ShipLuaActorSnapshot
---@field handle ShipLuaActorHandle
---@field actor_id integer
---@field category integer

--- Opções de registro de hotkey (tecla default e rótulo).
---@class ShipLuaHotkeyOptions
---@field default? string
---@field label? string

---@class ShipLuaEventGameReady
---@field game_id ShipLuaGameId
---@field host_version string
---@field runtime_version string
---@field api_version string

---@class ShipLuaEventGameFrame
---@field frame integer

---@class ShipLuaEventGameShutdown

---@class ShipLuaEventSceneEnter
---@field scene_id integer

---@class ShipLuaEventActorInit
---@field actor ShipLuaActorSnapshot

---@class ShipLuaEventActorUpdate
---@field actor ShipLuaActorSnapshot

---@class ShipLuaEventActorDestroy
---@field handle ShipLuaActorHandle

---@class ShipLuaEventSaveLoaded
---@field slot integer

---@class ShipLuaEventTextOpen
---@field text_id integer

---@class ShipLuaEventAudioSequenceStarted
---@field player_index integer
---@field sequence_id integer

---@class ShipLuaEventInputHotkey
---@field action string
---@field key string

---@alias ShipLuaEventName "game.ready"|"game.frame"|"game.shutdown"|"scene.enter"|"actor.init"|"actor.update"|"actor.destroy"|"save.loaded"|"text.open"|"audio.sequence_started"|"input.hotkey"

ship = ship or {}
ship.actor = ship.actor or {}
ship.api = ship.api or {}
ship.capabilities = ship.capabilities or {}
ship.events = ship.events or {}
ship.game = ship.game or {}
ship.hotkeys = ship.hotkeys or {}
ship.log = ship.log or {}
ship.mm = ship.mm or {}
ship.oot = ship.oot or {}
ship.player = ship.player or {}
ship.runtime = ship.runtime or {}
ship.storage = ship.storage or {}
ship.timer = ship.timer or {}
ship.world = ship.world or {}
ship.mm.player = ship.mm.player or {}
ship.oot.player = ship.oot.player or {}

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: nenhum.
---@return ShipLuaGameId
function ship.game.id() end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: nenhum.
---@return string
function ship.game.host_version() end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: nenhum.
---@return string
function ship.runtime.version() end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: nenhum.
---@return string
function ship.api.version() end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_argument.
---@param name string
---@return boolean
function ship.capabilities.has(name) end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: nenhum.
---@return string[]
function ship.capabilities.list() end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_argument, unsupported.
---@param event ShipLuaEventName
---@param options_or_callback any
---@param callback? function
---@return ShipLuaSubscription
function ship.events.on(event, options_or_callback, callback) end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_handle.
---@param subscription ShipLuaSubscription
---@return boolean
function ship.events.off(subscription) end

--- API common; estabilidade: preview; desde: 0.2.0; capability: comum; erros: invalid_argument, unsupported.
---@param id string
---@param options? ShipLuaHotkeyOptions
---@param callback function
---@return boolean
function ship.hotkeys.register(id, options, callback) end

--- API common; estabilidade: experimental; desde: 0.4.0; capability: actor.spawn; erros: invalid_argument, unsupported, permission_denied, invalid_state, resource_limit, host_failure.
---@param actor_type string
---@param options ShipLuaActorSpawnOptions
---@return ShipLuaActorHandle? value
---@return ShipLuaOperationError? error
function ship.actor.spawn(actor_type, options) end

--- API common; estabilidade: experimental; desde: 0.4.0; capability: actor.destroy; erros: invalid_argument, unsupported, permission_denied, invalid_handle, host_failure.
---@param handle ShipLuaActorHandle
---@return boolean? value
---@return ShipLuaOperationError? error
function ship.actor.destroy(handle) end

--- API common; estabilidade: experimental; desde: 0.4.0; capability: actor.exists; erros: invalid_argument, unsupported, permission_denied, host_failure.
---@param handle ShipLuaActorHandle
---@return boolean? value
---@return ShipLuaOperationError? error
function ship.actor.exists(handle) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: world.travel; erros: invalid_argument, unsupported, invalid_state, host_failure.
---@param world ShipLuaGameId
---@param destination string
---@return boolean
function ship.world.travel(world, destination) end

--- API mm; estabilidade: experimental; desde: 0.2.0; capability: mm.player.jump; erros: nenhum.
---@return boolean
function ship.mm.player.jump() end

--- API mm; estabilidade: experimental; desde: 0.3.0; capability: mm.spawn_dog; erros: nenhum.
---@return boolean
function ship.mm.spawn_dog() end

--- API mm; estabilidade: experimental; desde: 0.4.0; capability: mm.player.sword_skin; erros: nenhum.
---@param skin string
---@return boolean
function ship.mm.player.set_sword_skin(skin) end

--- API oot; estabilidade: experimental; desde: 0.3.0; capability: oot.player.jump; erros: nenhum.
---@return boolean
function ship.oot.player.jump() end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: oot.player.bunny_hood; erros: nenhum.
---@param equipped boolean
---@return boolean
function ship.oot.player.set_bunny_hood(equipped) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: oot.player.mask; erros: nenhum.
---@param mask string
---@return boolean
function ship.oot.player.set_mask(mask) end

--- API common; estabilidade: experimental; desde: 0.4.0; capability: player.speed; erros: nenhum.
---@param factor number
---@return boolean
function ship.player.set_speed_multiplier(factor) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: player.fields; erros: nenhum.
---@param field string
---@return any
function ship.player.get(field) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: player.fields; erros: nenhum.
---@param field string
---@param value number
---@return boolean
function ship.player.set(field, value) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: oot.player.attach_model; erros: nenhum.
---@param slot string
---@param path string
---@return boolean
function ship.oot.player.attach_model(slot, path) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: oot.player.immunity; erros: nenhum.
---@param kind string
---@param enabled boolean
---@return boolean
function ship.oot.player.set_damage_immunity(kind, enabled) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: oot.player.weight; erros: nenhum.
---@param weight string
---@return boolean
function ship.oot.player.set_weight(weight) end

--- API oot; estabilidade: experimental; desde: 0.4.0; capability: oot.player.roll; erros: nenhum.
---@param mode string
---@return boolean
function ship.oot.player.set_roll_mode(mode) end

--- API oot; estabilidade: experimental; desde: 0.3.0; capability: oot.spawn_dog; erros: nenhum.
---@return boolean
function ship.oot.spawn_dog() end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_argument.
---@param message string
---@return nil
function ship.log.debug(message) end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_argument.
---@param message string
---@return nil
function ship.log.info(message) end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_argument.
---@param message string
---@return nil
function ship.log.warn(message) end

--- API common; estabilidade: stable; desde: 0.1.0; capability: comum; erros: invalid_argument.
---@param message string
---@return nil
function ship.log.error(message) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.timers; erros: invalid_argument, resource_limit, unsupported.
---@param frames integer
---@param callback function
---@return ShipLuaTimerHandle
function ship.timer.after(frames, callback) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.timers; erros: invalid_argument, resource_limit, unsupported.
---@param frames integer
---@param callback function
---@return ShipLuaTimerHandle
function ship.timer.every(frames, callback) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.timers; erros: invalid_argument, invalid_handle.
---@param handle ShipLuaTimerHandle
---@return boolean
function ship.timer.cancel(handle) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.storage; erros: invalid_argument, unsupported.
---@param key string
---@param default? any
---@return any
function ship.storage.get(key, default) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.storage; erros: invalid_argument, resource_limit, unsupported.
---@param key string
---@param value any
---@return boolean
function ship.storage.set(key, value) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.storage; erros: invalid_argument, unsupported.
---@param key string
---@return boolean
function ship.storage.delete(key) end

--- API common; estabilidade: experimental; desde: 0.3.0; capability: core.storage; erros: unsupported.
---@return integer
function ship.storage.clear() end
