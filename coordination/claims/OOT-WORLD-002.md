# OOT-WORLD-002

- Status: review
- Agent: codex-windows-01
- Platform: Windows 11 / Codex / MSVC
- Repository: BaiterYamato/Shipwright-HyliaFoundry
- Branch: agent/OOT-WORLD-002-assets
- Started: 2026-07-13T18:35:00-03:00
- Depends on: OOT-WORLD-001, WORLD-004
- Files:
  - soh/soh/OotWorldAdapter.h
  - soh/soh/OotWorldAdapter.cpp
  - extern/ship-lua
  - coordination/claims/OOT-WORLD-002.md
  - coordination/handoffs/OOT-WORLD-002.md
- Goal:
  - Provar bundles de espada OoT pelo ResourceManager e factory Fast.
  - Consultar WorldAssetCatalog em vez de aceitar qualquer prefixo `oot.`.
  - Manter archives estrangeiros desmontados até existir bundle allowlisted e namespaced.
