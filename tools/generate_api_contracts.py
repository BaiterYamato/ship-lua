#!/usr/bin/env python3
"""Gera validadores, mock runtime, testes de contrato e matriz de compatibilidade.

Consome os schemas canônicos (schema/api.yml, schema/events.yml,
schema/capabilities.yml) — a IDL declarativa da API pública ShipLua — e
materializa os artefatos previstos no plan-sdk.md §3.6 que ainda não tinham
gerador próprio:

- validadores Lua (generated/lua/shiplua_validate.lua);
- mock runtime Lua (generated/lua/shiplua_mock.lua);
- testes de contrato executados dentro do runtime real
  (generated/tests/api_contract.lua, carregado por tests/contract/ApiContractTests.cpp);
- testes de contrato do mock e dos validadores
  (generated/tests/mock_contract.lua, executado por tests/contract/MockRuntimeTests.cpp);
- matriz de compatibilidade OoT/MM (generated/docs/compatibility-matrix.md).

Bindings C++ (tools/generate_cpp_api.py) e documentação de referência
(tools/generate_api_docs.py) já possuem geradores próprios; este arquivo não
duplica essas responsabilidades. Com --check, falha quando algum artefato
diverge dos schemas (gate de drift registrado no ctest).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
from validate_api_schemas import load_document, validate_documents  # noqa: E402

HEADER_LUA = "-- Gerado por tools/generate_api_contracts.py. Não edite manualmente."
HEADER_MD = "<!-- Gerado por tools/generate_api_contracts.py. Não edite manualmente. -->"
HOSTS = ("oot", "mm")


def q(value: str) -> str:
    """Literal Lua para uma string (JSON é subconjunto de Lua 5.4 para strings)."""
    return json.dumps(value, ensure_ascii=False)


def qlist(values: list[str]) -> str:
    if not values:
        return "{}"
    return "{ " + ", ".join(q(value) for value in values) + " }"


def dotted(path: str) -> str:
    """Remove o prefixo 'ship.' de um nome pontuado da IDL."""
    return path[len("ship."):] if path.startswith("ship.") else path


def contract_capabilities(capabilities: dict[str, Any]) -> dict[str, list[str]]:
    """Capabilities com status 'contract' suportadas por cada host, na ordem do schema."""
    per_host: dict[str, list[str]] = {host: [] for host in HOSTS}
    for capability in capabilities["capabilities"]:
        if capability["status"] != "contract":
            continue
        for host in capability["hosts"]:
            per_host[host].append(capability["name"])
    return per_host


def requires_callback(function: dict[str, Any]) -> bool:
    """Funções com sobrecarga '<opções ou callback>' exigem ao menos um callback."""
    types = {argument["type"] for argument in function.get("arguments", [])}
    return "any" in types and "callback" in types


def type_spec_lua(item: dict[str, Any]) -> list[str]:
    kind = item["kind"]
    lines = [f"  [{q(item['name'])}] = {{ kind = {q(kind)},"]
    if kind == "enum":
        lines.append(f"    values = {qlist(item['values'])},")
    elif kind == "opaque":
        lines.append(f"    lua_type = {q(item['lua_type'])},")
    elif kind == "object":
        fields = item.get("fields", [])
        if fields:
            lines.append("    fields = {")
            for field in fields:
                required = "true" if field.get("required", True) else "false"
                lines.append(
                    f"      {{ name = {q(field['name'])}, type = {q(field['type'])}, "
                    f"required = {required} }},")
            lines.append("    },")
    lines.append("  },")
    return lines


def function_spec_lua(function: dict[str, Any]) -> list[str]:
    lines = [
        f"  [{q(function['name'])}] = {{",
        f"    version = {q(function['version'])},",
        f"    stability = {q(function['stability'])},",
        f"    availability = {q(function['availability'])},",
    ]
    if function.get("capability"):
        lines.append(f"    capability = {q(function['capability'])},")
    arguments = function.get("arguments", [])
    if arguments:
        lines.append("    arguments = {")
        for argument in arguments:
            required = "true" if argument.get("required", True) else "false"
            lines.append(
                f"      {{ name = {q(argument['name'])}, type = {q(argument['type'])}, "
                f"required = {required} }},")
        lines.append("    },")
    else:
        lines.append("    arguments = {},")
    lines.append(f"    returns = {q(function['returns'])},")
    lines.append(f"    error_mode = {q(function.get('error_mode', 'raise'))},")
    if function.get("error_type"):
        lines.append(f"    error_type = {q(function['error_type'])},")
    lines.append(f"    errors = {qlist(function.get('errors', []))},")
    if requires_callback(function):
        lines.append("    requires_callback = true,")
    lines.append("  },")
    return lines


def event_spec_lua(event: dict[str, Any]) -> list[str]:
    cancellable = "true" if event["cancellable"] else "false"
    lines = [
        f"  [{q(event['name'])}] = {{",
        f"    kind = {q(event['kind'])},",
        f"    phase = {q(event['phase'])},",
        f"    cancellable = {cancellable},",
        f"    support = {qlist(event['support'])},",
    ]
    if event.get("capability"):
        lines.append(f"    capability = {q(event['capability'])},")
    payload = event.get("payload", [])
    if payload:
        lines.append("    payload = {")
        for field in payload:
            required = "true" if field.get("required", True) else "false"
            lines.append(
                f"      {{ name = {q(field['name'])}, type = {q(field['type'])}, "
                f"required = {required} }},")
        lines.append("    },")
    lines.append("  },")
    return lines


def capabilities_lua(per_host: dict[str, list[str]]) -> list[str]:
    lines = ["M.contract_capabilities = {"]
    for host in HOSTS:
        lines.append(f"  {host} = {qlist(per_host[host])},")
    lines.append("}")
    return lines


def generate_validate_lua(api: dict[str, Any], events: dict[str, Any],
                          capabilities: dict[str, Any]) -> str:
    lines = [
        HEADER_LUA,
        "-- Validadores de argumentos derivados da IDL pública (schema/*.yml).",
        "-- Uso: local validate = require(\"shiplua_validate\")",
        "--   local ok, code, message = validate.validate(\"ship.events.on\", \"game.ready\", fn)",
        "local M = {}",
        "",
        f"M.api_version = {q(api['api_version'])}",
        "",
        "M.types = {",
    ]
    for item in api["types"]:
        lines.extend(type_spec_lua(item))
    lines.extend(["}", "", "M.functions = {"])
    for function in api["functions"]:
        lines.extend(function_spec_lua(function))
    lines.extend(["}", "", "M.events = {"])
    for event in events["events"]:
        lines.extend(event_spec_lua(event))
    lines.extend([
        "}",
        "",
        "local function builtin_check(type_name, value)",
        "  if type_name == \"any\" then return true end",
        "  if type_name == \"boolean\" then return type(value) == \"boolean\" end",
        "  if type_name == \"string\" then return type(value) == \"string\" end",
        "  if type_name == \"callback\" then return type(value) == \"function\" end",
        "  if type_name == \"number\" then return type(value) == \"number\" end",
        "  if type_name == \"integer\" then return type(value) == \"number\" and math.type(value) == \"integer\" end",
        "  if type_name == \"nil\" then return value == nil end",
        "  return nil",
        "end",
        "",
        "-- Verifica um valor contra um tipo da IDL. Retorna true ou false + nome esperado.",
        "function M.check_type(type_name, value)",
        "  if type_name:match(\"^array<.+>$\") then",
        "    if type(value) ~= \"table\" then return false, type_name end",
        "    return true",
        "  end",
        "  local builtin = builtin_check(type_name, value)",
        "  if builtin ~= nil then",
        "    if builtin then return true end",
        "    return false, type_name",
        "  end",
        "  local spec = M.types[type_name]",
        "  if spec == nil then return false, type_name end",
        "  if spec.kind == \"enum\" then",
        "    if type(value) ~= \"string\" then return false, type_name end",
        "    for _, allowed in ipairs(spec.values) do",
        "      if value == allowed then return true end",
        "    end",
        "    return false, type_name",
        "  end",
        "  if spec.kind == \"opaque\" then",
        "    local ok = M.check_type(spec.lua_type, value)",
        "    if not ok then return false, type_name end",
        "    return true",
        "  end",
        "  if type(value) ~= \"table\" then return false, type_name end",
        "  for _, field in ipairs(spec.fields or {}) do",
        "    local field_value = value[field.name]",
        "    if field_value == nil then",
        "      if field.required then return false, type_name .. \".\" .. field.name end",
        "    else",
        "      local ok, expected = M.check_type(field.type, field_value)",
        "      if not ok then return false, expected end",
        "    end",
        "  end",
        "  return true",
        "end",
        "",
        "-- Valida os argumentos de uma função da IDL.",
        "-- Retorna true, ou false + código de erro estruturado + mensagem.",
        "function M.validate(name, ...)",
        "  local spec = M.functions[name]",
        "  if spec == nil then",
        "    return false, \"unsupported\", \"função desconhecida: \" .. tostring(name)",
        "  end",
        "  local args = table.pack(...)",
        "  for index, argument in ipairs(spec.arguments) do",
        "    local value = args[index]",
        "    if value == nil then",
        "      if argument.required then",
        "        return false, \"invalid_argument\",",
        "          name .. \": argumento obrigatório ausente '\" .. argument.name .. \"'\"",
        "      end",
        "    else",
        "      local ok, expected = M.check_type(argument.type, value)",
        "      if not ok then",
        "        return false, \"invalid_argument\",",
        "          name .. \": argumento '\" .. argument.name .. \"' espera \" .. expected",
        "      end",
        "    end",
        "  end",
        "  if spec.requires_callback then",
        "    local found = false",
        "    for index = 1, args.n do",
        "      if type(args[index]) == \"function\" then",
        "        found = true",
        "        break",
        "      end",
        "    end",
        "    if not found then",
        "      return false, \"invalid_argument\", name .. \": exige um callback\"",
        "    end",
        "  end",
        "  return true",
        "end",
        "",
        "return M",
        "",
    ])
    return "\n".join(lines)


def generate_mock_lua(api: dict[str, Any], events: dict[str, Any],
                      capabilities: dict[str, Any]) -> str:
    per_host = contract_capabilities(capabilities)
    lines = [
        HEADER_LUA,
        "-- Mock runtime da API pública ShipLua, derivado da IDL (schema/*.yml).",
        "-- Não executa lógica de jogo: valida argumentos conforme a IDL, registra",
        "-- chamadas e retorna valores determinísticos compatíveis com o contrato.",
        "-- Erros são erros Lua cujo texto carrega o código estruturado: \"<code>: <msg>\".",
        "local validate = require(\"shiplua_validate\")",
        "",
        "local M = {}",
        "",
    ]
    lines.extend(capabilities_lua(per_host))
    lines.extend([
        "",
        "M.host = {",
        "  game = \"oot\",",
        "  host_version = \"mock-host 0.0.0\",",
        "  runtime_version = \"0.0.0-mock\",",
        "  capabilities = nil, -- nil = capabilities contract do host configurado",
        "}",
        "",
        "M.calls = {}",
        "M.subscriptions = {}",
        "M.hotkeys = {}",
        "M.next_subscription = 0",
        "",
        "local function copy_list(values)",
        "  local out = {}",
        "  for index, value in ipairs(values) do out[index] = value end",
        "  return out",
        "end",
        "",
        "function M.configure(options)",
        "  options = options or {}",
        "  if options.game ~= nil then",
        "    assert(M.contract_capabilities[options.game] ~= nil,",
        "      \"host desconhecido: \" .. tostring(options.game))",
        "    M.host.game = options.game",
        "  end",
        "  if options.host_version ~= nil then M.host.host_version = options.host_version end",
        "  if options.runtime_version ~= nil then M.host.runtime_version = options.runtime_version end",
        "  M.host.capabilities = options.capabilities ~= nil",
        "    and copy_list(options.capabilities)",
        "    or copy_list(M.contract_capabilities[M.host.game])",
        "  return M",
        "end",
        "",
        "function M.reset()",
        "  M.calls = {}",
        "  M.subscriptions = {}",
        "  M.hotkeys = {}",
        "  M.next_subscription = 0",
        "end",
        "",
        "local function host_has(capability)",
        "  for _, granted in ipairs(M.host.capabilities) do",
        "    if granted == capability then return true end",
        "  end",
        "  return false",
        "end",
        "",
        "-- Disponibilidade derivada da IDL: funções 'common' existem sempre; funções",
        "-- específicas de host são instaladas pelo adaptador quando o host anuncia",
        "-- a capability correspondente (mesma regra de ValidateHostContext do núcleo).",
        "local function available_on(spec)",
        "  if spec.availability == \"common\" then return true end",
        "  if spec.availability ~= M.host.game then return false end",
        "  if spec.capability ~= nil then return host_has(spec.capability) end",
        "  return true",
        "end",
        "",
        "local function default_return(type_name)",
        "  if type_name == \"boolean\" then return true end",
        "  if type_name == \"integer\" or type_name == \"number\" then return 0 end",
        "  if type_name == \"string\" then return \"\" end",
        "  if type_name == \"nil\" then return nil end",
        "  if type_name:match(\"^array<.+>$\") then return {} end",
        "  local spec = validate.types[type_name]",
        "  if spec ~= nil and spec.kind == \"enum\" then return spec.values[1] end",
        "  if spec ~= nil and spec.kind == \"opaque\" then return default_return(spec.lua_type) end",
        "  return {}",
        "end",
        "",
        "-- Semântica mínima das funções de infraestrutura, dirigida pelos dados da IDL.",
        "local handlers = {}",
        "",
        "handlers[\"ship.game.id\"] = function() return M.host.game end",
        "handlers[\"ship.game.host_version\"] = function() return M.host.host_version end",
        "handlers[\"ship.runtime.version\"] = function() return M.host.runtime_version end",
        "handlers[\"ship.api.version\"] = function() return validate.api_version end",
        "",
        "handlers[\"ship.capabilities.has\"] = function(name) return host_has(name) end",
        "handlers[\"ship.capabilities.list\"] = function() return copy_list(M.host.capabilities) end",
        "",
        "handlers[\"ship.events.on\"] = function(event, options_or_callback, callback)",
        "  if validate.events[event] == nil then",
        "    error(\"unsupported: evento desconhecido '\" .. tostring(event) .. \"'\", 2)",
        "  end",
        "  local fn = callback or options_or_callback",
        "  M.next_subscription = M.next_subscription + 1",
        "  M.subscriptions[M.next_subscription] = { event = event, callback = fn }",
        "  return M.next_subscription",
        "end",
        "",
        "handlers[\"ship.events.off\"] = function(subscription)",
        "  if M.subscriptions[subscription] == nil then",
        "    error(\"invalid_handle: inscrição desconhecida\", 2)",
        "  end",
        "  M.subscriptions[subscription] = nil",
        "  return true",
        "end",
        "",
        "handlers[\"ship.hotkeys.register\"] = function(id, options, callback)",
        "  M.hotkeys[id] = callback",
        "  return true",
        "end",
        "",
        "local function make_stub(name, spec)",
        "  return function(...)",
        "    local ok, code, message = validate.validate(name, ...)",
        "    if not ok then",
        "      if spec.error_mode == \"return\" then",
        "        return nil, { code = code, message = message }",
        "      end",
        "      error(code .. \": \" .. message, 2)",
        "    end",
        "    local args = table.pack(...)",
        "    args.n = nil",
        "    M.calls[#M.calls + 1] = { name = name, arguments = args }",
        "    local handler = handlers[name]",
        "    if handler ~= nil then return handler(...) end",
        "    local value = default_return(spec.returns)",
        "    if spec.error_mode == \"return\" then return value, nil end",
        "    return value",
        "  end",
        "end",
        "",
        "local function set_dotted(root, path, value)",
        "  local node = root",
        "  local parts = {}",
        "  for part in path:gmatch(\"[^%.]+\") do parts[#parts + 1] = part end",
        "  for index = 1, #parts - 1 do",
        "    local part = parts[index]",
        "    if node[part] == nil then node[part] = {} end",
        "    node = node[part]",
        "  end",
        "  node[parts[#parts]] = value",
        "end",
        "",
        "-- Constrói o módulo `ship` para o host configurado.",
        "function M.build()",
        "  if M.host.capabilities == nil then M.configure({}) end",
        "  local ship = {}",
        "  for name, spec in pairs(validate.functions) do",
        "    if available_on(spec) then",
        "      set_dotted(ship, (name:gsub(\"^ship%.\", \"\")), make_stub(name, spec))",
        "    end",
        "  end",
        "  return ship",
        "end",
        "",
        "-- Dispara um evento para as inscrições do mock, em ordem determinística.",
        "function M.emit(event, payload)",
        "  for id = 1, M.next_subscription do",
        "    local subscription = M.subscriptions[id]",
        "    if subscription ~= nil and subscription.event == event then",
        "      subscription.callback(payload or {})",
        "    end",
        "  end",
        "end",
        "",
        "M.configure({})",
        "M.reset()",
        "",
        "return M",
        "",
    ])
    return "\n".join(lines)


def api_contract_probe(function: dict[str, Any]) -> list[str]:
    """Sondas específicas por função, derivadas da IDL. Funções sem sonda
    específica caem no fallback genérico de aridade zero (quando aplicável)."""
    name = function["name"]
    if name == "ship.game.id":
        return [
            'check(ship.game.id() == host, "ship.game.id() deveria igualar o host do contexto")',
        ]
    if name in ("ship.game.host_version", "ship.runtime.version"):
        return [
            f'check(type(ship.{dotted(name)}()) == "string", "{name}() deveria retornar string")',
        ]
    if name == "ship.api.version":
        return [
            'check(ship.api.version() == API_VERSION, "ship.api.version() diverge da IDL")',
        ]
    if name in ("ship.capabilities.has", "ship.capabilities.list", "ship.events.on",
                "ship.events.off", "ship.hotkeys.register"):
        return []  # cobertos por blocos dedicados
    if name.startswith("ship.log."):
        return [
            f'check(pcall(ship.{dotted(name)}, "contract probe"), '
            f'"{name} deveria aceitar mensagem textual")',
        ]
    if function["availability"] != "common":
        return []
    required = [argument for argument in function.get("arguments", [])
                if argument.get("required", True)]
    if not required:
        returns = function["returns"]
        if returns == "nil":
            return [f'check(pcall(ship.{dotted(name)}), "{name} deveria executar sem argumentos")']
        return [
            f"local ok_value, value = pcall(ship.{dotted(name)})",
            f'check(ok_value, "{name} deveria executar sem argumentos")',
            f'if ok_value then check_return_type("{name}", {q(returns)}, value) end',
        ]
    return []


def generate_api_contract_lua(api: dict[str, Any], events: dict[str, Any],
                              capabilities: dict[str, Any]) -> str:
    per_host = contract_capabilities(capabilities)
    function_names = {function["name"] for function in api["functions"]}
    enum_types = {item["name"]: item["values"] for item in api["types"]
                  if item["kind"] == "enum"}

    lines = [
        HEADER_LUA,
        "-- Testes de contrato da API pública, executados dentro do runtime real",
        "-- (ModHost) em contextos OoT e MM por tests/contract/ApiContractTests.cpp.",
        "-- Derivados da IDL (schema/*.yml); qualquer falha aborta a carga do mod.",
        "local ship = require(\"ship\")",
        "",
        f"local API_VERSION = {q(api['api_version'])}",
        "",
        "local FUNCTIONS = {",
    ]
    for function in api["functions"]:
        required_count = sum(1 for argument in function.get("arguments", [])
                             if argument.get("required", True))
        lines.append(
            f"  {{ name = {q(function['name'])}, availability = {q(function['availability'])}, "
            f"required_arguments = {required_count}, error_mode = "
            f"{q(function.get('error_mode', 'raise'))} }},")
    lines.extend([
        "}",
        "",
        f"local EVENTS = {qlist([event['name'] for event in events['events']])}",
        "",
        "local ENUM_VALUES = {",
    ])
    for name, values in enum_types.items():
        lines.append(f"  {name} = {qlist(values)},")
    lines.extend(["}", "", "local CONTRACT_CAPABILITIES = {"])
    for host in HOSTS:
        lines.append(f"  {host} = {qlist(per_host[host])},")
    lines.extend([
        "}",
        "",
        "local failures = {}",
        "local function check(condition, message)",
        "  if not condition then failures[#failures + 1] = message end",
        "end",
        "",
        "local function resolve(path)",
        "  local node = ship",
        "  for part in path:gmatch(\"[^%.]+\") do",
        "    if type(node) ~= \"table\" then return nil end",
        "    node = node[part]",
        "    if node == nil then return nil end",
        "  end",
        "  return node",
        "end",
        "",
        "local function check_return_type(name, type_name, value)",
        "  if type_name:match(\"^array<.+>$\") then",
        "    check(type(value) == \"table\", name .. \" deveria retornar tabela (\" .. type_name .. \")\")",
        "    return",
        "  end",
        "  local enum_values = ENUM_VALUES[type_name]",
        "  if enum_values ~= nil then",
        "    if type(value) ~= \"string\" then",
        "      check(false, name .. \" deveria retornar string (\" .. type_name .. \")\")",
        "      return",
        "    end",
        "    for _, allowed in ipairs(enum_values) do",
        "      if value == allowed then return end",
        "    end",
        "    check(false, name .. \" retornou valor fora do enum \" .. type_name)",
        "    return",
        "  end",
        "  if type_name == \"string\" then",
        "    check(type(value) == \"string\", name .. \" deveria retornar string\")",
        "  elseif type_name == \"boolean\" then",
        "    check(type(value) == \"boolean\", name .. \" deveria retornar boolean\")",
        "  elseif type_name == \"integer\" or type_name == \"number\" then",
        "    check(type(value) == \"number\", name .. \" deveria retornar number\")",
        "  elseif type_name == \"nil\" then",
        "    check(value == nil, name .. \" deveria retornar nil\")",
        "  else",
        "    check(type(value) == \"table\" or type(value) == \"number\",",
        "      name .. \" deveria retornar tabela ou handle (\" .. type_name .. \")\")",
        "  end",
        "end",
        "",
        "local host = ship.game.id()",
        "",
        "-- Superfície: funções 'common' existem no núcleo; funções específicas de host",
        "-- são instaladas pelos adaptadores de host, nunca pelo núcleo (RFC 0001).",
        "for _, spec in ipairs(FUNCTIONS) do",
        "  local fn = resolve((spec.name:gsub(\"^ship%.\", \"\")))",
        "  if spec.availability == \"common\" then",
        "    check(type(fn) == \"function\", spec.name .. \" ausente no núcleo\")",
        "  else",
        "    check(fn == nil, spec.name .. \" não deveria estar no núcleo \" ..",
        "      \"(adaptador \" .. spec.availability .. \")\")",
        "  end",
        "end",
        "",
        "-- Aridade: função com argumento obrigatório falha de forma controlada",
        "-- (erro Lua, nunca crash) quando chamada sem argumentos.",
        "for _, spec in ipairs(FUNCTIONS) do",
        "  local fn = resolve((spec.name:gsub(\"^ship%.\", \"\")))",
        "  if spec.availability == \"common\" and spec.required_arguments > 0",
        "      and type(fn) == \"function\" then",
        "    if spec.error_mode == \"return\" then",
        "      local ok, value, err = pcall(fn)",
        "      check(ok and value == nil and type(err) == \"table\"",
        "          and err.code == \"invalid_argument\",",
        "        spec.name .. \" deveria retornar invalid_argument estruturado\")",
        "    else",
        "      check(not pcall(fn), spec.name .. \" deveria falhar sem argumentos obrigatórios\")",
        "    end",
        "  end",
        "end",
        "",
    ])

    probes: list[str] = []
    for function in api["functions"]:
        probes.extend(api_contract_probe(function))
    if probes:
        lines.append("-- Sondas de retorno derivadas dos tipos da IDL.")
        lines.extend(probes)
        lines.append("")

    if {"ship.capabilities.has", "ship.capabilities.list"} <= function_names:
        lines.extend([
            "-- Capabilities: feature detection reflete o contexto anunciado pelo host.",
            "local granted = CONTRACT_CAPABILITIES[host] or {}",
            "for _, capability in ipairs(granted) do",
            "  check(ship.capabilities.has(capability) == true,",
            "    \"capability contratada ausente no host \" .. host .. \": \" .. capability)",
            "end",
            "check(ship.capabilities.has(\"contract.missing\") == false,",
            "  \"capability desconhecida deveria retornar false\")",
            "local listed = ship.capabilities.list()",
            "check(type(listed) == \"table\", \"ship.capabilities.list deveria retornar tabela\")",
            "if type(listed) == \"table\" then",
            "  for _, capability in ipairs(listed) do",
            "    check(ship.capabilities.has(capability) == true,",
            "      \"capability listada deveria ser detectável: \" .. tostring(capability))",
            "  end",
            "end",
            "",
        ])

    if {"ship.events.on", "ship.events.off"} <= function_names:
        lines.extend([
            "-- Eventos: toda entrada da IDL aceita inscrição e cancelamento; nomes",
            "-- desconhecidos e chamadas sem callback são rejeitados.",
            "for _, event in ipairs(EVENTS) do",
            "  local subscription = ship.events.on(event, function() end)",
            "  check(type(subscription) == \"number\",",
            "    \"inscrição em \" .. event .. \" deveria retornar inteiro\")",
            "  if type(subscription) == \"number\" then",
            "    check(ship.events.off(subscription) == true,",
            "      \"cancelamento de \" .. event .. \" deveria confirmar\")",
            "  end",
            "end",
            "check(not pcall(ship.events.on, \"contract.unknown\", function() end),",
            "  \"evento desconhecido deveria ser rejeitado\")",
            "check(not pcall(ship.events.on, EVENTS[1]),",
            "  \"ship.events.on sem callback deveria falhar\")",
            "check(not pcall(ship.events.off, 0),",
            "  \"ship.events.off com inscrição inválida deveria falhar\")",
            "",
        ])

    if "ship.hotkeys.register" in function_names:
        lines.extend([
            "-- Hotkeys: registro com opções, forma curta e rejeição de id inválido.",
            "-- O contexto de teste sempre fornece um registry (host com suporte).",
            "check(ship.hotkeys.register(\"contract.probe\",",
            "    { default = \"F5\", label = \"probe\" }, function() end) == true,",
            "  \"registro de hotkey com opções deveria confirmar\")",
            "check(ship.hotkeys.register(\"contract.probe_shorthand\", function() end) == true,",
            "  \"registro de hotkey sem opções deveria confirmar\")",
            "check(not pcall(ship.hotkeys.register, \"Invalid ID\", function() end),",
            "  \"id de hotkey inválido deveria ser rejeitado\")",
            "",
        ])

    lines.extend([
        "if #failures > 0 then",
        "  error(\"contract failures (\" .. #failures .. \"):\\n\" .. table.concat(failures, \"\\n\"), 0)",
        "end",
        "",
    ])
    return "\n".join(lines)


def valid_arg_lua(type_name: str, api: dict[str, Any]) -> str | None:
    """Literal Lua de um argumento válido para o tipo, ou None se não deriva."""
    if type_name == "any" or type_name == "string":
        return q("probe")
    if type_name in ("integer", "number"):
        return "1"
    if type_name == "boolean":
        return "true"
    if type_name == "callback":
        return "function() end"
    if type_name.startswith("array<") and type_name.endswith(">"):
        return "{}"
    for item in api["types"]:
        if item["name"] != type_name:
            continue
        if item["kind"] == "enum":
            return q(item["values"][0])
        if item["kind"] == "opaque":
            return valid_arg_lua(item["lua_type"], api)
        if item["kind"] == "object":
            if any(field.get("required", True) for field in item.get("fields", [])):
                return None
            return "{}"
    return None


def expect_kind(type_name: str, api: dict[str, Any]) -> str | None:
    """Classificação de retorno usada pelas sondas do mock."""
    if type_name in ("nil", "boolean", "string", "integer", "number"):
        return type_name
    if type_name.startswith("array<") and type_name.endswith(">"):
        return "table"
    for item in api["types"]:
        if item["name"] != type_name:
            continue
        if item["kind"] == "enum":
            return f"enum:{type_name}"
        if item["kind"] == "opaque":
            return expect_kind(item["lua_type"], api)
        if item["kind"] == "object":
            return "table"
    return None


def mock_probe(function: dict[str, Any], api: dict[str, Any], events: dict[str, Any],
               capabilities: dict[str, Any]) -> list[str]:
    """Sonda de chamada válida por função, derivada da IDL."""
    name = function["name"]
    if name == "ship.game.id":
        return ['probe("ship.game.id", {}, "enum:game_id")']
    if name in ("ship.game.host_version", "ship.runtime.version"):
        return [f'probe("{name}", {{}}, "string")']
    if name == "ship.api.version":
        return ['probe("ship.api.version", {}, "api_version")']
    if name == "ship.capabilities.has":
        first = contract_capabilities(capabilities)["oot"][0]
        return [f'probe("ship.capabilities.has", {{ {q(first)} }}, "boolean")']
    if name == "ship.capabilities.list":
        return ['probe("ship.capabilities.list", {}, "table")']
    if name == "ship.events.on":
        first_event = events["events"][0]["name"]
        return [f'probe("ship.events.on", {{ {q(first_event)}, function() end }}, "integer")']
    if name == "ship.events.off":
        return []  # coberto pelo fluxo de eventos dedicado
    if name == "ship.hotkeys.register":
        return ['probe("ship.hotkeys.register", '
                '{ "contract.probe", { default = "F5" }, function() end }, "boolean")']
    if name.startswith("ship.log."):
        return [f'probe("{name}", {{ "contract probe" }}, "nil")']
    arguments: list[str] = []
    for argument in function.get("arguments", []):
        literal = valid_arg_lua(argument["type"], api)
        if literal is None:
            return []
        arguments.append(literal)
    expect = expect_kind(function["returns"], api)
    if expect is None:
        return []
    return [f'probe("{name}", {{ {", ".join(arguments)} }}, {q(expect)})']


def generate_mock_contract_lua(api: dict[str, Any], events: dict[str, Any],
                               capabilities: dict[str, Any]) -> str:
    per_host = contract_capabilities(capabilities)
    first_event = events["events"][0]["name"]
    function_names = {function["name"] for function in api["functions"]}
    enum_types = [item for item in api["types"] if item["kind"] == "enum"]
    opaque_types = [item for item in api["types"] if item["kind"] == "opaque"]
    host_functions = [function for function in api["functions"]
                      if function["availability"] != "common"]
    arg_checked = [function for function in api["functions"]
                   if function["availability"] == "common"
                   and any(argument.get("required", True)
                           for argument in function.get("arguments", []))]

    lines = [
        HEADER_LUA,
        "-- Testes de contrato do mock runtime e dos validadores, executados em um",
        "-- lua_State bruto por tests/contract/MockRuntimeTests.cpp. Derivados da IDL.",
        "local validate = require(\"shiplua_validate\")",
        "local mock = require(\"shiplua_mock\")",
        "",
        "local failures = {}",
        "local function check(condition, message)",
        "  if not condition then failures[#failures + 1] = message end",
        "end",
        "",
        "local function resolve(root, path)",
        "  local node = root",
        "  for part in path:gmatch(\"[^%.]+\") do",
        "    if type(node) ~= \"table\" then return nil end",
        "    node = node[part]",
        "    if node == nil then return nil end",
        "  end",
        "  return node",
        "end",
        "",
        "local function check_return(name, expect, value)",
        "  if expect == \"nil\" then",
        "    check(value == nil, name .. \" deveria retornar nil\")",
        "    return",
        "  end",
        "  if expect == \"api_version\" then",
        "    check(value == validate.api_version, name .. \" deveria retornar a versão da IDL\")",
        "    return",
        "  end",
        "  local enum_name = expect:match(\"^enum:(.+)$\")",
        "  if enum_name then",
        "    local spec = validate.types[enum_name]",
        "    if type(value) ~= \"string\" then",
        "      check(false, name .. \" deveria retornar string (\" .. enum_name .. \")\")",
        "      return",
        "    end",
        "    for _, allowed in ipairs(spec.values) do",
        "      if value == allowed then return end",
        "    end",
        "    check(false, name .. \" retornou valor fora do enum \" .. enum_name)",
        "    return",
        "  end",
        "  if expect == \"integer\" then",
        "    check(type(value) == \"number\" and math.type(value) == \"integer\",",
        "      name .. \" deveria retornar integer\")",
        "    return",
        "  end",
        "  if expect == \"number\" then",
        "    check(type(value) == \"number\", name .. \" deveria retornar number\")",
        "    return",
        "  end",
        "  if expect == \"table\" then",
        "    check(type(value) == \"table\", name .. \" deveria retornar tabela\")",
        "    return",
        "  end",
        "  check(type(value) == expect, name .. \" deveria retornar \" .. expect)",
        "end",
        "",
        "local ship_mod",
        "",
        "local function probe(name, args, expect)",
        "  local fn = resolve(ship_mod, (name:gsub(\"^ship%.\", \"\")))",
        "  check(type(fn) == \"function\", name .. \" ausente no mock (host \" .. mock.host.game .. \")\")",
        "  if type(fn) ~= \"function\" then return end",
        "  local outcome = table.pack(pcall(fn, table.unpack(args)))",
        "  check(outcome[1], name .. \" falhou com argumentos válidos: \" .. tostring(outcome[2]))",
        "  if outcome[1] then check_return(name, expect, outcome[2]) end",
        "end",
        "",
        "local function expected_available(spec, game, granted)",
        "  if spec.availability == \"common\" then return true end",
        "  if spec.availability ~= game then return false end",
        "  if spec.capability ~= nil then",
        "    for _, capability in ipairs(granted) do",
        "      if capability == spec.capability then return true end",
        "    end",
        "    return false",
        "  end",
        "  return true",
        "end",
        "",
    ]
    lines.extend([
        "-- Validadores: função desconhecida e argumento obrigatório ausente.",
        "do",
        "  local ok, code = validate.validate(\"ship.unknown.fn\", 1)",
        "  check(not ok and code == \"unsupported\", \"função desconhecida deveria retornar unsupported\")",
    ])
    if "ship.capabilities.has" in function_names:
        lines.extend([
            "  local ok_missing, code_missing = validate.validate(\"ship.capabilities.has\")",
            "  check(not ok_missing and code_missing == \"invalid_argument\",",
            "    \"argumento obrigatório ausente deveria retornar invalid_argument\")",
            "  check(validate.validate(\"ship.capabilities.has\", \"scene.events\") == true,",
            "    \"argumentos válidos deveriam ser aceitos\")",
            "  local ok_wrong = validate.validate(\"ship.capabilities.has\", 42)",
            "  check(not ok_wrong, \"tipo errado de argumento deveria ser rejeitado\")",
        ])
    lines.extend([
        "end",
        "",
        "-- Validadores: tipos enum, opacos e arrays conforme a IDL.",
    ])
    for item in enum_types:
        good = item["values"][0]
        lines.extend([
            f"check(validate.check_type({q(item['name'])}, {q(good)}) == true,",
            f"  \"enum {item['name']} deveria aceitar {good}\")",
            f"check(select(1, validate.check_type({q(item['name'])}, {q('__invalid__')})) == false,",
            f"  \"enum {item['name']} deveria rejeitar valor fora do contrato\")",
        ])
    for item in opaque_types:
        literal = valid_arg_lua(item["lua_type"], api) or "1"
        lines.append(
            f"check(validate.check_type({q(item['name'])}, {literal}) == true,"
            f" \"opaco {item['name']} deveria aceitar {item['lua_type']}\")")
    lines.extend([
        "check(validate.check_type(\"array<string>\", {}) == true, \"array deveria aceitar tabela\")",
        "check(select(1, validate.check_type(\"array<string>\", \"não-tabela\")) == false,",
        "  \"array deveria rejeitar escalar\")",
        "",
    ])
    callback_overloads = [function for function in api["functions"]
                          if requires_callback(function)]
    if callback_overloads:
        overload = callback_overloads[0]
        first_argument = overload["arguments"][0]
        first_literal = valid_arg_lua(first_argument["type"], api) or q("probe")
        lines.extend([
            "-- Sobrecarga '<opções ou callback>': ao menos um callback é exigido.",
            f"check(validate.validate({q(overload['name'])}, {first_literal}, function() end) == true,",
            f"  \"{overload['name']} com callback deveria validar\")",
            f"check(select(1, validate.validate({q(overload['name'])}, {first_literal}, {{}})) == false,",
            f"  \"{overload['name']} sem callback deveria falhar\")",
            "",
        ])
    lines.extend([
        "-- Matriz de disponibilidade: superfície do mock casa com a IDL nos dois hosts.",
        "for _, game in ipairs({ \"oot\", \"mm\" }) do",
        "  mock.configure({ game = game })",
        "  ship_mod = mock.build()",
        "  for name, spec in pairs(validate.functions) do",
        "    local present = resolve(ship_mod, (name:gsub(\"^ship%.\", \"\"))) ~= nil",
        "    check(present == expected_available(spec, game, mock.host.capabilities),",
        "      name .. \": disponibilidade diverge do contrato no host \" .. game)",
        "  end",
        "end",
        "",
        "-- Sondas de chamada válida no host OoT (funções common).",
        "mock.configure({ game = \"oot\" })",
        "ship_mod = mock.build()",
    ])
    for function in api["functions"]:
        if function["availability"] == "common":
            lines.extend(mock_probe(function, api, events, capabilities))
    for function in host_functions:
        if function["availability"] == "oot":
            continue
        lines.append(
            f"check(resolve(ship_mod, {q(dotted(function['name']))}) == nil,"
            f" {q(function['name'] + ' não deveria existir no host oot')})")
    lines.append("")

    for host in HOSTS:
        specific = [function for function in host_functions
                    if function["availability"] == host]
        if not specific:
            continue
        lines.extend([
            f"-- Sondas de chamada válida no host {host.upper()} (funções do adaptador).",
            f"mock.configure({{ game = {q(host)} }})",
            "ship_mod = mock.build()",
        ])
        for function in specific:
            lines.extend(mock_probe(function, api, events, capabilities))
        lines.append("")

    gated = [function for function in host_functions if function.get("capability")]
    if gated:
        lines.extend([
            "-- Gating por capability: sem o grant, o binding do adaptador não é instalado.",
        ])
        for function in gated:
            lines.extend([
                f"mock.configure({{ game = {q(function['availability'])}, capabilities = {{}} }})",
                "ship_mod = mock.build()",
                f"check(resolve(ship_mod, {q(dotted(function['name']))}) == nil,"
                f" {q(function['name'] + ' sem capability não deveria existir')})",
            ])
        lines.append("")

    if arg_checked:
        lines.extend([
            "-- Erros estruturados: chamada sem argumentos obrigatórios falha com código.",
            "mock.configure({ game = \"oot\" })",
            "ship_mod = mock.build()",
        ])
        for function in arg_checked:
            if function.get("error_mode") == "return":
                lines.extend([
                    "do",
                    f"  local fn = resolve(ship_mod, {q(dotted(function['name']))})",
                    "  local value, err = fn()",
                    f"  check(value == nil, {q(function['name'] + ' sem argumentos não deveria retornar valor')})",
                    "  check(type(err) == \"table\" and err.code == \"invalid_argument\",",
                    f"    {q('erro de ' + function['name'] + ' deveria carregar invalid_argument')})",
                    "end",
                ])
            else:
                lines.extend([
                    "do",
                    f"  local fn = resolve(ship_mod, {q(dotted(function['name']))})",
                    "  local ok, err = pcall(fn)",
                    f"  check(not ok, {q(function['name'] + ' sem argumentos deveria falhar')})",
                    "  check(type(err) == \"string\" and err:find(\"invalid_argument\", 1, true) ~= nil,",
                    f"    {q('erro de ' + function['name'] + ' deveria carregar invalid_argument')})",
                    "end",
                ])
        lines.append("")

    lines.extend([
        "-- Semântica de infraestrutura do mock.",
        "mock.configure({ game = \"oot\" })",
        "ship_mod = mock.build()",
    ])
    if "ship.game.id" in function_names:
        lines.append(
            "check(ship_mod.game.id() == \"oot\", \"ship.game.id do mock deveria refletir o host\")")
    if "ship.api.version" in function_names:
        lines.extend([
            "check(ship_mod.api.version() == validate.api_version,",
            "  \"ship.api.version do mock deveria casar com a IDL\")",
        ])
    if "ship.capabilities.has" in function_names:
        lines.extend([
            f"check(ship_mod.capabilities.has({q(per_host['oot'][0])}) == true,",
            "  \"capability contract do host deveria ser detectável no mock\")",
            "check(ship_mod.capabilities.has(\"contract.missing\") == false,",
            "  \"capability desconhecida deveria retornar false no mock\")",
        ])
    lines.append("")

    if {"ship.events.on", "ship.events.off"} <= function_names:
        lines.extend([
            "-- Fluxo de eventos: inscrição, dispatch determinístico, cancelamento.",
            "mock.reset()",
            "local fired = 0",
            f"local subscription = ship_mod.events.on({q(first_event)}, function() fired = fired + 1 end)",
            f"mock.emit({q(first_event)}, {{}})",
            "check(fired == 1, \"mock.emit deveria disparar o callback inscrito\")",
            "check(ship_mod.events.off(subscription) == true,",
            "  \"cancelamento de inscrição válida deveria confirmar\")",
            f"mock.emit({q(first_event)}, {{}})",
            "check(fired == 1, \"callback removido não deveria disparar\")",
            f"check(not pcall(ship_mod.events.on, \"contract.unknown\", function() end),",
            "  \"evento desconhecido deveria falhar no mock\")",
            "check(#mock.calls == 3, \"chamadas da API deveriam ser registradas em ordem\")",
            "check(mock.calls[1].name == \"ship.events.on\",",
            "  \"primeira chamada registrada deveria ser ship.events.on\")",
            "",
        ])

    if "ship.hotkeys.register" in function_names:
        lines.extend([
            "-- Hotkeys: registro retorna true e retém o callback.",
            "check(ship_mod.hotkeys.register(\"contract.probe\",",
            "    { default = \"F5\", label = \"probe\" }, function() end) == true,",
            "  \"registro de hotkey deveria confirmar no mock\")",
            "check(type(mock.hotkeys[\"contract.probe\"]) == \"function\",",
            "  \"callback de hotkey deveria ser retido pelo mock\")",
            "",
        ])

    lines.extend([
        "-- Reset limpa o histórico de chamadas.",
        "mock.reset()",
        "check(#mock.calls == 0, \"reset deveria limpar as chamadas registradas\")",
        "",
        "if #failures > 0 then",
        "  error(\"mock contract failures (\" .. #failures .. \"):\\n\" .. table.concat(failures, \"\\n\"), 0)",
        "end",
        'print("mock contract OK")',
        "",
    ])
    return "\n".join(lines)


STABILITY_ORDER = ("stable", "preview", "experimental", "deprecated")


def generate_compatibility_matrix(api: dict[str, Any], events: dict[str, Any],
                                  capabilities: dict[str, Any]) -> str:
    lines = [
        HEADER_MD,
        "# Matriz de compatibilidade — ShipLua Runtime API",
        "",
        f"Versão da API: `{api['api_version']}` (schema `{api['schema_version']}`).",
        "Derivada dos schemas canônicos em `schema/` (IDL); regenere com",
        "`tools/generate_api_contracts.py`.",
        "",
        "## Funções",
        "",
        "| Função | Desde | Estabilidade | OoT | MM | Capability | Erros |",
        "|---|---|---|---|---|---|---|",
    ]
    for function in api["functions"]:
        availability = function["availability"]
        oot = "sim" if availability in ("common", "oot") else "—"
        mm = "sim" if availability in ("common", "mm") else "—"
        capability = f"`{function['capability']}`" if function.get("capability") else "—"
        errors = ", ".join(f"`{code}`" for code in function.get("errors", [])) or "—"
        lines.append(
            f"| `{function['name']}` | `{function['version']}` | `{function['stability']}` | "
            f"{oot} | {mm} | {capability} | {errors} |")
    lines.extend([
        "",
        "Funções com disponibilidade específica (`oot`/`mm`) são instaladas pelo",
        "adaptador do host quando a capability correspondente é anunciada; o núcleo",
        "nunca registra `ship.oot.*` ou `ship.mm.*` (RFC 0001).",
        "",
        "## Eventos",
        "",
        "| Evento | Fase | Cancelável | OoT | MM | Capability |",
        "|---|---|---:|---|---|---|",
    ])
    for event in events["events"]:
        support = set(event["support"])
        oot = "sim" if "oot" in support else "—"
        mm = "sim" if "mm" in support else "—"
        cancellable = "sim" if event["cancellable"] else "não"
        capability = f"`{event['capability']}`" if event.get("capability") else "—"
        lines.append(
            f"| `{event['name']}` | `{event['phase']}` | {cancellable} | {oot} | {mm} | "
            f"{capability} |")
    lines.extend([
        "",
        "## Capabilities",
        "",
        "| Capability | Status | OoT | MM |",
        "|---|---|---|---|",
    ])
    for capability in capabilities["capabilities"]:
        hosts = set(capability["hosts"])
        oot = "sim" if "oot" in hosts else "—"
        mm = "sim" if "mm" in hosts else "—"
        lines.append(f"| `{capability['name']}` | `{capability['status']}` | {oot} | {mm} |")
    lines.extend([
        "",
        "## Estabilidade das funções",
        "",
        "| Estabilidade | Funções |",
        "|---|---:|",
    ])
    for stability in STABILITY_ORDER:
        count = sum(1 for function in api["functions"] if function["stability"] == stability)
        if count:
            lines.append(f"| `{stability}` | {count} |")
    lines.extend(["", ""])
    return "\n".join(lines)


OUTPUTS: tuple[tuple[str, Callable[[dict[str, Any], dict[str, Any], dict[str, Any]], str], str], ...] = (
    ("generated/lua/shiplua_validate.lua", generate_validate_lua, "Validadores Lua"),
    ("generated/lua/shiplua_mock.lua", generate_mock_lua, "Mock runtime Lua"),
    ("generated/tests/api_contract.lua", generate_api_contract_lua,
     "Testes de contrato (runtime real)"),
    ("generated/tests/mock_contract.lua", generate_mock_contract_lua,
     "Testes de contrato (mock)"),
    ("generated/docs/compatibility-matrix.md", generate_compatibility_matrix,
     "Matriz de compatibilidade"),
)


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(
        description="Gera validadores, mock, testes de contrato e matriz da IDL ShipLua.")
    root = Path(__file__).resolve().parents[1]
    parser.add_argument("--schema-dir", type=Path, default=root / "schema")
    parser.add_argument("--root", type=Path, default=root,
                        help="raiz do repositório para localizar os artefatos gerados")
    parser.add_argument("--check", action="store_true",
                        help="falha se algum artefato estiver ausente ou divergente")
    args = parser.parse_args()
    try:
        api = load_document(args.schema_dir / "api.yml")
        events = load_document(args.schema_dir / "events.yml")
        capabilities = load_document(args.schema_dir / "capabilities.yml")
        schema_errors = validate_documents(api, events, capabilities)
        if schema_errors:
            raise ValueError("schemas inválidos:\n- " + "\n- ".join(schema_errors))
        artifacts = [(args.root / relative, generator(api, events, capabilities), label)
                     for relative, generator, label in OUTPUTS]
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"Falha ao gerar artefatos de contrato: {error}", file=sys.stderr)
        return 1

    if args.check:
        drift: list[str] = []
        for path, expected, label in artifacts:
            try:
                current = path.read_text(encoding="utf-8")
            except OSError:
                drift.append(f"{label} ausente: {path}")
                continue
            if current != expected:
                drift.append(f"{label} diverge dos schemas: {path}")
        if drift:
            print("\n".join(drift), file=sys.stderr)
            print("Execute tools/generate_api_contracts.py para regenerar.", file=sys.stderr)
            return 1
        print("Artefatos de contrato sincronizados com os schemas.")
        return 0

    for path, content, label in artifacts:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
        print(f"Gerado ({label}): {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
