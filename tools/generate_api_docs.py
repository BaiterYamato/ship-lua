#!/usr/bin/env python3
"""Gera LuaDoc e referência Markdown dos schemas públicos ShipLua."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_cpp_api import cpp_identifier  # noqa: E402
from validate_api_schemas import load_document, validate_documents  # noqa: E402


def lua_type(type_name: str, custom_types: dict[str, str]) -> str:
    builtins = {
        "any": "any",
        "boolean": "boolean",
        "callback": "function",
        "integer": "integer",
        "nil": "nil",
        "number": "number",
        "string": "string",
    }
    if type_name.startswith("array<") and type_name.endswith(">"):
        return lua_type(type_name[6:-1], custom_types) + "[]"
    return builtins.get(type_name, custom_types.get(type_name, type_name))


def description(value: Any) -> str:
    if not isinstance(value, str) or not value.strip():
        return "Sem descrição adicional."
    return " ".join(value.split())


def argument_lua_type(function_name: str, argument: dict[str, Any],
                      custom_types: dict[str, str]) -> str:
    if function_name == "ship.events.on" and argument["name"] == "event":
        return "ShipLuaEventName"
    return lua_type(argument["type"], custom_types)


def generate_luadoc(api: dict[str, Any], events: dict[str, Any],
                    capabilities: dict[str, Any]) -> str:
    errors = validate_documents(api, events, capabilities)
    if errors:
        raise ValueError("schemas inválidos:\n- " + "\n- ".join(errors))

    custom_types = {item["name"]: "ShipLua" + cpp_identifier(item["name"])
                    for item in api["types"]}
    lines = [
        "---@meta ShipLua",
        "-- Gerado por tools/generate_api_docs.py. Não edite manualmente.",
        f"-- API {api['api_version']} / schema {api['schema_version']}",
        "",
    ]
    for item in api["types"]:
        type_name = custom_types[item["name"]]
        lines.append("--- " + description(item.get("description")))
        if item["kind"] == "enum":
            variants = "|".join(json.dumps(value, ensure_ascii=False) for value in item["values"])
            lines.append(f"---@alias {type_name} {variants}")
        elif item["kind"] == "opaque":
            lines.append(f"---@alias {type_name} {lua_type(item['lua_type'], custom_types)}")
        else:
            lines.append(f"---@class {type_name}")
            for field in item["fields"]:
                optional = "?" if not field.get("required", True) else ""
                lines.append(
                    f"---@field {field['name']}{optional} {lua_type(field['type'], custom_types)}")
        lines.append("")

    for event in events["events"]:
        payload_name = "ShipLuaEvent" + cpp_identifier(event["name"])
        lines.append(f"---@class {payload_name}")
        for field in event.get("payload", []):
            optional = "?" if not field.get("required", True) else ""
            lines.append(f"---@field {field['name']}{optional} "
                         f"{lua_type(field['type'], custom_types)}")
        lines.append("")
    event_names = "|".join(json.dumps(item["name"], ensure_ascii=False)
                           for item in events["events"])
    lines.extend([f"---@alias ShipLuaEventName {event_names}", ""])

    namespaces: set[str] = set()
    for function in api["functions"]:
        parts = function["name"].split(".")[:-1]
        for end in range(1, len(parts) + 1):
            namespaces.add(".".join(parts[:end]))
    for namespace in sorted(namespaces, key=lambda item: (item.count("."), item)):
        lines.append(f"{namespace} = {namespace} or {{}}")
    lines.append("")

    for function in api["functions"]:
        capability = function.get("capability") or "comum"
        error_list = ", ".join(function.get("errors", [])) or "nenhum"
        lines.extend([
            f"--- API {function['availability']}; estabilidade: {function['stability']}; "
            f"desde: {function['version']}; capability: {capability}; erros: {error_list}.",
        ])
        arguments = function.get("arguments", [])
        for argument in arguments:
            optional = "?" if not argument.get("required", True) else ""
            lines.append(f"---@param {argument['name']}{optional} "
                         f"{argument_lua_type(function['name'], argument, custom_types)}")
        return_type = lua_type(function['returns'], custom_types)
        if function.get("error_mode") == "return":
            lines.append(f"---@return {return_type}? value")
            lines.append(
                f"---@return {lua_type(function['error_type'], custom_types)}? error")
        else:
            lines.append(f"---@return {return_type}")
        parameters = ", ".join(argument["name"] for argument in arguments)
        lines.extend([f"function {function['name']}({parameters}) end", ""])
    return "\n".join(lines)


def md_escape(value: Any) -> str:
    return str(value).replace("|", "\\|").replace("\n", " ")


def field_list(fields: list[dict[str, Any]]) -> str:
    if not fields:
        return "—"
    return ", ".join(
        f"`{md_escape(field['name'])}: {md_escape(field['type'])}"
        f"{'?' if not field.get('required', True) else ''}`" for field in fields)


def generate_markdown(api: dict[str, Any], events: dict[str, Any],
                      capabilities: dict[str, Any]) -> str:
    errors = validate_documents(api, events, capabilities)
    if errors:
        raise ValueError("schemas inválidos:\n- " + "\n- ".join(errors))

    lines = [
        "<!-- Gerado por tools/generate_api_docs.py. Não edite manualmente. -->",
        "# Referência da API ShipLua",
        "",
        f"Versão da API: `{api['api_version']}`. Versão do schema: `{api['schema_version']}`.",
        "",
        "## Tipos",
        "",
        "| Nome | Tipo | Contrato | Descrição |",
        "|---|---|---|---|",
    ]
    for item in api["types"]:
        if item["kind"] == "enum":
            contract = ", ".join(f"`{value}`" for value in item["values"])
        elif item["kind"] == "opaque":
            contract = f"Lua `{item['lua_type']}`"
        else:
            contract = field_list(item["fields"])
        lines.append(f"| `{item['name']}` | `{item['kind']}` | {contract} | "
                     f"{md_escape(description(item.get('description')))} |")

    lines.extend([
        "", "## Funções", "",
        "| Função | Argumentos | Retorno | Disponibilidade | Estabilidade | Desde | Capability | Erros |",
        "|---|---|---|---|---|---|---|---|",
    ])
    for function in api["functions"]:
        errors_text = ", ".join(f"`{code}`" for code in function.get("errors", [])) or "—"
        capability = f"`{function['capability']}`" if function.get("capability") else "—"
        return_text = function['returns']
        if function.get("error_mode") == "return":
            return_text += f", {function['error_type']}?"
        lines.append(f"| `{function['name']}` | {field_list(function.get('arguments', []))} | "
                     f"`{return_text}` | `{function['availability']}` | "
                     f"`{function['stability']}` | `{function['version']}` | {capability} | "
                     f"{errors_text} |")

    lines.extend([
        "", "## Eventos", "",
        "| Evento | Tipo | Fase | Hosts | Cancelável | Capability | Payload |",
        "|---|---|---|---|---:|---|---|",
    ])
    for event in events["events"]:
        capability = f"`{event['capability']}`" if event.get("capability") else "—"
        hosts = ", ".join(f"`{host}`" for host in event["support"])
        lines.append(f"| `{event['name']}` | `{event['kind']}` | `{event['phase']}` | {hosts} | "
                     f"{'sim' if event['cancellable'] else 'não'} | {capability} | "
                     f"{field_list(event.get('payload', []))} |")

    lines.extend([
        "", "## Capabilities", "",
        "| Capability | Estado | Hosts | Descrição |",
        "|---|---|---|---|",
    ])
    for capability in capabilities["capabilities"]:
        hosts = ", ".join(f"`{host}`" for host in capability["hosts"])
        lines.append(f"| `{capability['name']}` | `{capability['status']}` | {hosts} | "
                     f"{md_escape(description(capability.get('description')))} |")

    lines.extend([
        "", "## Códigos de erro", "",
        ", ".join(f"`{code}`" for code in api["error_codes"]) + ".", "",
    ])
    return "\n".join(lines)


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Gera LuaDoc e Markdown da API ShipLua.")
    root = Path(__file__).resolve().parents[1]
    parser.add_argument("--schema-dir", type=Path, default=root / "schema")
    parser.add_argument("--luadoc-output", type=Path, default=root / "generated/lua/shiplua.lua")
    parser.add_argument("--markdown-output", type=Path,
                        default=root / "generated/docs/api-reference.md")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        api = load_document(args.schema_dir / "api.yml")
        events = load_document(args.schema_dir / "events.yml")
        capabilities = load_document(args.schema_dir / "capabilities.yml")
        luadoc = generate_luadoc(api, events, capabilities)
        markdown = generate_markdown(api, events, capabilities)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"Falha ao gerar documentação da API: {error}", file=sys.stderr)
        return 1

    outputs = ((args.luadoc_output, luadoc, "LuaDoc"),
               (args.markdown_output, markdown, "Markdown"))
    if args.check:
        drift: list[str] = []
        for path, expected, label in outputs:
            try:
                current = path.read_text(encoding="utf-8")
            except OSError:
                drift.append(f"{label} ausente: {path}")
                continue
            if current != expected:
                drift.append(f"{label} diverge dos schemas: {path}")
        if drift:
            print("\n".join(drift), file=sys.stderr)
            return 1
        print("LuaDoc e Markdown sincronizados com os schemas.")
        return 0

    for path, content, _ in outputs:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
        print(f"Gerado: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
