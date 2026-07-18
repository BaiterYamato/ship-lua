# LINK-004

- Status: done
- Agent: codex-windows
- Platform: Windows 11 / Codex
- Branch: agent/LINK-003-consolidate
- Started: 2026-07-16T15:00:00-03:00
- Depends on: LINK-003
- Files:
  - src/launcher/**
  - tests/launcher/**
  - tests/tools/**
  - build-linkspan.ps1
  - tools/LinkSpanPackaging.psm1
  - docs/link-span-process.md
  - docs/agents/Plans.md
  - coordination/STATUS.md
  - coordination/claims/LINK-004.md
  - coordination/handoffs/LINK-004.md
- Goal:
  - Publish an English Link-Span launcher release without user ROMs or extracted game assets.
  - Automatically launch the only available game, show a chooser when both are available,
    and block single-game startup when an installed mod explicitly requires both games.
