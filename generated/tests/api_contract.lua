-- Gerado por tools/generate_api_contracts.py. Não edite manualmente.
-- Testes de contrato da API pública, executados dentro do runtime real
-- (ModHost) em contextos OoT e MM por tests/contract/ApiContractTests.cpp.
-- Derivados da IDL (schema/*.yml); qualquer falha aborta a carga do mod.
local ship = require("ship")

local API_VERSION = "0.3.0"

local FUNCTIONS = {
  { name = "ship.game.id", availability = "common", required_arguments = 0 },
  { name = "ship.game.host_version", availability = "common", required_arguments = 0 },
  { name = "ship.runtime.version", availability = "common", required_arguments = 0 },
  { name = "ship.api.version", availability = "common", required_arguments = 0 },
  { name = "ship.capabilities.has", availability = "common", required_arguments = 1 },
  { name = "ship.capabilities.list", availability = "common", required_arguments = 0 },
  { name = "ship.events.on", availability = "common", required_arguments = 2 },
  { name = "ship.events.off", availability = "common", required_arguments = 1 },
  { name = "ship.hotkeys.register", availability = "common", required_arguments = 2 },
  { name = "ship.mm.player.jump", availability = "mm", required_arguments = 0 },
  { name = "ship.mm.spawn_dog", availability = "mm", required_arguments = 0 },
  { name = "ship.oot.player.jump", availability = "oot", required_arguments = 0 },
  { name = "ship.oot.spawn_dog", availability = "oot", required_arguments = 0 },
  { name = "ship.log.debug", availability = "common", required_arguments = 1 },
  { name = "ship.log.info", availability = "common", required_arguments = 1 },
  { name = "ship.log.warn", availability = "common", required_arguments = 1 },
  { name = "ship.log.error", availability = "common", required_arguments = 1 },
}

local EVENTS = { "game.ready", "game.frame", "game.shutdown", "scene.enter", "actor.init", "actor.update", "actor.destroy", "save.loaded", "text.open", "audio.sequence_started", "input.hotkey" }

local ENUM_VALUES = {
  game_id = { "oot", "mm" },
}

local CONTRACT_CAPABILITIES = {
  oot = { "scene.events", "actor.events", "save.events", "text.events", "audio.sequence.events", "oot.player.jump", "oot.spawn_dog" },
  mm = { "scene.events", "actor.events", "save.events", "text.events", "audio.sequence.events", "mm.player.jump", "mm.spawn_dog" },
}

local failures = {}
local function check(condition, message)
  if not condition then failures[#failures + 1] = message end
end

local function resolve(path)
  local node = ship
  for part in path:gmatch("[^%.]+") do
    if type(node) ~= "table" then return nil end
    node = node[part]
    if node == nil then return nil end
  end
  return node
end

local function check_return_type(name, type_name, value)
  if type_name:match("^array<.+>$") then
    check(type(value) == "table", name .. " deveria retornar tabela (" .. type_name .. ")")
    return
  end
  local enum_values = ENUM_VALUES[type_name]
  if enum_values ~= nil then
    if type(value) ~= "string" then
      check(false, name .. " deveria retornar string (" .. type_name .. ")")
      return
    end
    for _, allowed in ipairs(enum_values) do
      if value == allowed then return end
    end
    check(false, name .. " retornou valor fora do enum " .. type_name)
    return
  end
  if type_name == "string" then
    check(type(value) == "string", name .. " deveria retornar string")
  elseif type_name == "boolean" then
    check(type(value) == "boolean", name .. " deveria retornar boolean")
  elseif type_name == "integer" or type_name == "number" then
    check(type(value) == "number", name .. " deveria retornar number")
  elseif type_name == "nil" then
    check(value == nil, name .. " deveria retornar nil")
  else
    check(type(value) == "table" or type(value) == "number",
      name .. " deveria retornar tabela ou handle (" .. type_name .. ")")
  end
end

local host = ship.game.id()

-- Superfície: funções 'common' existem no núcleo; funções específicas de host
-- são instaladas pelos adaptadores de host, nunca pelo núcleo (RFC 0001).
for _, spec in ipairs(FUNCTIONS) do
  local fn = resolve((spec.name:gsub("^ship%.", "")))
  if spec.availability == "common" then
    check(type(fn) == "function", spec.name .. " ausente no núcleo")
  else
    check(fn == nil, spec.name .. " não deveria estar no núcleo " ..
      "(adaptador " .. spec.availability .. ")")
  end
end

-- Aridade: função com argumento obrigatório falha de forma controlada
-- (erro Lua, nunca crash) quando chamada sem argumentos.
for _, spec in ipairs(FUNCTIONS) do
  local fn = resolve((spec.name:gsub("^ship%.", "")))
  if spec.availability == "common" and spec.required_arguments > 0
      and type(fn) == "function" then
    check(not pcall(fn), spec.name .. " deveria falhar sem argumentos obrigatórios")
  end
end

-- Sondas de retorno derivadas dos tipos da IDL.
check(ship.game.id() == host, "ship.game.id() deveria igualar o host do contexto")
check(type(ship.game.host_version()) == "string", "ship.game.host_version() deveria retornar string")
check(type(ship.runtime.version()) == "string", "ship.runtime.version() deveria retornar string")
check(ship.api.version() == API_VERSION, "ship.api.version() diverge da IDL")
check(pcall(ship.log.debug, "contract probe"), "ship.log.debug deveria aceitar mensagem textual")
check(pcall(ship.log.info, "contract probe"), "ship.log.info deveria aceitar mensagem textual")
check(pcall(ship.log.warn, "contract probe"), "ship.log.warn deveria aceitar mensagem textual")
check(pcall(ship.log.error, "contract probe"), "ship.log.error deveria aceitar mensagem textual")

-- Capabilities: feature detection reflete o contexto anunciado pelo host.
local granted = CONTRACT_CAPABILITIES[host] or {}
for _, capability in ipairs(granted) do
  check(ship.capabilities.has(capability) == true,
    "capability contratada ausente no host " .. host .. ": " .. capability)
end
check(ship.capabilities.has("contract.missing") == false,
  "capability desconhecida deveria retornar false")
local listed = ship.capabilities.list()
check(type(listed) == "table", "ship.capabilities.list deveria retornar tabela")
if type(listed) == "table" then
  for _, capability in ipairs(listed) do
    check(ship.capabilities.has(capability) == true,
      "capability listada deveria ser detectável: " .. tostring(capability))
  end
end

-- Eventos: toda entrada da IDL aceita inscrição e cancelamento; nomes
-- desconhecidos e chamadas sem callback são rejeitados.
for _, event in ipairs(EVENTS) do
  local subscription = ship.events.on(event, function() end)
  check(type(subscription) == "number",
    "inscrição em " .. event .. " deveria retornar inteiro")
  if type(subscription) == "number" then
    check(ship.events.off(subscription) == true,
      "cancelamento de " .. event .. " deveria confirmar")
  end
end
check(not pcall(ship.events.on, "contract.unknown", function() end),
  "evento desconhecido deveria ser rejeitado")
check(not pcall(ship.events.on, EVENTS[1]),
  "ship.events.on sem callback deveria falhar")
check(not pcall(ship.events.off, 0),
  "ship.events.off com inscrição inválida deveria falhar")

-- Hotkeys: registro com opções, forma curta e rejeição de id inválido.
-- O contexto de teste sempre fornece um registry (host com suporte).
check(ship.hotkeys.register("contract.probe",
    { default = "F5", label = "probe" }, function() end) == true,
  "registro de hotkey com opções deveria confirmar")
check(ship.hotkeys.register("contract.probe_shorthand", function() end) == true,
  "registro de hotkey sem opções deveria confirmar")
check(not pcall(ship.hotkeys.register, "Invalid ID", function() end),
  "id de hotkey inválido deveria ser rejeitado")

if #failures > 0 then
  error("contract failures (" .. #failures .. "):\n" .. table.concat(failures, "\n"), 0)
end
