# LINK-004 handoff

- Status: review
- Branch: `agent/LINK-003-consolidate`
- Plan: `PLN-20260716-0001`
- Release: `v0.1.0-alpha.1` (prerelease)
- Root review: `BaiterYamato/link-span#32`.
- Release URL: `https://github.com/BaiterYamato/link-span/releases/tag/v0.1.0-alpha.1`.

## Delivered

- English-only launcher dialogs, including a native game chooser.
- Automatic OoT startup when only an OoT asset is present.
- Automatic MM startup when only an MM asset is present.
- Game chooser when both game assets are present.
- Startup blocking with an English warning when an installed unpacked or packaged
  `.shipmod` requires both games but only one game asset is available.
- ROM-free public packaging through `build-linkspan.ps1 -RomFree`.
- Archive safety scan covering `.z64`, `.n64`, `.v64`, `.o2r`, and `.otr`.
- English public-package instructions in `README.md`.

## Validation

- MSVC Release build completed successfully.
- CTest: 32/32 tests passed.
- Visual smoke checks confirmed the English missing-assets dialog, English game
  chooser, and dual-game mod warning.
- Runtime validation from LINK-003 confirmed OoT-to-MM handoff, Shift+F3 logging
  in both hosts, and packaged OoT startup.
- Public ZIP contains 9,083 entries and zero protected archive entries.
- SHA-256: `ca0bb7fef85b8f0a9c53cc5df908d6114ff16178eea342584a82c852c7850d1f`.

## Related host reviews

- OoT runtime: `BaiterYamato/Shipwright-HyliaFoundry#13`.
- MM runtime: `BaiterYamato/2ship2harkinian#10`.

## Remaining review risk

- This is an alpha prerelease while the two host pull requests remain draft and
  stacked on their respective asset branches.
- Public release artifacts intentionally contain no user ROM, O2R, or OTR files.
- The two transient MSVC failures during chooser implementation were mechanical
  include/pragma issues. The repository has no `SPEC.md`; regression coverage is
  provided by the successful MSVC build and launcher visual checks.
