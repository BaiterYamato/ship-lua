> English translation of [docs/shipmod-cli.md](shipmod-cli.md). The Portuguese document remains the canonical project record.

# `shipmod` development CLI

`shipmod` is the ShipLua mod development CLI (plan-sdk.md §16, release
`v0.1.0-alpha.1` §29). It is a Python script at `tools/shipmod.py` — with no
external dependencies — that orchestrates the repository's compiled tools.

```powershell
python tools/shipmod.py <subcommand> [options]
```

Subcommands available in this release: `new`, `validate`, `test` and `doctor`.

## `shipmod new <name>`

Creates the scaffold for a new mod modeled after `examples/`:

```powershell
python tools/shipmod.py new kafei-puppet --dir mods
```

Structure generated under `mods/kafei-puppet/`:

```text
manifest.toml              # id community.kafei_puppet, api ">=0.1 <0.3"
main.lua                   # minimal entrypoint with game.ready + log
README.md                  # validation and test instructions
tests/kafei_puppet_test.lua  # smoke test in the describe/it/assert DSL
```

Options: `--dir` (parent directory), `--id` (mod id), `--autor`/`--author`
(defaults to git `user.name`), `--desc` (short description).

The name must be a lowercase slug (`kafei-puppet`); the derived id uses
underscores (`community.kafei_puppet`). The scaffold refuses to write into a
non-empty directory.

## `shipmod validate <path> [...]`

Validates mod manifests — a directory with `manifest.toml`, a TOML file or a
`.shipmod` package — delegating to the compiled `shiplua_manifest_validator`:

```powershell
python tools/shipmod.py validate examples/hello-runtime
python tools/shipmod.py validate build/examples/hello-world.shipmod
```

When the canonical validator has not been built, the CLI falls back to a basic
structural check in Python (required fields, SemVer and `games`), labeled in
the output as a basic check. The executable is searched in the
`SHIPLUA_MANIFEST_VALIDATOR` variable, the repository's `build*` directories
and the `PATH`.

## `shipmod test <path>`

Runs the mod's tests (`tests/*.lua`) on the mock runtime, without game or ROM,
delegating to `shiplua_mod_test_runner` (MODSDK-004):

```powershell
python tools/shipmod.py test examples/hello-runtime --game oot
python tools/shipmod.py test mods/kafei-puppet --capability core.input
```

The `--game` and `--capability` options are forwarded to the runner. Until
MODSDK-004 lands on `main`, the subcommand fails with exit code `3` and build
instructions; an existing executable can be pointed to with the
`SHIPLUA_MOD_TEST_RUNNER` variable.

## `shipmod doctor`

Diagnoses the development environment and repository health:

```powershell
python tools/shipmod.py doctor
```

Checks:

- Python, git, CMake (>= 3.20), Ninja and C++ compiler versions;
- repo Python tools present under `tools/`;
- `shiplua_manifest_validator` and `shiplua_mod_test_runner` built;
- valid API schemas (`tools/validate_api_schemas.py`);
- drift-free codegen (`generate_cpp_api.py --check`, `generate_api_docs.py --check`);
- valid `examples/` manifests.

Each check reports `ok`, `aviso` (warning) or `falha` (failure); the command
exits with code `1` if there is at least one failure.

## Exit codes

| Code | Meaning |
|---:|---|
| `0` | success |
| `1` | failure (invalid manifest, failing tests, failing doctor checks) |
| `2` | invalid usage (arguments) |
| `3` | required tool missing (e.g. `shiplua_mod_test_runner`) |
