> English translation of [coordination/claims/CODEGEN-002-003.md](CODEGEN-002-003.md). The Portuguese document remains the canonical project record.

# CODEGEN-002 / CODEGEN-003

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / Python 3.14
- Branch: agent/CODEGEN-002-003-api-docs
- Started: 2026-07-13T01:39:58-03:00
- Depends on: CODEGEN-001 (PR #10; stacked branch)
- Files:
  - tools/generate_api_docs.py
  - generated/lua/shiplua.lua
  - generated/docs/api-reference.md
  - tests/schema/test_generate_api_docs.py
  - tests/CMakeLists.txt
  - docs/api/generated-docs.md
  - coordination/claims/CODEGEN-002-003.md
  - coordination/handoffs/CODEGEN-002-003.md
  - coordination/STATUS.md
- Goal:
  - Generate consumable LuaDoc by Lua Language Server.
  - Generate Markdown reference in Brazilian Portuguese.
  - Derive both exclusively from validated canonical schemas.
  - Detect drift of the two artifacts in CTest.
- Completed: 2026-07-13T01:42:42-03:00
