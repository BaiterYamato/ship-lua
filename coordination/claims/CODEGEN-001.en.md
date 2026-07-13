> English translation of [coordination/claims/CODEGEN-001.md](CODEGEN-001.md). The Portuguese document remains the canonical project record.

#CODEGEN-001

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / Python 3.14 / MinGW g++ 15.2
- Branch: agent/CODEGEN-001-cpp-bindings
- Started: 2026-07-13T01:34:03-03:00
- Depends on: API-001/API-002/API-003 (PR #7), TIMER-001 (base stack PR #9)
- Files:
  - tools/generate_cpp_api.py
  - generated/include/shiplua/generated/ApiBindings.h
  - tests/schema/test_generate_cpp_api.py
  - tests/unit/GeneratedApiBindingsTests.cpp
  - tests/CMakeLists.txt
  - CMakeLists.txt
  - docs/api/cpp-codegen.md
  - coordination/claims/CODEGEN-001.md
  - coordination/handoffs/CODEGEN-001.md
  - coordination/STATUS.md
- Goal:
  - Generate deterministic C++ types and metadata from canonical schemas.
  - Reuse validation without duplicating contract rules.
  - Detect drift between schemas and versioned artifacts in CTest.
  - Produce portable output, without pointers or host structures.
- Completed: 2026-07-13T01:37:29-03:00
