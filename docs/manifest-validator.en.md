> English translation of [docs/manifest-validator.md](manifest-validator.md). The Portuguese document remains the canonical project record.

# Manifest validator

The `shiplua_manifest_validator` executable checks manifests without running the mod's Lua code.

## Accepted entries

- direct path to a `manifest.toml`;
- directory containing `manifest.toml`;
- `.shipmod` package in ZIP format.

Packages are extracted into a temporary directory with the same security locks as the runtime. Temporary content is removed at the end of validation.

## Usage

```powershell
shiplua_manifest_validator path\to\manifest.toml
shiplua_manifest_validator path\to\mod
shiplua_manifest_validator path\to\mod.shipmod
```

Multiple inputs can be checked in the same run.

## Exit codes

| Code | Meaning |
|---:|---|
| `0` | all entries are valid |
| `1` | one or more entries are invalid |
| `2` | no path was given |

The validator does not load the entrypoint, does not create a Lua state and does not resolve installed dependencies. It only checks the local contract of each manifest and the structural security of the package.
