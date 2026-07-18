#include "shiplua/handles/HandleRegistry.h"

namespace ShipLua {

const char* HandleKindName(HandleKind kind) noexcept {
    switch (kind) {
        case HandleKind::Actor:     return "actor";
        case HandleKind::Prop:      return "prop";
        case HandleKind::Light:     return "light";
        case HandleKind::Collision: return "collision";
        case HandleKind::Audio:     return "audio";
        case HandleKind::Vfx:       return "vfx";
        case HandleKind::Camera:    return "camera";
        case HandleKind::Resource:  return "resource";
    }
    return "unknown";
}

std::optional<HandleKind> HandleKindFromName(const std::string& name) noexcept {
    if (name == "actor") return HandleKind::Actor;
    if (name == "prop") return HandleKind::Prop;
    if (name == "light") return HandleKind::Light;
    if (name == "collision") return HandleKind::Collision;
    if (name == "audio") return HandleKind::Audio;
    if (name == "vfx") return HandleKind::Vfx;
    if (name == "camera") return HandleKind::Camera;
    if (name == "resource") return HandleKind::Resource;
    return std::nullopt;
}

HandleRegistry::HandleRegistry(std::uint16_t hostId, HandleLimits limits)
    : mHostId(hostId), mLimits(limits) {}

void HandleRegistry::FreeSlot(Slot& slot) noexcept {
    slot.occupied = false;
    slot.ownerModId.clear();
    ++slot.generation;
    if (slot.generation == 0) {
        // Generation 0 is reserved: a default-constructed Handle must never
        // resolve. Skip it on the (practically unreachable) 32-bit wrap.
        slot.generation = 1;
    }
}

const HandleRegistry::Slot* HandleRegistry::Resolve(const Handle& handle) const {
    if (handle.slot >= mSlots.size()) {
        return nullptr;
    }
    const Slot& slot = mSlots[handle.slot];
    if (!slot.occupied || slot.generation != handle.generation) {
        return nullptr;
    }
    if (handle.sceneGeneration != mSceneGeneration ||
        slot.sceneGeneration != mSceneGeneration) {
        return nullptr;
    }
    return &slot;
}

Result<Handle> HandleRegistry::Create(HandleKind kind, const std::string& ownerModId) {
    if (ownerModId.empty()) {
        return Result<Handle>::err(ErrorCode::InvalidArgument,
                                   "handle owner mod id must not be empty");
    }
    if (mLimits.maxPerMod != 0 && CountForMod(ownerModId) >= mLimits.maxPerMod) {
        return Result<Handle>::err(ErrorCode::ResourceLimit,
                                   "mod '" + ownerModId + "' exceeded its handle limit of " +
                                       std::to_string(mLimits.maxPerMod));
    }
    if (mLimits.maxTotal != 0 && mLive >= mLimits.maxTotal) {
        return Result<Handle>::err(ErrorCode::ResourceLimit,
                                   "handle registry is full (" +
                                       std::to_string(mLimits.maxTotal) + " handles)");
    }

    // Lowest free slot first keeps allocation deterministic across runs.
    std::size_t index = mSlots.size();
    for (std::size_t i = 0; i < mSlots.size(); ++i) {
        if (!mSlots[i].occupied) {
            index = i;
            break;
        }
    }
    if (index == mSlots.size()) {
        mSlots.emplace_back();
    }

    Slot& slot = mSlots[index];
    slot.occupied = true;
    slot.kind = kind;
    slot.ownerModId = ownerModId;
    slot.sceneGeneration = mSceneGeneration;
    ++mLive;

    return Result<Handle>::ok(Handle{kind, static_cast<std::uint32_t>(index),
                                     slot.generation, mSceneGeneration});
}

Result<void> HandleRegistry::Validate(const Handle& handle, HandleKind expectedKind,
                                      const std::string& callingModId) const {
    if (handle.slot >= mSlots.size()) {
        return Result<void>::err(ErrorCode::InvalidHandle, "handle slot is out of range");
    }
    const Slot& slot = mSlots[handle.slot];
    if (!slot.occupied) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "handle refers to a released object");
    }
    if (slot.generation != handle.generation) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "handle generation is stale");
    }
    if (handle.sceneGeneration != mSceneGeneration ||
        slot.sceneGeneration != mSceneGeneration) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "handle belongs to a previous scene");
    }
    if (handle.kind != expectedKind || slot.kind != expectedKind) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 std::string("handle kind mismatch (expected '") +
                                     HandleKindName(expectedKind) + "', got '" +
                                     HandleKindName(handle.kind) + "')");
    }
    if (slot.ownerModId != callingModId) {
        return Result<void>::err(ErrorCode::PermissionDenied,
                                 "handle is owned by mod '" + slot.ownerModId + "'");
    }
    return Result<void>::ok();
}

bool HandleRegistry::IsAlive(const Handle& handle) const {
    return Resolve(handle) != nullptr;
}

Result<void> HandleRegistry::Destroy(const Handle& handle, const std::string& callingModId) {
    // Destroy requires the same guarantees as any control operation: a stale
    // or foreign handle must fail without touching live state.
    const Slot* resolved = Resolve(handle);
    if (resolved == nullptr) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "cannot destroy a stale or released handle");
    }
    if (resolved->kind != handle.kind) {
        return Result<void>::err(ErrorCode::InvalidHandle, "handle kind mismatch");
    }
    if (resolved->ownerModId != callingModId) {
        return Result<void>::err(ErrorCode::PermissionDenied,
                                 "handle is owned by mod '" + resolved->ownerModId + "'");
    }
    FreeSlot(mSlots[handle.slot]);
    --mLive;
    return Result<void>::ok();
}

std::size_t HandleRegistry::InvalidateScene() {
    std::size_t released = 0;
    for (Slot& slot : mSlots) {
        if (slot.occupied) {
            FreeSlot(slot);
            ++released;
        }
    }
    mLive = 0;
    ++mSceneGeneration;
    if (mSceneGeneration == 0) {
        mSceneGeneration = 1;
    }
    return released;
}

std::size_t HandleRegistry::ReleaseMod(const std::string& modId) {
    std::size_t released = 0;
    for (Slot& slot : mSlots) {
        if (slot.occupied && slot.ownerModId == modId) {
            FreeSlot(slot);
            ++released;
        }
    }
    mLive -= released;
    return released;
}

std::size_t HandleRegistry::CountForMod(const std::string& modId) const {
    std::size_t count = 0;
    for (const Slot& slot : mSlots) {
        if (slot.occupied && slot.ownerModId == modId) {
            ++count;
        }
    }
    return count;
}

std::optional<std::string> HandleRegistry::OwnerOf(const Handle& handle) const {
    const Slot* slot = Resolve(handle);
    if (slot == nullptr) {
        return std::nullopt;
    }
    return slot->ownerModId;
}

} // namespace ShipLua
