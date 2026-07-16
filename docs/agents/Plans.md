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
