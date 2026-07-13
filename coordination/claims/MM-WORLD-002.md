# MM-WORLD-002

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / MSVC
- Repository: BaiterYamato/2ship2harkinian
- Branch: agent/MM-WORLD-002-assets
- Started: 2026-07-13T18:35:00-03:00
- Depends on: MM-WORLD-001, WORLD-004
- Files:
  - mm/2s2h/MmWorldAdapter.h
  - mm/2s2h/MmWorldAdapter.cpp
  - extern/ship-lua
  - coordination/claims/MM-WORLD-002.md
  - coordination/handoffs/MM-WORLD-002.md
- Goal:
  - Provar bundles de espada MM pelo ResourceManager e factory Fast.
  - Consultar WorldAssetCatalog em vez de aceitar qualquer prefixo `mm.`.
  - Manter archives estrangeiros desmontados até existir bundle allowlisted e namespaced.
