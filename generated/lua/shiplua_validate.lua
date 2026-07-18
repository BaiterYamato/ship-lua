-- Gerado por tools/generate_api_contracts.py. Não edite manualmente.
-- Validadores de argumentos derivados da IDL pública (schema/*.yml).
-- Uso: local validate = require("shiplua_validate")
--   local ok, code, message = validate.validate("ship.events.on", "game.ready", fn)
local M = {}

M.api_version = "0.3.0"

M.types = {
  ["game_id"] = { kind = "enum",
    values = { "oot", "mm" },
  },
  ["subscription"] = { kind = "opaque",
    lua_type = "integer",
  },
  ["timer_handle"] = { kind = "opaque",
    lua_type = "integer",
  },
  ["event_options"] = { kind = "object",
    fields = {
      { name = "priority", type = "integer", required = false },
    },
  },
  ["actor_handle"] = { kind = "object",
    fields = {
      { name = "slot", type = "integer", required = true },
      { name = "generation", type = "integer", required = true },
      { name = "game", type = "game_id", required = true },
    },
  },
  ["actor_snapshot"] = { kind = "object",
    fields = {
      { name = "handle", type = "actor_handle", required = true },
      { name = "actor_id", type = "integer", required = true },
      { name = "category", type = "integer", required = true },
    },
  },
  ["hotkey_options"] = { kind = "object",
    fields = {
      { name = "default", type = "string", required = false },
      { name = "label", type = "string", required = false },
    },
  },
}

M.functions = {
  ["ship.game.id"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {},
    returns = "game_id",
    errors = {},
  },
  ["ship.game.host_version"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {},
    returns = "string",
    errors = {},
  },
  ["ship.runtime.version"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {},
    returns = "string",
    errors = {},
  },
  ["ship.api.version"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {},
    returns = "string",
    errors = {},
  },
  ["ship.capabilities.has"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "name", type = "string", required = true },
    },
    returns = "boolean",
    errors = { "invalid_argument" },
  },
  ["ship.capabilities.list"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {},
    returns = "array<string>",
    errors = {},
  },
  ["ship.events.on"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "event", type = "string", required = true },
      { name = "options_or_callback", type = "any", required = true },
      { name = "callback", type = "callback", required = false },
    },
    returns = "subscription",
    errors = { "invalid_argument", "unsupported" },
    requires_callback = true,
  },
  ["ship.events.off"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "subscription", type = "subscription", required = true },
    },
    returns = "boolean",
    errors = { "invalid_handle" },
  },
  ["ship.hotkeys.register"] = {
    version = "0.2.0",
    stability = "preview",
    availability = "common",
    arguments = {
      { name = "id", type = "string", required = true },
      { name = "options", type = "hotkey_options", required = false },
      { name = "callback", type = "callback", required = true },
    },
    returns = "boolean",
    errors = { "invalid_argument", "unsupported" },
  },
  ["ship.mm.player.jump"] = {
    version = "0.2.0",
    stability = "experimental",
    availability = "mm",
    capability = "mm.player.jump",
    arguments = {},
    returns = "boolean",
    errors = {},
  },
  ["ship.mm.spawn_dog"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "mm",
    capability = "mm.spawn_dog",
    arguments = {},
    returns = "boolean",
    errors = {},
  },
  ["ship.oot.player.jump"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "oot",
    capability = "oot.player.jump",
    arguments = {},
    returns = "boolean",
    errors = {},
  },
  ["ship.oot.spawn_dog"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "oot",
    capability = "oot.spawn_dog",
    arguments = {},
    returns = "boolean",
    errors = {},
  },
  ["ship.log.debug"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "message", type = "string", required = true },
    },
    returns = "nil",
    errors = { "invalid_argument" },
  },
  ["ship.log.info"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "message", type = "string", required = true },
    },
    returns = "nil",
    errors = { "invalid_argument" },
  },
  ["ship.log.warn"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "message", type = "string", required = true },
    },
    returns = "nil",
    errors = { "invalid_argument" },
  },
  ["ship.log.error"] = {
    version = "0.1.0",
    stability = "stable",
    availability = "common",
    arguments = {
      { name = "message", type = "string", required = true },
    },
    returns = "nil",
    errors = { "invalid_argument" },
  },
  ["ship.timer.after"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.timers",
    arguments = {
      { name = "frames", type = "integer", required = true },
      { name = "callback", type = "callback", required = true },
    },
    returns = "timer_handle",
    errors = { "invalid_argument", "resource_limit", "unsupported" },
  },
  ["ship.timer.every"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.timers",
    arguments = {
      { name = "frames", type = "integer", required = true },
      { name = "callback", type = "callback", required = true },
    },
    returns = "timer_handle",
    errors = { "invalid_argument", "resource_limit", "unsupported" },
  },
  ["ship.timer.cancel"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.timers",
    arguments = {
      { name = "handle", type = "timer_handle", required = true },
    },
    returns = "boolean",
    errors = { "invalid_argument", "invalid_handle" },
  },
  ["ship.storage.get"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.storage",
    arguments = {
      { name = "key", type = "string", required = true },
      { name = "default", type = "any", required = false },
    },
    returns = "any",
    errors = { "invalid_argument", "unsupported" },
  },
  ["ship.storage.set"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.storage",
    arguments = {
      { name = "key", type = "string", required = true },
      { name = "value", type = "any", required = true },
    },
    returns = "boolean",
    errors = { "invalid_argument", "resource_limit", "unsupported" },
  },
  ["ship.storage.delete"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.storage",
    arguments = {
      { name = "key", type = "string", required = true },
    },
    returns = "boolean",
    errors = { "invalid_argument", "unsupported" },
  },
  ["ship.storage.clear"] = {
    version = "0.3.0",
    stability = "experimental",
    availability = "common",
    capability = "core.storage",
    arguments = {},
    returns = "integer",
    errors = { "unsupported" },
  },
}

M.events = {
  ["game.ready"] = {
    kind = "observe",
    phase = "mvp",
    cancellable = false,
    support = { "oot", "mm" },
    payload = {
      { name = "game_id", type = "game_id", required = true },
      { name = "host_version", type = "string", required = true },
      { name = "runtime_version", type = "string", required = true },
      { name = "api_version", type = "string", required = true },
    },
  },
  ["game.frame"] = {
    kind = "observe",
    phase = "mvp",
    cancellable = false,
    support = { "oot", "mm" },
    payload = {
      { name = "frame", type = "integer", required = true },
    },
  },
  ["game.shutdown"] = {
    kind = "observe",
    phase = "mvp",
    cancellable = false,
    support = { "oot", "mm" },
  },
  ["scene.enter"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "scene.events",
    payload = {
      { name = "scene_id", type = "integer", required = true },
    },
  },
  ["actor.init"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "actor.events",
    payload = {
      { name = "actor", type = "actor_snapshot", required = true },
    },
  },
  ["actor.update"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "actor.events",
    payload = {
      { name = "actor", type = "actor_snapshot", required = true },
    },
  },
  ["actor.destroy"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "actor.events",
    payload = {
      { name = "handle", type = "actor_handle", required = true },
    },
  },
  ["save.loaded"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "save.events",
    payload = {
      { name = "slot", type = "integer", required = true },
    },
  },
  ["text.open"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "text.events",
    payload = {
      { name = "text_id", type = "integer", required = true },
    },
  },
  ["audio.sequence_started"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    capability = "audio.sequence.events",
    payload = {
      { name = "player_index", type = "integer", required = true },
      { name = "sequence_id", type = "integer", required = true },
    },
  },
  ["input.hotkey"] = {
    kind = "observe",
    phase = "host_bridge",
    cancellable = false,
    support = { "oot", "mm" },
    payload = {
      { name = "action", type = "string", required = true },
      { name = "key", type = "string", required = true },
    },
  },
}

local function builtin_check(type_name, value)
  if type_name == "any" then return true end
  if type_name == "boolean" then return type(value) == "boolean" end
  if type_name == "string" then return type(value) == "string" end
  if type_name == "callback" then return type(value) == "function" end
  if type_name == "number" then return type(value) == "number" end
  if type_name == "integer" then return type(value) == "number" and math.type(value) == "integer" end
  if type_name == "nil" then return value == nil end
  return nil
end

-- Verifica um valor contra um tipo da IDL. Retorna true ou false + nome esperado.
function M.check_type(type_name, value)
  if type_name:match("^array<.+>$") then
    if type(value) ~= "table" then return false, type_name end
    return true
  end
  local builtin = builtin_check(type_name, value)
  if builtin ~= nil then
    if builtin then return true end
    return false, type_name
  end
  local spec = M.types[type_name]
  if spec == nil then return false, type_name end
  if spec.kind == "enum" then
    if type(value) ~= "string" then return false, type_name end
    for _, allowed in ipairs(spec.values) do
      if value == allowed then return true end
    end
    return false, type_name
  end
  if spec.kind == "opaque" then
    local ok = M.check_type(spec.lua_type, value)
    if not ok then return false, type_name end
    return true
  end
  if type(value) ~= "table" then return false, type_name end
  for _, field in ipairs(spec.fields or {}) do
    local field_value = value[field.name]
    if field_value == nil then
      if field.required then return false, type_name .. "." .. field.name end
    else
      local ok, expected = M.check_type(field.type, field_value)
      if not ok then return false, expected end
    end
  end
  return true
end

-- Valida os argumentos de uma função da IDL.
-- Retorna true, ou false + código de erro estruturado + mensagem.
function M.validate(name, ...)
  local spec = M.functions[name]
  if spec == nil then
    return false, "unsupported", "função desconhecida: " .. tostring(name)
  end
  local args = table.pack(...)
  for index, argument in ipairs(spec.arguments) do
    local value = args[index]
    if value == nil then
      if argument.required then
        return false, "invalid_argument",
          name .. ": argumento obrigatório ausente '" .. argument.name .. "'"
      end
    else
      local ok, expected = M.check_type(argument.type, value)
      if not ok then
        return false, "invalid_argument",
          name .. ": argumento '" .. argument.name .. "' espera " .. expected
      end
    end
  end
  if spec.requires_callback then
    local found = false
    for index = 1, args.n do
      if type(args[index]) == "function" then
        found = true
        break
      end
    end
    if not found then
      return false, "invalid_argument", name .. ": exige um callback"
    end
  end
  return true
end

return M
