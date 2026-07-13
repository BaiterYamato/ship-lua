> English translation of [coordination/handoffs/CODEGEN-001.md](CODEGEN-001.md). The Portuguese document remains the canonical project record.

# Handoff — CODEGEN-001

## State

review

## Result

- Python generator reuses the canonical schema validator.
- Versioned C++ header contains public types, errors, functions, events and
  capabilities.
- Arguments, returns, payloads and per-host support are preserved as
  metadata for the bindings.
- Colliding C++ identifiers are explicitly rejected.
- Gate `--check` and CTest detect drift of the generated artifact.
- C++ test proves that the header is consumable by the project toolchain.

## Commits

- `f1be716` — `feat(codegen): generate C++ API bindings`

## Changed files

- `tools/generate_cpp_api.py`
- `generated/include/shiplua/generated/ApiBindings.h`
- `tests/schema/test_generate_cpp_api.py`
- `tests/unit/GeneratedApiBindingsTests.cpp`
- `tests/CMakeLists.txt`
- `CMakeLists.txt`
- `docs/api/cpp-codegen.md`
- `coordination/claims/CODEGEN-001.md`
- `coordination/handoffs/CODEGEN-001.md`
- `coordination/STATUS.md`

## Validation performed

```powershell
rtk python tools/generate_cpp_api.py --check
rtk python tests/schema/test_generate_cpp_api.py
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk git diff --check
```

## Validation result

- Windows 11, Python 3.14, MinGW g++ 15.2, Ninja, C++20.
- 5/5 Generator Python tests passed.
- Full build completed.
- 17/17 CTest tests passed.
- Synchronized header without pointers or host dependencies.

## Tests not executed

- Linux and macOS.
- Use by real Lua binding, not yet implemented.

## Pending

- `CODEGEN-002`: generate LuaDoc from the same source.
- `CODEGEN-003`: generate Markdown reference from the same source.
- Connect `FunctionId` to the Lua runtime function register.

## Risks

- The order of the generated arrays follows the canonical order of the schemas; changes
  of order produce diff even without semantic change.
- Dynamic types `any` and `callback` remain metadata and will be validated in
  Lua binding, not materialized as C++ fields.

## Recommended next action

Implement `CODEGEN-002` and `CODEGEN-003` in the same generator or in modules
shared, maintaining separate drift gates per artifact.
