# Plans Ledger

This file is the fixed, append-only plan ledger for this repository.

## [PLN-20260716-0001] English ROM-free Link-Span GitHub release
- createdUtc: 2026-07-16T17:59:49Z
- status: in_progress
- scope: mixed
- summary: Finish the English launcher UX, verify automatic single-game selection and dual-game mod gating, build a Release package with no protected assets, then publish code and artifact to GitHub.
- milestones:
  1. Translate every launcher-facing message to English
  2. Extend launcher tests for single-game auto-launch, dual chooser prerequisites, and dual-mod blocking
  3. Build and smoke-test the Release package with synthetic fixtures only
  4. Scan repository and artifact for ROM/O2R/OTR content
  5. Commit, push, open a draft PR, and publish a GitHub prerelease asset
- tags: link-span, launcher, release, rom-free
- refs:
  - src/launcher/main.cpp
  - tests/launcher/AssetDiscoveryTests.cpp
  - docs/link-span-process.md
  - coordination/claims/LINK-004.md

## [PLN-20260716-0001][UPDATE] 2026-07-16T18:29:28Z
- status: done
- note: English launcher policy, dual-game mod gating, ROM-free packaging, Release build, 32/32 tests, visual smoke checks, GitHub branches and draft reviews completed.
- refs:
  - https://github.com/BaiterYamato/link-span/pull/32
  - https://github.com/BaiterYamato/Shipwright-HyliaFoundry/pull/13
  - https://github.com/BaiterYamato/2ship2harkinian/pull/10
  - https://github.com/BaiterYamato/link-span/releases/tag/v0.1.0-alpha.1

## [PLN-20260716-0002] Restore required port archives in ROM-free release
- createdUtc: 2026-07-16T18:44:55Z
- status: in_progress
- scope: mixed
- summary: Fix the public package so it includes redistributable soh.o2r and 2ship.o2r runtime archives while excluding user ROMs and oot.o2r/mm.o2r, then rebuild, smoke-test, and publish a corrected prerelease.
- milestones:
  1. Classify port runtime archives separately from user-derived game archives
  2. Add a packaging regression that requires soh.o2r and 2ship.o2r
  3. Build and scan a corrected Windows x64 Release package
  4. Run both real hosts with synthetic base archive names and confirm the missing-port-archive dialogs are gone
  5. Push the hotfix and publish v0.1.0-alpha.2
- tags: link-span, hotfix, packaging, rom-free
- refs:
  - coordination/claims/LINK-005.md
  - tools/LinkSpanPackaging.psm1
  - tests/tools/BuildLinkSpanPackagingTests.ps1

## [PLN-20260716-0002][UPDATE] 2026-07-16T18:56:51Z
- status: done
- note: Port archives are now required and preserved, ROM/user archives remain excluded, MSVC and 32/32 tests pass, both real hosts started successfully, and the corrected alpha.2 artifact is scanned and ready for publication.
- refs:
  - https://github.com/BaiterYamato/link-span/pull/33
  - https://github.com/BaiterYamato/link-span/releases/tag/v0.1.0-alpha.2
  - coordination/handoffs/LINK-005.md

## [PLN-20260716-0003] Make Link-Span ZIP native-extractor compatible
- createdUtc: 2026-07-16T19:08:02Z
- status: in_progress
- scope: mixed
- summary: Replace the Unix-flavored tar ZIP release path with a Windows ZIP creator and require a complete Expand-Archive round trip before publication, then replace the broken alpha.2 assets.
- milestones:
  1. Add Windows ZIP creation and extraction validation helpers
  2. Add V-LINK-9 regression for native Windows extraction
  3. Generate and fully extract the corrected 9,051-file package
  4. Replace alpha.2 ZIP and checksum on GitHub
  5. Mark plan and handoff complete
- tags: link-span, hotfix, zip, windows, release
- refs:
  - coordination/claims/LINK-006.md
  - tools/LinkSpanPackaging.psm1
  - tests/tools/BuildLinkSpanPackagingTests.ps1
