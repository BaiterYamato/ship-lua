#include "shiplua/lifecycle/ModLifecycle.h"

#include <utility>

namespace ShipLua {

const char* ModLifecyclePhaseName(ModLifecyclePhase phase) noexcept {
    switch (phase) {
        case ModLifecyclePhase::Init:      return "init";
        case ModLifecyclePhase::Loading:   return "loading";
        case ModLifecyclePhase::Active:    return "active";
        case ModLifecyclePhase::Unloading: return "unloading";
        case ModLifecyclePhase::Unloaded:  return "unloaded";
        case ModLifecyclePhase::Failed:    return "failed";
    }
    return "unknown";
}

ModLifecycle::ModLifecycle(std::string modId, HandleRegistry& handles, Logger logger)
    : mModId(std::move(modId)), mHandles(handles), mLogger(std::move(logger)) {}

bool ModLifecycle::CanMintHandles() const noexcept {
    return mPhase == ModLifecyclePhase::Loading || mPhase == ModLifecyclePhase::Active;
}

Result<Handle> ModLifecycle::MintHandle(HandleKind kind) {
    if (!CanMintHandles()) {
        return Result<Handle>::err(
            ErrorCode::InvalidState,
            "mod '" + mModId + "' cannot create handles in phase '" +
                ModLifecyclePhaseName(mPhase) + "'");
    }
    return mHandles.Create(kind, mModId);
}

Result<void> ModLifecycle::Transition(ModLifecyclePhase next) {
    auto legal = false;
    switch (next) {
        case ModLifecyclePhase::Loading:
            legal = mPhase == ModLifecyclePhase::Init;
            break;
        case ModLifecyclePhase::Active:
            legal = mPhase == ModLifecyclePhase::Loading;
            break;
        case ModLifecyclePhase::Failed:
            legal = mPhase == ModLifecyclePhase::Loading ||
                    mPhase == ModLifecyclePhase::Active;
            break;
        case ModLifecyclePhase::Unloading:
            legal = mPhase == ModLifecyclePhase::Init ||
                    mPhase == ModLifecyclePhase::Loading ||
                    mPhase == ModLifecyclePhase::Active ||
                    mPhase == ModLifecyclePhase::Failed;
            break;
        case ModLifecyclePhase::Unloaded:
            legal = mPhase == ModLifecyclePhase::Unloading;
            break;
        case ModLifecyclePhase::Init:
            break; // Init is only the construction state, never a target.
    }
    if (!legal) {
        return Result<void>::err(
            ErrorCode::InvalidState,
            "mod '" + mModId + "' cannot transition from '" +
                ModLifecyclePhaseName(mPhase) + "' to '" + ModLifecyclePhaseName(next) + "'");
    }
    const ModLifecyclePhase from = mPhase;
    mPhase = next;
    LogTransition(from, next);
    return Result<void>::ok();
}

void ModLifecycle::LogTransition(ModLifecyclePhase from, ModLifecyclePhase to) const {
    mLogger.debug(mModId, std::string("lifecycle: ") + ModLifecyclePhaseName(from) + " -> " +
                              ModLifecyclePhaseName(to));
}

Result<void> ModLifecycle::BeginLoad() {
    return Transition(ModLifecyclePhase::Loading);
}

Result<void> ModLifecycle::CompleteLoad() {
    return Transition(ModLifecyclePhase::Active);
}

Result<void> ModLifecycle::Fail(const std::string& reason) {
    const auto result = Transition(ModLifecyclePhase::Failed);
    if (result.isOk()) {
        mLastError = reason;
        mLogger.error(mModId, "lifecycle failure: " + reason);
    }
    return result;
}

Result<void> ModLifecycle::BeginUnload() {
    return Transition(ModLifecyclePhase::Unloading);
}

Result<void> ModLifecycle::CompleteUnload() {
    const auto result = Transition(ModLifecyclePhase::Unloaded);
    if (result.isOk()) {
        // Ownership contract: nothing the mod created outlives its unload.
        const std::size_t released = mHandles.ReleaseMod(mModId);
        if (released != 0) {
            mLogger.info(mModId, "lifecycle: released " + std::to_string(released) +
                                     " handle(s) on unload");
        }
    }
    return result;
}

ModLifecycleManager::ModLifecycleManager(std::uint16_t hostId, HandleLimits limits,
                                         Logger logger)
    : mLogger(std::move(logger)), mHandles(hostId, limits) {}

Result<void> ModLifecycleManager::RegisterMod(const std::string& modId) {
    if (modId.empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument, "mod id must not be empty");
    }
    if (mMods.find(modId) != mMods.end()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "mod '" + modId + "' is already registered");
    }
    mMods.try_emplace(modId, modId, mHandles, mLogger);
    return Result<void>::ok();
}

ModLifecycle* ModLifecycleManager::Find(const std::string& modId) {
    const auto it = mMods.find(modId);
    return it == mMods.end() ? nullptr : &it->second;
}

const ModLifecycle* ModLifecycleManager::Find(const std::string& modId) const {
    const auto it = mMods.find(modId);
    return it == mMods.end() ? nullptr : &it->second;
}

Result<void> ModLifecycleManager::UnloadMod(const std::string& modId) {
    ModLifecycle* lifecycle = Find(modId);
    if (lifecycle == nullptr) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "mod '" + modId + "' is not registered");
    }
    if (lifecycle->Phase() == ModLifecyclePhase::Unloaded) {
        return Result<void>::ok(); // idempotent: cleanup already happened
    }
    const auto begun = lifecycle->BeginUnload();
    if (!begun.isOk()) {
        return begun;
    }
    return lifecycle->CompleteUnload();
}

Result<void> ModLifecycleManager::UnregisterMod(const std::string& modId) {
    ModLifecycle* lifecycle = Find(modId);
    if (lifecycle == nullptr) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "mod '" + modId + "' is not registered");
    }
    if (lifecycle->Phase() != ModLifecyclePhase::Unloaded) {
        const auto unloaded = UnloadMod(modId);
        if (!unloaded.isOk()) {
            return unloaded;
        }
    }
    mMods.erase(modId);
    return Result<void>::ok();
}

std::size_t ModLifecycleManager::OnSceneChange() {
    return mHandles.InvalidateScene();
}

} // namespace ShipLua
