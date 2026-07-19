#!/usr/bin/env python3
"""Gera contratos C++ determinísticos a partir dos schemas públicos ShipLua."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
from validate_api_schemas import load_document, validate_documents  # noqa: E402


def cpp_identifier(value: str) -> str:
    parts = [part for part in re.split(r"[^A-Za-z0-9]+", value) if part]
    identifier = "".join(part[:1].upper() + part[1:] for part in parts)
    if not identifier or identifier[0].isdigit():
        identifier = "Value" + identifier
    return identifier


# C++ reserved words that cannot be used as struct field names. A schema field
# named with one of these is emitted with a trailing underscore so the struct
# compiles; the on-wire/Lua name is unchanged (FieldBinding.name keeps the
# original string).
CPP_KEYWORDS = frozenset({
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
    "class", "compl", "concept", "const", "consteval", "constexpr", "const_cast",
    "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete",
    "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
    "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
    "operator", "or", "or_eq", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
    "static", "static_assert", "static_cast", "struct", "switch", "template",
    "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
    "wchar_t", "while", "xor", "xor_eq",
})


def cpp_field_name(name: str) -> str:
    return name + "_" if name in CPP_KEYWORDS else name


def cpp_string(value: str | None) -> str:
    if value is None:
        return "{}"
    return json.dumps(value, ensure_ascii=False)


def cpp_type(type_name: str, custom_types: dict[str, str]) -> str:
    builtins = {
        "boolean": "bool",
        "integer": "std::int64_t",
        "number": "double",
        "string": "std::string",
    }
    if type_name in builtins:
        return builtins[type_name]
    if type_name in custom_types:
        return custom_types[type_name]
    raise ValueError(f"tipo '{type_name}' não pode ser materializado como campo C++")


def ensure_unique_identifiers(values: list[str], label: str) -> None:
    generated: dict[str, str] = {}
    for value in values:
        identifier = cpp_identifier(value)
        previous = generated.get(identifier)
        if previous is not None:
            raise ValueError(
                f"{label}: '{previous}' e '{value}' geram o mesmo identificador C++ '{identifier}'")
        generated[identifier] = value


def emit_fields(name: str, fields: list[dict[str, Any]]) -> list[str]:
    lines = [f"inline constexpr std::array<FieldBinding, {len(fields)}> {name}{{{{"]
    for field in fields:
        lines.append(
            f"    {{{cpp_string(field['name'])}, {cpp_string(field['type'])}, "
            f"{'true' if field.get('required', True) else 'false'}}},")
    lines.append("}};")
    return lines


def emit_strings(name: str, values: list[str]) -> list[str]:
    lines = [f"inline constexpr std::array<std::string_view, {len(values)}> {name}{{{{"]
    for value in values:
        lines.append(f"    {cpp_string(value)},")
    lines.append("}};")
    return lines


def generate(api: dict[str, Any], events: dict[str, Any],
             capabilities: dict[str, Any]) -> str:
    errors = validate_documents(api, events, capabilities)
    if errors:
        raise ValueError("schemas inválidos:\n- " + "\n- ".join(errors))

    ensure_unique_identifiers([item["name"] for item in api["types"]], "tipos")
    ensure_unique_identifiers([item["name"] for item in api["functions"]], "funções")
    ensure_unique_identifiers([item["name"] for item in events["events"]], "eventos")

    custom_types = {item["name"]: cpp_identifier(item["name"]) for item in api["types"]}
    lines = [
        "// Gerado por tools/generate_cpp_api.py. Não edite manualmente.",
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstdint>",
        "#include <optional>",
        "#include <span>",
        "#include <string>",
        "#include <string_view>",
        "",
        "namespace ShipLua::Generated {",
        "",
        f"inline constexpr std::string_view kApiVersion = {cpp_string(api['api_version'])};",
        f"inline constexpr std::uint32_t kSchemaVersion = {api['schema_version']};",
        "",
    ]

    for item in api["types"]:
        name = custom_types[item["name"]]
        kind = item["kind"]
        if kind == "enum":
            lines.append(f"enum class {name} {{")
            lines.extend(f"    {cpp_identifier(value)}," for value in item["values"])
            lines.extend(["};", ""])
        elif kind == "opaque":
            lines.extend([f"using {name} = std::uint64_t;", ""])
        elif kind == "object":
            lines.append(f"struct {name} {{")
            for field in item["fields"]:
                field_type = cpp_type(field["type"], custom_types)
                if not field.get("required", True):
                    field_type = f"std::optional<{field_type}>"
                lines.append(f"    {field_type} {cpp_field_name(field['name'])};")
            lines.extend(["};", ""])

    lines.extend([
        "enum class ApiError {",
        *[f"    {cpp_identifier(code)}," for code in api["error_codes"]],
        "};",
        "",
        "enum class EventKind { Observe, Filter, Transform, Consume };",
        "",
        "struct FieldBinding {",
        "    std::string_view name;",
        "    std::string_view type;",
        "    bool required;",
        "};",
        "",
        "enum class FunctionId {",
        *[f"    {cpp_identifier(item['name'])}," for item in api["functions"]],
        "};",
        "",
        "struct FunctionBinding {",
        "    FunctionId id;",
        "    std::string_view name;",
        "    std::string_view version;",
        "    std::string_view stability;",
        "    std::string_view returnType;",
        "    std::string_view errorMode;",
        "    std::string_view errorType;",
        "    std::string_view availability;",
        "    std::string_view capability;",
        "    std::span<const FieldBinding> arguments;",
        "    std::span<const std::string_view> errors;",
        "};",
        "",
    ])
    for function in api["functions"]:
        stem = "k" + cpp_identifier(function["name"])
        lines.extend(emit_fields(stem + "Arguments", function.get("arguments", [])))
        lines.extend(emit_strings(stem + "Errors", function.get("errors", [])))
    lines.extend(["", f"inline constexpr std::array<FunctionBinding, {len(api['functions'])}> kFunctions{{{{"])
    for function in api["functions"]:
        stem = "k" + cpp_identifier(function["name"])
        lines.append(
            f"    {{FunctionId::{cpp_identifier(function['name'])}, {cpp_string(function['name'])}, "
            f"{cpp_string(function['version'])}, {cpp_string(function['stability'])}, "
            f"{cpp_string(function['returns'])}, "
            f"{cpp_string(function.get('error_mode', 'raise'))}, "
            f"{cpp_string(function.get('error_type'))}, "
            f"{cpp_string(function['availability'])}, "
            f"{cpp_string(function.get('capability'))}, {stem}Arguments, {stem}Errors}},")
    lines.extend(["}};", ""])

    lines.extend([
        "struct EventBinding {",
        "    std::string_view name;",
        "    EventKind kind;",
        "    std::string_view phase;",
        "    bool cancellable;",
        "    bool supportsOot;",
        "    bool supportsMm;",
        "    std::string_view capability;",
        "    std::span<const FieldBinding> payload;",
        "};",
        "",
    ])
    for event in events["events"]:
        lines.extend(emit_fields("k" + cpp_identifier(event["name"]) + "Payload",
                                 event.get("payload", [])))
    lines.extend(["", f"inline constexpr std::array<EventBinding, {len(events['events'])}> kEvents{{{{"])
    for event in events["events"]:
        hosts = set(event["support"])
        lines.append(
            f"    {{{cpp_string(event['name'])}, EventKind::{cpp_identifier(event['kind'])}, "
            f"{cpp_string(event['phase'])}, {'true' if event['cancellable'] else 'false'}, "
            f"{'true' if 'oot' in hosts else 'false'}, {'true' if 'mm' in hosts else 'false'}, "
            f"{cpp_string(event.get('capability'))}, k{cpp_identifier(event['name'])}Payload}},")
    lines.extend(["}};", ""])

    lines.extend([
        "struct CapabilityBinding {",
        "    std::string_view name;",
        "    std::string_view status;",
        "    bool supportsOot;",
        "    bool supportsMm;",
        "};",
        "",
        f"inline constexpr std::array<CapabilityBinding, {len(capabilities['capabilities'])}> kCapabilities{{{{",
    ])
    for capability in capabilities["capabilities"]:
        hosts = set(capability["hosts"])
        lines.append(
            f"    {{{cpp_string(capability['name'])}, {cpp_string(capability['status'])}, "
            f"{'true' if 'oot' in hosts else 'false'}, {'true' if 'mm' in hosts else 'false'}}},")
    lines.extend(["}};", "", "} // namespace ShipLua::Generated", ""])
    return "\n".join(lines)


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Gera bindings C++ dos schemas ShipLua.")
    root = Path(__file__).resolve().parents[1]
    parser.add_argument("--schema-dir", type=Path, default=root / "schema")
    parser.add_argument("--output", type=Path,
                        default=root / "generated/include/shiplua/generated/ApiBindings.h")
    parser.add_argument("--check", action="store_true",
                        help="falha se o arquivo gerado estiver ausente ou divergente")
    args = parser.parse_args()
    try:
        api = load_document(args.schema_dir / "api.yml")
        events = load_document(args.schema_dir / "events.yml")
        capabilities = load_document(args.schema_dir / "capabilities.yml")
        content = generate(api, events, capabilities)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"Falha ao gerar bindings C++: {error}", file=sys.stderr)
        return 1

    if args.check:
        try:
            current = args.output.read_text(encoding="utf-8")
        except OSError:
            print(f"Binding C++ ausente: {args.output}", file=sys.stderr)
            return 1
        if current != content:
            print("Binding C++ diverge dos schemas; execute tools/generate_cpp_api.py",
                  file=sys.stderr)
            return 1
        print("Binding C++ sincronizado com os schemas.")
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(content, encoding="utf-8", newline="\n")
    print(f"Binding C++ gerado: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
