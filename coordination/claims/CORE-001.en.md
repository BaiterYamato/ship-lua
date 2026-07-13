> English translation of [coordination/claims/CORE-001.md](CORE-001.md). The Portuguese document remains the canonical project record.

#CORE-001

- Status: review
- Agent: claude-opus-windows-01 (coordinator) + fable-5 (code implementation)
- Platform: Windows 11 / Claude Code / MinGW g++ 15.2 + Ninja
- Branch: main (newly started ship-lua local repo; no remote yet)
- Started: 2026-07-12
- Depends on: ARCH-002 (waived in local bootstrap — architectural decisions follow PLAN.md §5-6)
- Files:
  - CMakeLists.txt
  - cmake/**
  - include/shiplua/runtime/**
  - src/runtime/**
  - tests/unit/**
  - .gitignore
- Goal:
  - Integrate Lua 5.4 and create `LuaRuntime`: initialize and terminate an isolated Lua state.
  - Initial sandbox (CORE-003): remove `io`, `os.execute`, `debug`, `package.loadlib`, FFI.
  - Mod-structured logging (CORE-004) and protected error handling (CORE-005).
  - Unit test proving isolated states + absence of dangerous libs (TEST-001).
- Notes:
  - GOV-001/GOV-002 (forks and public repo on GitHub) NOT claimed — awaiting confirmation from the owner.
