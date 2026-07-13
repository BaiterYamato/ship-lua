> English translation of [docs/api/generated-docs.md](generated-docs.md). The Portuguese document remains the canonical project record.

# Documentation generated from the API

`tools/generate_api_docs.py` produces two versioned artifacts from the
canonical schemas:

- `generated/lua/shiplua.lua`: LuaDoc definitions for autocomplete and analysis
  static in Lua Language Server;
- `generated/docs/api-reference.md`: human reference in Brazilian Portuguese.

LuaDoc includes aliases, objects, event payloads, namespaces, and signatures.
The event parameter is aliased with all canonical names. Markdown
lists types, functions, events, host support, capabilities and errors.

## Update

```powershell
rtk python tools/generate_api_docs.py
```

## Drift check

```powershell
rtk python tools/generate_api_docs.py --check
```

CTest performs the gate and generator tests. Artifacts should not be
manually edited; Durable descriptions belong to schemas.
