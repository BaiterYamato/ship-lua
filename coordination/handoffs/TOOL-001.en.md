> English translation of [coordination/handoffs/TOOL-001.md](TOOL-001.md). The Portuguese document remains the canonical project record.

# Handoff — TOOL-001

## State

review

## Result

- CLI validates TOML file, mod directory or package `.shipmod`.
- Packages use the secure extractor and temporary directory removed by RAII.
- No Lua entrypoints are executed during validation.
- Exit codes: `0` valid, `1` invalid, `2` incorrect use.
- Messages and documentation are presented in Brazilian Portuguese.

## Commits

- `c6705a9` — feat(tool): validate mod manifests

## Changed files

- `tools/manifest_validator.cpp`
- `docs/manifest-validator.md`
- `tests/fixtures/validator/valid-mod/manifest.toml`
- `tests/CMakeLists.txt`
- `CMakeLists.txt`
- `coordination/claims/TOOL-001.md`
- `coordination/STATUS.md`

## Validation performed

```powershell
cmake --build build-verify
ctest --test-dir build-verify --output-on-failure
shiplua_manifest_validator valid-minimal.toml invalid-version.toml
git diff --check
rg --files | rg "\.(z64|n64|v64|o2r)$"
```

## Validation result

- Windows 11 / MinGW g++ 15.2 / Ninja: build completed.
- 10/10 CTest tests passed.
- Manual output displayed in Portuguese with valid and invalid results.
- `git diff --check` no errors and no protected assets found.

## Pending

- Package/install the executable with the releases in TOOL-003/TOOL-004.
- Run tests on Linux and macOS.

## Risks

- Internal diagnostics are reduced to secure messages in Portuguese; Technical details remain in the core `Result`.
- This branch depends on PR #4 and must remain stacked until its integration.

## Recommended next action

Integrate the stack in order and start ARCH-001/ARCH-002 before the public API.
