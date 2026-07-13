> English translation of [coordination/claims/TOOL-001.md](TOOL-001.md). The Portuguese document remains the canonical project record.

#TOOL-001

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / MinGW g++ 15.2 + Ninja
- Branch: agent/TOOL-001-manifest-validator
- Started: 2026-07-13T00:00:00-03:00
- Depends on: MOD-003/MOD-004 (PRs #2/#3; branch stacked on #4)
- Files:
  - tools/manifest_validator.cpp
  - docs/manifest-validator.md
  - tests/CMakeLists.txt
  - tests/fixtures/validator/**
  - CMakeLists.txt
  - coordination/claims/TOOL-001.md
  - coordination/handoffs/TOOL-001.md
  - coordination/STATUS.md
- Goal:
  - Validate manifest in file, directory or package `.shipmod`.
  - Reuse canonical parser/extractor without executing the Lua entrypoint.
  - Return different exit codes for success, invalid and incorrect use.
  - Display interface and documentation in Brazilian Portuguese.
