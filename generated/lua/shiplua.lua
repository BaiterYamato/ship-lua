---@meta ShipLua
-- Gerado por tools/generate_api_docs.py. Não edite manualmente.
-- API 0.3.0 / schema 1

--- Identificador público do jogo host.
---@alias ShipLuaGameId "oot"|"mm"

--- Identificador opaco de inscrição em evento.
---@alias ShipLuaSubscription integer

--- Opções determinísticas de inscrição.
---@class ShipLuaEventOptions
---@field priority? integer

--- Handle validado por slot, geração e host.
---@class ShipLuaActorHandle
---@field slot integer
---@field generation integer
---@field game ShipLuaGameId

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
ship.api = ship.api or {}
ship.capabilities = ship.capabilities or {}
ship.events = ship.events or {}
ship.game = ship.game or {}
ship.hotkeys = ship.hotkeys or {}
ship.log = ship.log or {}
ship.mm = ship.mm or {}
ship.oot = ship.oot or {}
ship.runtime = ship.runtime or {}
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

--- API mm; estabilidade: experimental; desde: 0.2.0; capability: mm.player.jump; erros: nenhum.
---@return boolean
function ship.mm.player.jump() end

--- API mm; estabilidade: experimental; desde: 0.3.0; capability: mm.spawn_dog; erros: nenhum.
---@return boolean
function ship.mm.spawn_dog() end

--- API oot; estabilidade: experimental; desde: 0.3.0; capability: oot.player.jump; erros: nenhum.
---@return boolean
function ship.oot.player.jump() end

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
