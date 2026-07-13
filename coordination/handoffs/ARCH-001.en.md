> English translation of [coordination/handoffs/ARCH-001.md](ARCH-001.md). The Portuguese document remains the canonical project record.

# Handoff — ARCH-001 / ARCH-002

## State

review

## Result

- Hooks of the two hosts inventoried in identified commits.
- Differences in frame, scene, save, actors, text and audio recorded.
- Actual bootstrap points located at `OTRGlobals.cpp` and `BenPort.cpp`.
- RFC 0001 defines `IGameAdapter`, initial Lua contract, capabilities, lifecycle and security.
- Exclusive resources remain conditioned on capabilities; no simulation between games.

## Commits

- `13ddf71` — docs(architecture): define host adapters

## Changed files

- `docs/architecture/host-hook-inventory.md`
- `rfcs/0001-shiplua-architecture.md`
- `coordination/claims/ARCH-001.md`
- `coordination/STATUS.md`

## Validation performed

```powershell
rg "DEFINE_HOOK(...)" <Shipwright HookTable>
rg "DEFINE_HOOK(...)" <2Ship HookTable>
ctest --test-dir build-verify --output-on-failure
git diff --check
```

## Validation result

- Central signatures found in the two HookTables observed.
- 10/10 CTest tests passed.
- `git diff --check` no errors.

## Evidence from hosts

- Shipwright: `3bed8cc2f3c1fe67b9fca15e6434551fcec57d0c`.
- 2Ship2Harkinian: `b3cc366288c0a3e7583810be816d5fed3cd54ac2`.

## Pending

- Approval of the RFC by the integrator.
- Implement API-001/API-002/API-003 schemas derived from the approved RFC.
- Revalidate SHAs and signatures immediately before patching hosts.

## Risks

- Forks can advance before integration; check drift must treat SHAs as observed baseline.
- Explicit shutdown of `GameInteractor` was not found; ShipLua needs own ownership/lifecycle.
- This branch depends on PR #5 and should remain stacked until integration.

## Recommended next action

Review the RFC and start API-001, API-002 and API-003 in core.
