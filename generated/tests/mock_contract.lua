-- Gerado por tools/generate_api_contracts.py. Não edite manualmente.
-- Testes de contrato do mock runtime e dos validadores, executados em um
-- lua_State bruto por tests/contract/MockRuntimeTests.cpp. Derivados da IDL.
local validate = require("shiplua_validate")
local mock = require("shiplua_mock")

local failures = {}
local function check(condition, message)
  if not condition then failures[#failures + 1] = message end
end

local function resolve(root, path)
  local node = root
  for part in path:gmatch("[^%.]+") do
    if type(node) ~= "table" then return nil end
    node = node[part]
    if node == nil then return nil end
  end
  return node
end

local function check_return(name, expect, value)
  if expect == "nil" then
    check(value == nil, name .. " deveria retornar nil")
    return
  end
  if expect == "api_version" then
    check(value == validate.api_version, name .. " deveria retornar a versão da IDL")
    return
  end
  local enum_name = expect:match("^enum:(.+)$")
  if enum_name then
    local spec = validate.types[enum_name]
    if type(value) ~= "string" then
      check(false, name .. " deveria retornar string (" .. enum_name .. ")")
      return
    end
    for _, allowed in ipairs(spec.values) do
      if value == allowed then return end
    end
    check(false, name .. " retornou valor fora do enum " .. enum_name)
    return
  end
  if expect == "integer" then
    check(type(value) == "number" and math.type(value) == "integer",
      name .. " deveria retornar integer")
    return
  end
  if expect == "number" then
    check(type(value) == "number", name .. " deveria retornar number")
    return
  end
  if expect == "table" then
    check(type(value) == "table", name .. " deveria retornar tabela")
    return
  end
  check(type(value) == expect, name .. " deveria retornar " .. expect)
end

local ship_mod

local function probe(name, args, expect)
  local fn = resolve(ship_mod, (name:gsub("^ship%.", "")))
  check(type(fn) == "function", name .. " ausente no mock (host " .. mock.host.game .. ")")
  if type(fn) ~= "function" then return end
  local outcome = table.pack(pcall(fn, table.unpack(args)))
  check(outcome[1], name .. " falhou com argumentos válidos: " .. tostring(outcome[2]))
  if outcome[1] then check_return(name, expect, outcome[2]) end
end

local function expected_available(spec, game, granted)
  if spec.availability == "common" then return true end
  if spec.availability ~= game then return false end
  if spec.capability ~= nil then
    for _, capability in ipairs(granted) do
      if capability == spec.capability then return true end
    end
    return false
  end
  return true
end

-- Validadores: função desconhecida e argumento obrigatório ausente.
do
  local ok, code = validate.validate("ship.unknown.fn", 1)
  check(not ok and code == "unsupported", "função desconhecida deveria retornar unsupported")
  local ok_missing, code_missing = validate.validate("ship.capabilities.has")
  check(not ok_missing and code_missing == "invalid_argument",
    "argumento obrigatório ausente deveria retornar invalid_argument")
  check(validate.validate("ship.capabilities.has", "scene.events") == true,
    "argumentos válidos deveriam ser aceitos")
  local ok_wrong = validate.validate("ship.capabilities.has", 42)
  check(not ok_wrong, "tipo errado de argumento deveria ser rejeitado")
end

-- Validadores: tipos enum, opacos e arrays conforme a IDL.
check(validate.check_type("game_id", "oot") == true,
  "enum game_id deveria aceitar oot")
check(select(1, validate.check_type("game_id", "__invalid__")) == false,
  "enum game_id deveria rejeitar valor fora do contrato")
check(validate.check_type("subscription", 1) == true, "opaco subscription deveria aceitar integer")
check(validate.check_type("timer_handle", 1) == true, "opaco timer_handle deveria aceitar integer")
check(validate.check_type("array<string>", {}) == true, "array deveria aceitar tabela")
check(select(1, validate.check_type("array<string>", "não-tabela")) == false,
  "array deveria rejeitar escalar")

-- Sobrecarga '<opções ou callback>': ao menos um callback é exigido.
check(validate.validate("ship.events.on", "probe", function() end) == true,
  "ship.events.on com callback deveria validar")
check(select(1, validate.validate("ship.events.on", "probe", {})) == false,
  "ship.events.on sem callback deveria falhar")

-- Matriz de disponibilidade: superfície do mock casa com a IDL nos dois hosts.
for _, game in ipairs({ "oot", "mm" }) do
  mock.configure({ game = game })
  ship_mod = mock.build()
  for name, spec in pairs(validate.functions) do
    local present = resolve(ship_mod, (name:gsub("^ship%.", ""))) ~= nil
    check(present == expected_available(spec, game, mock.host.capabilities),
      name .. ": disponibilidade diverge do contrato no host " .. game)
  end
end

-- Sondas de chamada válida no host OoT (funções common).
mock.configure({ game = "oot" })
ship_mod = mock.build()
probe("ship.game.id", {}, "enum:game_id")
probe("ship.game.host_version", {}, "string")
probe("ship.runtime.version", {}, "string")
probe("ship.api.version", {}, "api_version")
probe("ship.capabilities.has", { "core.events" }, "boolean")
probe("ship.capabilities.list", {}, "table")
probe("ship.events.on", { "game.ready", function() end }, "integer")
probe("ship.hotkeys.register", { "contract.probe", { default = "F5" }, function() end }, "boolean")
probe("ship.world.travel", { "oot", "probe" }, "boolean")
probe("ship.log.debug", { "contract probe" }, "nil")
probe("ship.log.info", { "contract probe" }, "nil")
probe("ship.log.warn", { "contract probe" }, "nil")
probe("ship.log.error", { "contract probe" }, "nil")
probe("ship.timer.after", { 1, function() end }, "integer")
probe("ship.timer.every", { 1, function() end }, "integer")
probe("ship.timer.cancel", { 1 }, "boolean")
probe("ship.storage.set", { "probe", "probe" }, "boolean")
probe("ship.storage.delete", { "probe" }, "boolean")
probe("ship.storage.clear", {  }, "integer")
check(resolve(ship_mod, "mm.player.jump") == nil, "ship.mm.player.jump não deveria existir no host oot")
check(resolve(ship_mod, "mm.spawn_dog") == nil, "ship.mm.spawn_dog não deveria existir no host oot")

-- Sondas de chamada válida no host OOT (funções do adaptador).
mock.configure({ game = "oot" })
ship_mod = mock.build()
probe("ship.oot.player.jump", {  }, "boolean")
probe("ship.oot.spawn_dog", {  }, "boolean")

-- Sondas de chamada válida no host MM (funções do adaptador).
mock.configure({ game = "mm" })
ship_mod = mock.build()
probe("ship.mm.player.jump", {  }, "boolean")
probe("ship.mm.spawn_dog", {  }, "boolean")

-- Gating por capability: sem o grant, o binding do adaptador não é instalado.
mock.configure({ game = "mm", capabilities = {} })
ship_mod = mock.build()
check(resolve(ship_mod, "mm.player.jump") == nil, "ship.mm.player.jump sem capability não deveria existir")
mock.configure({ game = "mm", capabilities = {} })
ship_mod = mock.build()
check(resolve(ship_mod, "mm.spawn_dog") == nil, "ship.mm.spawn_dog sem capability não deveria existir")
mock.configure({ game = "oot", capabilities = {} })
ship_mod = mock.build()
check(resolve(ship_mod, "oot.player.jump") == nil, "ship.oot.player.jump sem capability não deveria existir")
mock.configure({ game = "oot", capabilities = {} })
ship_mod = mock.build()
check(resolve(ship_mod, "oot.spawn_dog") == nil, "ship.oot.spawn_dog sem capability não deveria existir")

-- Erros estruturados: chamada sem argumentos obrigatórios falha com código.
mock.configure({ game = "oot" })
ship_mod = mock.build()
do
  local fn = resolve(ship_mod, "capabilities.has")
  local ok, err = pcall(fn)
  check(not ok, "ship.capabilities.has sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.capabilities.has deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "events.on")
  local ok, err = pcall(fn)
  check(not ok, "ship.events.on sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.events.on deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "events.off")
  local ok, err = pcall(fn)
  check(not ok, "ship.events.off sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.events.off deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "hotkeys.register")
  local ok, err = pcall(fn)
  check(not ok, "ship.hotkeys.register sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.hotkeys.register deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "actor.spawn")
  local value, err = fn()
  check(value == nil, "ship.actor.spawn sem argumentos não deveria retornar valor")
  check(type(err) == "table" and err.code == "invalid_argument",
    "erro de ship.actor.spawn deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "actor.destroy")
  local value, err = fn()
  check(value == nil, "ship.actor.destroy sem argumentos não deveria retornar valor")
  check(type(err) == "table" and err.code == "invalid_argument",
    "erro de ship.actor.destroy deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "actor.exists")
  local value, err = fn()
  check(value == nil, "ship.actor.exists sem argumentos não deveria retornar valor")
  check(type(err) == "table" and err.code == "invalid_argument",
    "erro de ship.actor.exists deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "world.travel")
  local ok, err = pcall(fn)
  check(not ok, "ship.world.travel sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.world.travel deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "log.debug")
  local ok, err = pcall(fn)
  check(not ok, "ship.log.debug sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.log.debug deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "log.info")
  local ok, err = pcall(fn)
  check(not ok, "ship.log.info sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.log.info deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "log.warn")
  local ok, err = pcall(fn)
  check(not ok, "ship.log.warn sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.log.warn deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "log.error")
  local ok, err = pcall(fn)
  check(not ok, "ship.log.error sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.log.error deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "timer.after")
  local ok, err = pcall(fn)
  check(not ok, "ship.timer.after sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.timer.after deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "timer.every")
  local ok, err = pcall(fn)
  check(not ok, "ship.timer.every sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.timer.every deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "timer.cancel")
  local ok, err = pcall(fn)
  check(not ok, "ship.timer.cancel sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.timer.cancel deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "storage.get")
  local ok, err = pcall(fn)
  check(not ok, "ship.storage.get sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.storage.get deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "storage.set")
  local ok, err = pcall(fn)
  check(not ok, "ship.storage.set sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.storage.set deveria carregar invalid_argument")
end
do
  local fn = resolve(ship_mod, "storage.delete")
  local ok, err = pcall(fn)
  check(not ok, "ship.storage.delete sem argumentos deveria falhar")
  check(type(err) == "string" and err:find("invalid_argument", 1, true) ~= nil,
    "erro de ship.storage.delete deveria carregar invalid_argument")
end

-- Semântica de infraestrutura do mock.
mock.configure({ game = "oot" })
ship_mod = mock.build()
check(ship_mod.game.id() == "oot", "ship.game.id do mock deveria refletir o host")
check(ship_mod.api.version() == validate.api_version,
  "ship.api.version do mock deveria casar com a IDL")
check(ship_mod.capabilities.has("core.events") == true,
  "capability contract do host deveria ser detectável no mock")
check(ship_mod.capabilities.has("contract.missing") == false,
  "capability desconhecida deveria retornar false no mock")

-- Fluxo de eventos: inscrição, dispatch determinístico, cancelamento.
mock.reset()
local fired = 0
local subscription = ship_mod.events.on("game.ready", function() fired = fired + 1 end)
mock.emit("game.ready", {})
check(fired == 1, "mock.emit deveria disparar o callback inscrito")
check(ship_mod.events.off(subscription) == true,
  "cancelamento de inscrição válida deveria confirmar")
mock.emit("game.ready", {})
check(fired == 1, "callback removido não deveria disparar")
check(not pcall(ship_mod.events.on, "contract.unknown", function() end),
  "evento desconhecido deveria falhar no mock")
check(#mock.calls == 3, "chamadas da API deveriam ser registradas em ordem")
check(mock.calls[1].name == "ship.events.on",
  "primeira chamada registrada deveria ser ship.events.on")

-- Hotkeys: registro retorna true e retém o callback.
check(ship_mod.hotkeys.register("contract.probe",
    { default = "F5", label = "probe" }, function() end) == true,
  "registro de hotkey deveria confirmar no mock")
check(type(mock.hotkeys["contract.probe"]) == "function",
  "callback de hotkey deveria ser retido pelo mock")

-- Reset limpa o histórico de chamadas.
mock.reset()
check(#mock.calls == 0, "reset deveria limpar as chamadas registradas")

if #failures > 0 then
  error("mock contract failures (" .. #failures .. "):\n" .. table.concat(failures, "\n"), 0)
end
print("mock contract OK")
