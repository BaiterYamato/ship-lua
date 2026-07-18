#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "shiplua/handles/HandleRegistry.h"
#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

// ---------------------------------------------------------------------------
// MODSDK-002 — Mod lifecycle contract (plan-sdk.md §6, Fase 0)
//
// Every mod moves through a documented init/load/unload lifecycle. The
// lifecycle owns the moment when handle-minting is allowed and guarantees
// that every resource a mod created dies before the mod is gone.
//
// State machine (legal transitions):
//
//                    RegisterMod()
//                         │
//                         ▼
//                 +---------------+
//                 |     Init      | runtime reserved; mod has not run code
//                 +---------------+ and owns no handles yet
//                         │ BeginLoad()
//                         ▼
//                 +---------------+
//                 |    Loading    | entrypoint executing; MAY mint handles
//                 +---------------+
//                    │         │
//   CompleteLoad()   │         │ Fail(reason)
//                    ▼         ▼
//            +-----------+  +-----------+
//            |  Active   |  |  Failed   | load/run failure; handles already
//            +-----------+  +-----------+ minted stay tracked until unload
//                    │         │
//                    │ BeginUnload() (also from Init, Loading or Failed)
//                    ▼
//             +--------------+
//             |  Unloading   | teardown in progress; MUST NOT mint handles
//             +--------------+
//                    │ CompleteUnload()
//                    ▼
//             +--------------+
//             |   Unloaded   | terminal for this load cycle; every handle
//             +--------------+ owned by the mod has been released
//
// Rules enforced by ModLifecycle:
//   - handles may only be minted in Loading or Active (MintHandle checks);
//   - unload (BeginUnload → CompleteUnload) is legal from every phase except
//     Unloaded, so failed or half-loaded mods still get deterministic cleanup;
//   - CompleteUnload releases every handle the mod owns in the shared
//     HandleRegistry;
//   - scene changes invalidate all handles regardless of phase and are
//     applied registry-wide (ModLifecycleManager::OnSceneChange);
//   - Unloaded is terminal: hot reload means UnregisterMod + RegisterMod,
//     i.e. a brand-new load cycle whose fresh handles can never be confused
//     with the dead ones from the previous cycle.
//
// Threading: not thread-safe; all transitions run on the main/game thread.
// ---------------------------------------------------------------------------

enum class ModLifecyclePhase : std::uint8_t {
    Init,
    Loading,
    Active,
    Unloading,
    Unloaded,
    Failed,
};

const char* ModLifecyclePhaseName(ModLifecyclePhase phase) noexcept;

// Tracks one mod's lifecycle phase and ties it to the shared handle
// registry. Instances are created and owned by ModLifecycleManager.
class ModLifecycle {
  public:
    ModLifecycle(std::string modId, HandleRegistry& handles, Logger logger = {});

    ModLifecycle(const ModLifecycle&) = delete;
    ModLifecycle& operator=(const ModLifecycle&) = delete;

    const std::string& ModId() const noexcept { return mModId; }
    ModLifecyclePhase Phase() const noexcept { return mPhase; }
    const std::string& LastError() const noexcept { return mLastError; }

    // True only in Loading and Active — the phases where the mod runs code
    // and is allowed to create runtime objects.
    bool CanMintHandles() const noexcept;

    // Phase-checked handle creation: fails with ErrorCode::InvalidState
    // outside Loading/Active, otherwise forwards to the registry (ownership
    // is always recorded as this mod).
    Result<Handle> MintHandle(HandleKind kind);

    // init → loading. Illegal from any other phase (ErrorCode::InvalidState).
    Result<void> BeginLoad();
    // loading → active.
    Result<void> CompleteLoad();
    // loading|active → failed, recording `reason` for diagnostics.
    Result<void> Fail(const std::string& reason);
    // init|loading|active|failed → unloading.
    Result<void> BeginUnload();
    // unloading → unloaded; releases every handle owned by this mod.
    Result<void> CompleteUnload();

  private:
    Result<void> Transition(ModLifecyclePhase next);
    void LogTransition(ModLifecyclePhase from, ModLifecyclePhase to) const;

    std::string mModId;
    HandleRegistry& mHandles;
    Logger mLogger;
    ModLifecyclePhase mPhase = ModLifecyclePhase::Init;
    std::string mLastError;
};

// Owns the shared HandleRegistry plus the lifecycle of every registered mod.
// This is the integration point hosts use:
//   RegisterMod → BeginLoad → CompleteLoad   (load path)
//   UnloadMod / UnregisterMod                (unload path, auto-cleanup)
//   OnSceneChange                            (scene transition from the game)
class ModLifecycleManager {
  public:
    explicit ModLifecycleManager(std::uint16_t hostId = 0, HandleLimits limits = {},
                                 Logger logger = {});

    ModLifecycleManager(const ModLifecycleManager&) = delete;
    ModLifecycleManager& operator=(const ModLifecycleManager&) = delete;

    HandleRegistry& Handles() noexcept { return mHandles; }
    const HandleRegistry& Handles() const noexcept { return mHandles; }

    // Registers a mod in Init phase. Fails with ErrorCode::InvalidArgument
    // for an empty id and ErrorCode::InvalidState for a duplicate.
    Result<void> RegisterMod(const std::string& modId);

    // nullptr when the mod is not registered.
    ModLifecycle* Find(const std::string& modId);
    const ModLifecycle* Find(const std::string& modId) const;

    // Drives the mod to Unloaded (BeginUnload + CompleteUnload), releasing
    // all of its handles. Idempotent: an already-Unloaded mod is a no-op.
    // Fails with ErrorCode::InvalidHandle when the mod is not registered.
    Result<void> UnloadMod(const std::string& modId);

    // Unloads (when needed) and removes the mod entirely. This is the hot
    // reload / shutdown path: a later RegisterMod starts a fresh load cycle.
    Result<void> UnregisterMod(const std::string& modId);

    // Scene transition: invalidates every live handle in the registry and
    // returns how many objects were destroyed.
    std::size_t OnSceneChange();

    std::size_t Count() const { return mMods.size(); }

  private:
    Logger mLogger;
    HandleRegistry mHandles;
    std::map<std::string, ModLifecycle> mMods;
};

} // namespace ShipLua
