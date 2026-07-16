# LINK-006

- Status: done
- Agent: codex-windows
- Platform: Windows 11 / Codex
- Branch: hotfix/LINK-006-windows-zip
- Started: 2026-07-16T16:10:00-03:00
- Depends on: LINK-005
- Files:
  - tools/LinkSpanPackaging.psm1
  - tests/tools/BuildLinkSpanPackagingTests.ps1
  - build-linkspan.ps1
  - README.md
  - docs/link-span-process.md
  - docs/agents/Plans.md
  - coordination/STATUS.md
  - coordination/claims/LINK-006.md
  - coordination/handoffs/LINK-006.md
- Goal:
  - Generate Windows-native ZIP release archives without Unix root entries.
  - Require successful extraction through Windows PowerShell `Expand-Archive`
    before a public artifact can be published.
