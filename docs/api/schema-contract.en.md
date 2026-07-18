> English translation of [docs/api/schema-contract.md](schema-contract.md). The Portuguese document remains the canonical project record.

# API schema contract

The files `schema/api.yml`, `schema/events.yml`, and `schema/capabilities.yml` are the canonical source of the ShipLua public API. They use JSON syntax, which is also valid YAML 1.2, to allow validation without external Python dependencies.

## Responsibilities

- `api.yml`: public types, functions and error codes;
- `events.yml`: names, types, payloads, support by host and phase;
- `capabilities.yml`: common and specific catalog, including planned-only features.

A capability with status `planned` is roadmap documentation and cannot be referenced by function or public event. Only status `contract` integrates the `0.1.0` API.

## Declarative IDL

Each function in `api.yml` carries, besides name, arguments, return and errors:

- `version`: SemVer of the API in which the function entered the contract (never later than the document's `api_version`);
- `stability`: `experimental`, `preview`, `stable` or `deprecated` (plan-sdk.md §15.5 taxonomy; `internal` symbols are not part of the public IDL).

## Generated artifacts

| Tool | Artifact |
|---|---|
| `tools/generate_cpp_api.py` | `generated/include/shiplua/generated/ApiBindings.h` (C++ stubs/bindings) |
| `tools/generate_api_docs.py` | `generated/lua/shiplua.lua` (LuaDoc/autocomplete) and `generated/docs/api-reference.md` |
| `tools/generate_api_contracts.py` | `generated/lua/shiplua_validate.lua` (validators), `generated/lua/shiplua_mock.lua` (mock runtime), `generated/tests/api_contract.lua` and `generated/tests/mock_contract.lua` (contract tests), `generated/docs/compatibility-matrix.md` (OoT/MM matrix) |

All of them have a drift gate in ctest (`--check`): editing the schemas without regenerating breaks the test build.

## Automatic invariants

The command below validates the three documents together:

```powershell
python tools/validate_api_schemas.py
```

The validator rejects:

- divergent versions;
- duplicate or invalid names;
- functions without SemVer `version` or with invalid `stability`;
- function `version` later than `api_version`;
- non-existent error types and codes;
- pointers, addresses and internal layouts;
- orphan or still planned capabilities;
- specific resource without capability;
- support for a host incompatible with the capability;
- event `observe` marked as cancelable.

C++ bindings, LuaDoc and Markdown documentation should be generated from these files only. Contract changes require RFC and appropriate SemVer increment.
