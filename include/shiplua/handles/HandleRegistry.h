#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

// ---------------------------------------------------------------------------
// MODSDK-002 — Safe runtime handles, ownership and lifecycle (plan-sdk.md §6)
//
// Native object pointers NEVER cross the native/Lua boundary. Every runtime
// object a mod can touch is referenced by a Handle: plain value data with no
// pointer semantics. Lua receives exactly these fields as a table:
//
//     { kind = "actor", slot = 12, generation = 4, scene_generation = 9 }
//
// Handle ownership and lifecycle rules (enforced by HandleRegistry):
//   - every handle belongs to exactly one mod (its creator);
//   - handles are invalidated on scene changes;
//   - stale handles can never control newly created objects (generation ABA
//     guard: freeing a slot bumps its generation, so an old handle never
//     resolves to a different object);
//   - the runtime validates kind, generation, scene generation and owner on
//     every operation;
//   - each mod has a handle limit (HandleLimits::maxPerMod);
//   - objects created by a mod are destroyed when the mod unloads
//     (ReleaseMod) and on scene changes (InvalidateScene);
//   - handles are session-scoped and must NOT be serialized into save files;
//     persistence must use stable game ids re-resolved to fresh handles.
//
// Threading: HandleRegistry is not thread-safe. Per the SDK thread model,
// all mutations run on the main/game thread; adapters must synchronize.
// ---------------------------------------------------------------------------

// Object category a handle refers to. The kind is part of validation: a
// handle minted for one kind never validates as another, so a Lua table can
// be type-checked before any game state is touched.
enum class HandleKind : std::uint8_t {
    Actor,
    Prop,
    Light,
    Collision,
    Audio,
    Vfx,
    Camera,
    Resource,
};

const char* HandleKindName(HandleKind kind) noexcept;
std::optional<HandleKind> HandleKindFromName(const std::string& name) noexcept;

// Opaque value-type reference to a runtime object owned by a mod. This is the
// ONLY object representation allowed to cross into Lua; it contains no
// pointers and no native references.
struct Handle {
    HandleKind kind = HandleKind::Actor;
    std::uint32_t slot = 0;            // registry slot index
    std::uint32_t generation = 0;      // per-slot object generation (ABA guard)
    std::uint32_t sceneGeneration = 0; // scene in which the object was minted

    bool operator==(const Handle&) const = default;
};

struct HandleLimits {
    // Maximum live handles a single mod may own. 0 means no per-mod limit.
    std::size_t maxPerMod = 1024;
    // Maximum live handles in the whole registry. 0 means no total limit.
    std::size_t maxTotal = 16384;
};

// Slot+generation handle registry for one host process.
//
// Storage is a slot vector; each slot carries a monotonically increasing
// generation counter. A Handle resolves only when slot, generation and scene
// generation all match, so:
//   - a destroyed object's handle dies immediately (slot freed, generation
//     bumped);
//   - a recycled slot can never be driven by an old handle (generation moved
//     forward before reuse);
//   - a scene change kills every handle at once (all slots freed and the
//     scene generation bumped).
//
// Ownership is recorded per slot: only the owning mod may validate/destroy
// its handles; everyone else gets ErrorCode::PermissionDenied.
class HandleRegistry {
  public:
    explicit HandleRegistry(std::uint16_t hostId = 0, HandleLimits limits = {});

    HandleRegistry(const HandleRegistry&) = delete;
    HandleRegistry& operator=(const HandleRegistry&) = delete;

    // Host instance that minted these handles (0 = unspecified/standalone).
    // Lets cross-world tooling distinguish handles from different hosts.
    std::uint16_t HostId() const noexcept { return mHostId; }
    const HandleLimits& Limits() const noexcept { return mLimits; }
    std::uint32_t SceneGeneration() const noexcept { return mSceneGeneration; }

    // Mints a handle for `ownerModId`, reusing the lowest free slot
    // (deterministic). Fails with:
    //   ErrorCode::InvalidArgument — empty owner id;
    //   ErrorCode::ResourceLimit   — per-mod or total limit reached.
    Result<Handle> Create(HandleKind kind, const std::string& ownerModId);

    // Full validation for control operations: the handle must resolve to a
    // live object minted in the current scene, match `expectedKind`, and be
    // owned by `callingModId`. Fails with:
    //   ErrorCode::InvalidHandle    — stale, released, wrong-scene or
    //                                 wrong-kind handle;
    //   ErrorCode::PermissionDenied — handle owned by another mod.
    Result<void> Validate(const Handle& handle, HandleKind expectedKind,
                          const std::string& callingModId) const;

    // Structural liveness only (occupied slot + matching generations), with
    // no kind or ownership check. Intended for diagnostics, not for control.
    bool IsAlive(const Handle& handle) const;

    // Destroys the referenced object and frees its slot. Performs the same
    // validation as Validate(), so only the owner can destroy. Destroying an
    // already-stale handle fails with ErrorCode::InvalidHandle.
    Result<void> Destroy(const Handle& handle, const std::string& callingModId);

    // Scene transition: invalidates every live handle and advances the scene
    // generation. Returns the number of destroyed objects.
    std::size_t InvalidateScene();

    // Unload cleanup: destroys every handle owned by `modId` (handles owned
    // by other mods are untouched). Returns the number of destroyed objects.
    std::size_t ReleaseMod(const std::string& modId);

    std::size_t Count() const noexcept { return mLive; }
    std::size_t CountForMod(const std::string& modId) const;
    std::size_t SlotCount() const noexcept { return mSlots.size(); }

    // Owner of a live handle; std::nullopt when the handle does not resolve.
    std::optional<std::string> OwnerOf(const Handle& handle) const;

  private:
    struct Slot {
        std::uint32_t generation = 1; // bumped on every free; 0 is never valid
        bool occupied = false;
        HandleKind kind = HandleKind::Actor;
        std::string ownerModId;
        std::uint32_t sceneGeneration = 0;
    };

    // Resolves to the occupied slot only when generation and scene match.
    const Slot* Resolve(const Handle& handle) const;
    static void FreeSlot(Slot& slot) noexcept;

    std::uint16_t mHostId;
    HandleLimits mLimits;
    std::uint32_t mSceneGeneration = 1;
    std::vector<Slot> mSlots;
    std::size_t mLive = 0;
};

} // namespace ShipLua
