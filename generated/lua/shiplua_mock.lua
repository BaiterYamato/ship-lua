-- Gerado por tools/generate_api_contracts.py. Não edite manualmente.
-- Mock runtime da API pública ShipLua, derivado da IDL (schema/*.yml).
-- Não executa lógica de jogo: valida argumentos conforme a IDL, registra
-- chamadas e retorna valores determinísticos compatíveis com o contrato.
-- Erros são erros Lua cujo texto carrega o código estruturado: "<code>: <msg>".
local validate = require("shiplua_validate")

local M = {}

M.contract_capabilities = {
  oot = { "core.events", "core.timers", "core.input", "core.storage", "scene.events", "actor.events", "actor.spawn", "actor.destroy", "actor.exists", "save.events", "text.events", "audio.sequence.events", "world.travel", "oot.player.jump", "oot.spawn_dog" },
  mm = { "core.events", "core.timers", "core.input", "core.storage", "scene.events", "actor.events", "actor.spawn", "actor.destroy", "actor.exists", "save.events", "text.events", "audio.sequence.events", "world.travel", "mm.player.jump", "mm.spawn_dog", "mm.player.sword_skin" },
}

M.host = {
  game = "oot",
  host_version = "mock-host 0.0.0",
  runtime_version = "0.0.0-mock",
  capabilities = nil, -- nil = capabilities contract do host configurado
}

M.calls = {}
M.subscriptions = {}
M.hotkeys = {}
M.next_subscription = 0

local function copy_list(values)
  local out = {}
  for index, value in ipairs(values) do out[index] = value end
  return out
end

function M.configure(options)
  options = options or {}
  if options.game ~= nil then
    assert(M.contract_capabilities[options.game] ~= nil,
      "host desconhecido: " .. tostring(options.game))
    M.host.game = options.game
  end
  if options.host_version ~= nil then M.host.host_version = options.host_version end
  if options.runtime_version ~= nil then M.host.runtime_version = options.runtime_version end
  M.host.capabilities = options.capabilities ~= nil
    and copy_list(options.capabilities)
    or copy_list(M.contract_capabilities[M.host.game])
  return M
end

function M.reset()
  M.calls = {}
  M.subscriptions = {}
  M.hotkeys = {}
  M.next_subscription = 0
end

local function host_has(capability)
  for _, granted in ipairs(M.host.capabilities) do
    if granted == capability then return true end
  end
  return false
end

-- Disponibilidade derivada da IDL: funções 'common' existem sempre; funções
-- específicas de host são instaladas pelo adaptador quando o host anuncia
-- a capability correspondente (mesma regra de ValidateHostContext do núcleo).
local function available_on(spec)
  if spec.availability == "common" then return true end
  if spec.availability ~= M.host.game then return false end
  if spec.capability ~= nil then return host_has(spec.capability) end
  return true
end

local function default_return(type_name)
  if type_name == "boolean" then return true end
  if type_name == "integer" or type_name == "number" then return 0 end
  if type_name == "string" then return "" end
  if type_name == "nil" then return nil end
  if type_name:match("^array<.+>$") then return {} end
  local spec = validate.types[type_name]
  if spec ~= nil and spec.kind == "enum" then return spec.values[1] end
  if spec ~= nil and spec.kind == "opaque" then return default_return(spec.lua_type) end
  return {}
end

-- Semântica mínima das funções de infraestrutura, dirigida pelos dados da IDL.
local handlers = {}

handlers["ship.game.id"] = function() return M.host.game end
handlers["ship.game.host_version"] = function() return M.host.host_version end
handlers["ship.runtime.version"] = function() return M.host.runtime_version end
handlers["ship.api.version"] = function() return validate.api_version end

handlers["ship.capabilities.has"] = function(name) return host_has(name) end
handlers["ship.capabilities.list"] = function() return copy_list(M.host.capabilities) end

handlers["ship.events.on"] = function(event, options_or_callback, callback)
  if validate.events[event] == nil then
    error("unsupported: evento desconhecido '" .. tostring(event) .. "'", 2)
  end
  local fn = callback or options_or_callback
  M.next_subscription = M.next_subscription + 1
  M.subscriptions[M.next_subscription] = { event = event, callback = fn }
  return M.next_subscription
end

handlers["ship.events.off"] = function(subscription)
  if M.subscriptions[subscription] == nil then
    error("invalid_handle: inscrição desconhecida", 2)
  end
  M.subscriptions[subscription] = nil
  return true
end

handlers["ship.hotkeys.register"] = function(id, options, callback)
  M.hotkeys[id] = callback
  return true
end

local function make_stub(name, spec)
  return function(...)
    local ok, code, message = validate.validate(name, ...)
    if not ok then
      if spec.error_mode == "return" then
        return nil, { code = code, message = message }
      end
      error(code .. ": " .. message, 2)
    end
    local args = table.pack(...)
    args.n = nil
    M.calls[#M.calls + 1] = { name = name, arguments = args }
    local handler = handlers[name]
    if handler ~= nil then return handler(...) end
    local value = default_return(spec.returns)
    if spec.error_mode == "return" then return value, nil end
    return value
  end
end

local function set_dotted(root, path, value)
  local node = root
  local parts = {}
  for part in path:gmatch("[^%.]+") do parts[#parts + 1] = part end
  for index = 1, #parts - 1 do
    local part = parts[index]
    if node[part] == nil then node[part] = {} end
    node = node[part]
  end
  node[parts[#parts]] = value
end

-- Constrói o módulo `ship` para o host configurado.
function M.build()
  if M.host.capabilities == nil then M.configure({}) end
  local ship = {}
  for name, spec in pairs(validate.functions) do
    if available_on(spec) then
      set_dotted(ship, (name:gsub("^ship%.", "")), make_stub(name, spec))
    end
  end
  return ship
end

-- Dispara um evento para as inscrições do mock, em ordem determinística.
function M.emit(event, payload)
  for id = 1, M.next_subscription do
    local subscription = M.subscriptions[id]
    if subscription ~= nil and subscription.event == event then
      subscription.callback(payload or {})
    end
  end
end

M.configure({})
M.reset()

return M
