#include "shiplua/world/WorldSession.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <utility>

namespace ShipLua {

namespace {

constexpr std::uint16_t kMaxHealth = 0x140;
constexpr std::uint32_t kMaxRupees = 9999;
constexpr std::uint32_t kMaxItemQuantity = 9999;
constexpr std::size_t kMaxPortableItems = 256;

bool IsPortableId(const std::string& value) {
    if (value.empty() || value.size() > 128 || !std::islower(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::islower(character) || std::isdigit(character) || character == '.' ||
               character == '_' || character == '-';
    });
}

std::map<std::string, PortableItem> IndexItems(const std::vector<PortableItem>& items) {
    std::map<std::string, PortableItem> indexed;
    for (const PortableItem& item : items) {
        indexed.emplace(item.id, item);
    }
    return indexed;
}

class TravelGuard {
  public:
    explicit TravelGuard(bool& traveling) : mTraveling(traveling) { mTraveling = true; }
    ~TravelGuard() { mTraveling = false; }

  private:
    bool& mTraveling;
};

} // namespace

const char* WorldIdName(WorldId world) noexcept {
    switch (world) {
        case WorldId::Oot: return "oot";
        case WorldId::Mm: return "mm";
    }
    return "unknown";
}

Result<void> ValidatePortablePlayerState(const PortablePlayerState& state) {
    if (state.healthCapacity == 0 || state.healthCapacity > kMaxHealth || state.health > state.healthCapacity) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "portable player health is outside the supported range");
    }
    if (state.rupees > kMaxRupees) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "portable player rupees exceed the shared-session limit");
    }
    if (state.items.size() > kMaxPortableItems) {
        return Result<void>::err(ErrorCode::ResourceLimit,
                                 "portable player inventory exceeds 256 items");
    }

    std::set<std::string> ids;
    for (const PortableItem& item : state.items) {
        if (!IsPortableId(item.id)) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "portable item has an invalid canonical id");
        }
        if (!ids.insert(item.id).second) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "portable inventory contains a duplicate item id");
        }
        if (item.quantity == 0 || item.quantity > kMaxItemQuantity) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "portable item quantity is outside the supported range");
        }
        if (item.visualAsset && !IsPortableId(item.visualAsset->id)) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "portable item has an invalid asset id");
        }
    }
    return Result<void>::ok();
}

Result<void> ValidateWorldDestination(const WorldDestination& destination) {
    if (!IsPortableId(destination.id)) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "world destination has an invalid canonical id");
    }
    return Result<void>::ok();
}

WorldSession::WorldSession() : mOwnerThread(std::this_thread::get_id()) {}

Result<void> WorldSession::CheckOwnerThread() const {
    if (std::this_thread::get_id() != mOwnerThread) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "world session may only run on its owner thread");
    }
    return Result<void>::ok();
}

Result<void> WorldSession::RegisterAdapter(std::shared_ptr<IWorldAdapter> adapter) {
    const auto owner = CheckOwnerThread();
    if (!owner.isOk()) {
        return owner;
    }
    if (!adapter) {
        return Result<void>::err(ErrorCode::InvalidArgument, "world adapter is null");
    }
    if (mActiveWorld) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "world adapters cannot change after session activation");
    }
    const auto inserted = mAdapters.emplace(adapter->Id(), std::move(adapter));
    if (!inserted.second) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "world adapter is already registered");
    }
    return Result<void>::ok();
}

Result<void> WorldSession::SetActiveWorld(WorldId world) {
    const auto owner = CheckOwnerThread();
    if (!owner.isOk()) {
        return owner;
    }
    if (mActiveWorld) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "active world is already initialized");
    }
    if (!mAdapters.contains(world)) {
        return Result<void>::err(ErrorCode::Unsupported,
                                 "active world has no registered adapter");
    }
    mActiveWorld = world;
    mActiveAcceptedItems.clear();
    mActivePartitionKnown = false;
    return Result<void>::ok();
}

Result<PortablePlayerState> WorldSession::CaptureCanonicalState(IWorldAdapter& source) {
    Result<PortablePlayerState> captured;
    try {
        captured = source.CapturePlayerState();
    } catch (...) {
        return Result<PortablePlayerState>::err(ErrorCode::HostFailure,
                                                "world adapter threw while capturing player state");
    }
    if (!captured.isOk()) {
        return captured;
    }
    const auto valid = ValidatePortablePlayerState(*captured.value);
    if (!valid.isOk()) {
        return Result<PortablePlayerState>::err(valid.code, valid.message);
    }

    if (!mCanonicalState || !mActivePartitionKnown) {
        return captured;
    }

    std::map<std::string, PortableItem> merged = IndexItems(mCanonicalState->items);
    for (const std::string& accepted : mActiveAcceptedItems) {
        merged.erase(accepted);
    }
    for (const PortableItem& item : captured.value->items) {
        merged[item.id] = item;
    }

    captured.value->items.clear();
    captured.value->items.reserve(merged.size());
    for (auto& [id, item] : merged) {
        (void)id;
        captured.value->items.push_back(std::move(item));
    }
    const auto mergedValid = ValidatePortablePlayerState(*captured.value);
    if (!mergedValid.isOk()) {
        return Result<PortablePlayerState>::err(mergedValid.code, mergedValid.message);
    }
    return captured;
}

Result<void> WorldSession::ValidatePreview(const PortablePlayerState& state,
                                           const WorldImportPreview& preview,
                                           const IWorldAdapter& target) const {
    std::set<std::string> accepted(preview.acceptedItemIds.begin(), preview.acceptedItemIds.end());
    std::set<std::string> deferred(preview.deferredItemIds.begin(), preview.deferredItemIds.end());
    std::set<std::string> translated(preview.translatedItemIds.begin(),
                                     preview.translatedItemIds.end());
    if (accepted.size() != preview.acceptedItemIds.size() ||
        deferred.size() != preview.deferredItemIds.size() ||
        translated.size() != preview.translatedItemIds.size()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "world adapter returned duplicate item decisions");
    }
    for (const std::string& id : translated) {
        if (!accepted.contains(id)) {
            return Result<void>::err(ErrorCode::InvalidState,
                                     "translated item must also be accepted");
        }
    }

    for (const PortableItem& item : state.items) {
        const bool isAccepted = accepted.contains(item.id);
        const bool isDeferred = deferred.contains(item.id);
        if (isAccepted == isDeferred) {
            return Result<void>::err(ErrorCode::InvalidState,
                                     "world adapter must accept or defer every portable item exactly once");
        }
        if (isAccepted && !translated.contains(item.id) && item.equipped && item.visualAsset &&
            !target.CanResolveAsset(*item.visualAsset)) {
            return Result<void>::err(ErrorCode::Unsupported,
                                     "destination accepted equipped item without resolving its asset");
        }
    }
    if (accepted.size() + deferred.size() != state.items.size()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "world adapter returned decisions for unknown items");
    }
    return Result<void>::ok();
}

Result<WorldTravelResult> WorldSession::Travel(const WorldDestination& destination) {
    const auto owner = CheckOwnerThread();
    if (!owner.isOk()) {
        return Result<WorldTravelResult>::err(owner.code, owner.message);
    }
    if (mTraveling) {
        return Result<WorldTravelResult>::err(ErrorCode::InvalidState,
                                              "world travel cannot reenter");
    }
    const auto validDestination = ValidateWorldDestination(destination);
    if (!validDestination.isOk()) {
        return Result<WorldTravelResult>::err(validDestination.code, validDestination.message);
    }
    if (!mActiveWorld) {
        return Result<WorldTravelResult>::err(ErrorCode::InvalidState,
                                              "world session has no active world");
    }
    const auto sourceIt = mAdapters.find(*mActiveWorld);
    const auto targetIt = mAdapters.find(destination.world);
    if (sourceIt == mAdapters.end() || targetIt == mAdapters.end()) {
        return Result<WorldTravelResult>::err(ErrorCode::Unsupported,
                                              "source or destination adapter is unavailable");
    }

    TravelGuard guard(mTraveling);
    auto canonical = CaptureCanonicalState(*sourceIt->second);
    if (!canonical.isOk()) {
        return Result<WorldTravelResult>::err(canonical.code, canonical.message);
    }

    Result<WorldImportPreview> preview;
    try {
        preview = targetIt->second->PrepareImport(*canonical.value, destination);
    } catch (...) {
        targetIt->second->AbortImport();
        return Result<WorldTravelResult>::err(ErrorCode::HostFailure,
                                              "world adapter threw while preparing import");
    }
    if (!preview.isOk()) {
        targetIt->second->AbortImport();
        return Result<WorldTravelResult>::err(preview.code, preview.message);
    }
    const auto validPreview = ValidatePreview(*canonical.value, *preview.value, *targetIt->second);
    if (!validPreview.isOk()) {
        targetIt->second->AbortImport();
        return Result<WorldTravelResult>::err(validPreview.code, validPreview.message);
    }
    Result<void> committed;
    try {
        committed = targetIt->second->CommitImport();
    } catch (...) {
        targetIt->second->AbortImport();
        return Result<WorldTravelResult>::err(ErrorCode::HostFailure,
                                              "world adapter threw while committing import");
    }
    if (!committed.isOk()) {
        targetIt->second->AbortImport();
        return Result<WorldTravelResult>::err(committed.code, committed.message);
    }

    WorldTravelResult result;
    result.source = *mActiveWorld;
    result.destination = destination;
    result.acceptedItemIds = preview.value->acceptedItemIds;
    result.deferredItemIds = preview.value->deferredItemIds;
    result.translatedItemIds = preview.value->translatedItemIds;
    mCanonicalState = std::move(*canonical.value);
    mActiveWorld = destination.world;
    mActiveAcceptedItems = std::set<std::string>(result.acceptedItemIds.begin(),
                                                 result.acceptedItemIds.end());
    mActivePartitionKnown = true;
    return Result<WorldTravelResult>::ok(std::move(result));
}

std::optional<WorldId> WorldSession::ActiveWorld() const noexcept {
    return mActiveWorld;
}

const std::optional<PortablePlayerState>& WorldSession::CanonicalState() const noexcept {
    return mCanonicalState;
}

std::size_t WorldSession::AdapterCount() const noexcept {
    return mAdapters.size();
}

} // namespace ShipLua
