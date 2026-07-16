# LINK-005

- Status: done
- Agent: codex-windows
- Platform: Windows 11 / Codex
- Branch: hotfix/LINK-005-port-archives
- Started: 2026-07-16T15:45:00-03:00
- Depends on: LINK-004
- Files:
  - tools/LinkSpanPackaging.psm1
  - tests/tools/BuildLinkSpanPackagingTests.ps1
  - build-linkspan.ps1
  - README.md
  - docs/link-span-process.md
  - docs/agents/Plans.md
  - coordination/STATUS.md
  - coordination/claims/LINK-005.md
  - coordination/handoffs/LINK-005.md
- Goal:
  - Keep the redistributable host archives `soh.o2r` and `2ship.o2r` in public
    packages while continuing to exclude ROMs and user-derived game archives.
  - Publish a corrected Windows x64 prerelease and verify both hosts pass their
    startup archive checks with the user's `oot.o2r` and `mm.o2r` beside the launcher.
