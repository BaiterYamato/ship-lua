> English translation of [coordination/claims/EXAMPLE-001.md](EXAMPLE-001.md). The Portuguese document remains the canonical project record.

#EXAMPLE-001

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / CMake + MinGW
- Repository: BaiterYamato/ship-lua
- Branch: agent/EXAMPLE-001-hello-world
- Started: 2026-07-13T05:07:30-03:00
- Depends on: MOD-008, BIND-001
- Files:
  - examples/hello-world/**
  - tests/conformance/**
  - tests/CMakeLists.txt
  - CMakeLists.txt
  - .github/workflows/package-examples.yml
  - coordination/claims/EXAMPLE-001.md
  - coordination/handoffs/EXAMPLE-001.md
  - coordination/STATUS.md
- Goal:
  - Version the first common mod with real manifest and entrypoint.
  - Generate `hello-world.shipmod` without including binary artifact in Git.
  - Load the same package through OoT and MM contexts.
  - Prove `game.ready`, identity log and unload without residual callbacks.
  - Publish the checksum-validated package as a CI artifact.
