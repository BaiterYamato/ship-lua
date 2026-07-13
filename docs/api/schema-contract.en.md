> English translation of [docs/api/schema-contract.md](schema-contract.md). The Portuguese document remains the canonical project record.

# API schema contract

The files `schema/api.yml`, `schema/events.yml`, and `schema/capabilities.yml` are the canonical source of the ShipLua public API. They use JSON syntax, which is also valid YAML 1.2, to allow validation without external Python dependencies.

## Responsibilities

- `api.yml`: public types, functions and error codes;
- `events.yml`: names, types, payloads, support by host and phase;
- `capabilities.yml`: common and specific catalog, including planned-only features.

A capability with status `planned` is roadmap documentation and cannot be referenced by function or public event. Only status `contract` integrates the `0.1.0` API.

## Automatic invariants

The command below validates the three documents together:

```powershell
python tools/validate_api_schemas.py
```

The validator rejects:

- divergent versions;
- duplicate or invalid names;
- non-existent error types and codes;
- pointers, addresses and internal layouts;
- orphan or still planned capabilities;
- specific resource without capability;
- support for a host incompatible with the capability;
- event `observe` marked as cancelable.

C++ bindings, LuaDoc and Markdown documentation should be generated from these files only. Contract changes require RFC and appropriate SemVer increment.
