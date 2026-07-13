#!/usr/bin/env python3
"""Valida os schemas canônicos ShipLua, armazenados como JSON/YAML 1.2."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

BUILTIN_TYPES = {"any", "boolean", "callback", "integer", "nil", "number", "string"}
HOSTS = {"oot", "mm"}
EVENT_KINDS = {"observe", "filter", "transform", "consume"}
AVAILABILITY = {"common", "oot", "mm"}
SEMVER = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")
FUNCTION_NAME = re.compile(r"^ship(?:\.[a-z][a-z0-9_]*){2,}$")
DOTTED_NAME = re.compile(r"^[a-z][a-z0-9_]*(?:\.[a-z][a-z0-9_]*)+$")


def load_document(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as source:
        document = json.load(source)
    if not isinstance(document, dict):
        raise ValueError(f"{path}: documento raiz deve ser um objeto")
    return document


def _unique_names(items: list[dict[str, Any]], label: str, errors: list[str]) -> set[str]:
    names: set[str] = set()
    for item in items:
        name = item.get("name")
        if not isinstance(name, str) or not name:
            errors.append(f"{label}: item sem nome válido")
        elif name in names:
            errors.append(f"{label}: nome duplicado '{name}'")
        else:
            names.add(name)
    return names


def _base_type(type_name: Any, location: str, errors: list[str]) -> str | None:
    if not isinstance(type_name, str) or not type_name:
        errors.append(f"{location}: tipo ausente ou inválido")
        return None
    lowered = type_name.lower()
    if "pointer" in lowered or "address" in lowered or type_name.endswith("*"):
        errors.append(f"{location}: tipo proibido '{type_name}'")
    if type_name.startswith("array<") and type_name.endswith(">"):
        return type_name[6:-1]
    return type_name


def validate_documents(api: dict[str, Any], events: dict[str, Any],
                       capabilities: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    documents = {"api": api, "events": events, "capabilities": capabilities}
    versions = {document.get("api_version") for document in documents.values()}
    schema_versions = {document.get("schema_version") for document in documents.values()}
    if len(versions) != 1 or None in versions:
        errors.append("documentos: api_version divergente ou ausente")
    elif not SEMVER.fullmatch(next(iter(versions))):
        errors.append("documentos: api_version não é SemVer completo")
    if schema_versions != {1}:
        errors.append("documentos: schema_version deve ser 1 em todos os arquivos")

    type_items = api.get("types", [])
    function_items = api.get("functions", [])
    event_items = events.get("events", [])
    capability_items = capabilities.get("capabilities", [])
    for label, items in (("api.types", type_items), ("api.functions", function_items),
                         ("events", event_items), ("capabilities", capability_items)):
        if not isinstance(items, list):
            errors.append(f"{label}: deve ser uma lista")
            return errors

    custom_types = _unique_names(type_items, "api.types", errors)
    function_names = _unique_names(function_items, "api.functions", errors)
    event_names = _unique_names(event_items, "events", errors)
    capability_names = _unique_names(capability_items, "capabilities", errors)
    known_types = BUILTIN_TYPES | custom_types

    capability_map = {item.get("name"): item for item in capability_items if item.get("name")}
    for item in capability_items:
        name = item.get("name")
        status = item.get("status")
        hosts = item.get("hosts")
        if not DOTTED_NAME.fullmatch(name or ""):
            errors.append(f"capabilities: nome inválido '{name}'")
        if status not in {"contract", "planned", "deprecated"}:
            errors.append(f"capability '{name}': status inválido")
        if not isinstance(hosts, list) or not hosts or set(hosts) - HOSTS or len(hosts) != len(set(hosts)):
            errors.append(f"capability '{name}': hosts inválidos")
        if isinstance(name, str) and name.startswith("mm.") and hosts != ["mm"]:
            errors.append(f"capability '{name}': prefixo mm exige host mm")
        if isinstance(name, str) and name.startswith("oot.") and hosts != ["oot"]:
            errors.append(f"capability '{name}': prefixo oot exige host oot")

    def validate_fields(fields: Any, location: str) -> None:
        if not isinstance(fields, list):
            errors.append(f"{location}: fields deve ser uma lista")
            return
        _unique_names(fields, location, errors)
        for field in fields:
            base = _base_type(field.get("type"), f"{location}.{field.get('name')}", errors)
            if base is not None and base not in known_types:
                errors.append(f"{location}.{field.get('name')}: tipo desconhecido '{base}'")
            if "required" in field and not isinstance(field["required"], bool):
                errors.append(f"{location}.{field.get('name')}: required deve ser booleano")

    for item in type_items:
        name = item.get("name")
        kind = item.get("kind")
        if kind == "object":
            validate_fields(item.get("fields"), f"tipo '{name}'")
        elif kind == "enum":
            values = item.get("values")
            if not isinstance(values, list) or not values or len(values) != len(set(values)):
                errors.append(f"tipo '{name}': enum vazio ou duplicado")
        elif kind == "opaque":
            base = _base_type(item.get("lua_type"), f"tipo '{name}'", errors)
            if base not in BUILTIN_TYPES:
                errors.append(f"tipo '{name}': lua_type opaco inválido")
        else:
            errors.append(f"tipo '{name}': kind inválido")

    error_codes = api.get("error_codes", [])
    if not isinstance(error_codes, list) or len(error_codes) != len(set(error_codes)):
        errors.append("api.error_codes: lista inválida ou duplicada")
        error_codes = []
    for function in function_items:
        name = function.get("name")
        if name in function_names and not FUNCTION_NAME.fullmatch(name):
            errors.append(f"função '{name}': nome inválido")
        availability = function.get("availability")
        capability = function.get("capability")
        if availability not in AVAILABILITY:
            errors.append(f"função '{name}': availability inválida")
        if availability != "common" and not capability:
            errors.append(f"função '{name}': recurso específico exige capability")
        if capability:
            definition = capability_map.get(capability)
            if definition is None:
                errors.append(f"função '{name}': capability órfã '{capability}'")
            elif definition.get("status") != "contract":
                errors.append(f"função '{name}': capability '{capability}' ainda não contratada")
            elif availability != "common" and availability not in definition.get("hosts", []):
                errors.append(f"função '{name}': availability excede hosts da capability")
        validate_fields(function.get("arguments", []), f"função '{name}' argumentos")
        result_type = _base_type(function.get("returns"), f"função '{name}' retorno", errors)
        if result_type is not None and result_type not in known_types:
            errors.append(f"função '{name}': retorno desconhecido '{result_type}'")
        for code in function.get("errors", []):
            if code not in error_codes:
                errors.append(f"função '{name}': erro desconhecido '{code}'")

    for event in event_items:
        name = event.get("name")
        if name in event_names and not DOTTED_NAME.fullmatch(name):
            errors.append(f"evento '{name}': nome inválido")
        kind = event.get("kind")
        cancellable = event.get("cancellable")
        if kind not in EVENT_KINDS:
            errors.append(f"evento '{name}': kind inválido")
        if not isinstance(cancellable, bool):
            errors.append(f"evento '{name}': cancellable deve ser booleano")
        if kind == "observe" and cancellable is not False:
            errors.append(f"evento '{name}': observe não pode ser cancelável")
        support = event.get("support")
        if not isinstance(support, list) or not support or set(support) - HOSTS or len(support) != len(set(support)):
            errors.append(f"evento '{name}': support inválido")
            support = []
        capability = event.get("capability")
        if set(support) != HOSTS and not capability:
            errors.append(f"evento '{name}': suporte específico exige capability")
        if capability:
            definition = capability_map.get(capability)
            if definition is None:
                errors.append(f"evento '{name}': capability órfã '{capability}'")
            elif definition.get("status") != "contract":
                errors.append(f"evento '{name}': capability '{capability}' ainda não contratada")
            elif not set(support).issubset(set(definition.get("hosts", []))):
                errors.append(f"evento '{name}': support excede hosts da capability")
        validate_fields(event.get("payload", []), f"evento '{name}' payload")

    return errors


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Valida os schemas públicos ShipLua.")
    parser.add_argument("--schema-dir", type=Path,
                        default=Path(__file__).resolve().parents[1] / "schema")
    args = parser.parse_args()
    try:
        api = load_document(args.schema_dir / "api.yml")
        events = load_document(args.schema_dir / "events.yml")
        capabilities = load_document(args.schema_dir / "capabilities.yml")
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"Schemas inválidos: {error}", file=sys.stderr)
        return 1
    errors = validate_documents(api, events, capabilities)
    if errors:
        for error in errors:
            print(f"Erro: {error}", file=sys.stderr)
        return 1
    print("Schemas ShipLua válidos: API 0.2.0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
