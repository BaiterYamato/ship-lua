#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

enum class WorldId {
    Oot,
    Mm,
};

const char* WorldIdName(WorldId world) noexcept;

struct AssetReference {
    WorldId owner = WorldId::Oot;
    std::string id;

    bool operator==(const AssetReference&) const = default;
};

struct PortableItem {
    std::string id;
    WorldId origin = WorldId::Oot;
    std::uint32_t quantity = 1;
    bool equipped = false;
    std::optional<AssetReference> visualAsset;

    bool operator==(const PortableItem&) const = default;
};

struct PortablePlayerState {
    std::uint16_t health = 48;
    std::uint16_t healthCapacity = 48;
    std::uint32_t rupees = 0;
    std::vector<PortableItem> items;

    bool operator==(const PortablePlayerState&) const = default;
};

struct WorldDestination {
    WorldId world = WorldId::Oot;
    std::string id;

    bool operator==(const WorldDestination&) const = default;
};

struct WorldImportPreview {
    std::vector<std::string> acceptedItemIds;
    std::vector<std::string> deferredItemIds;
    // Accepted canonical items materialized as target-native equivalents.
    // Their source-world visual assets are not used by the target.
    std::vector<std::string> translatedItemIds;
};

struct WorldTravelResult {
    WorldId source = WorldId::Oot;
    WorldDestination destination;
    std::vector<std::string> acceptedItemIds;
    std::vector<std::string> deferredItemIds;
    std::vector<std::string> translatedItemIds;
};

class IWorldAdapter {
  public:
    virtual ~IWorldAdapter() = default;

    virtual WorldId Id() const noexcept = 0;
    virtual Result<PortablePlayerState> CapturePlayerState() = 0;
    virtual bool CanResolveAsset(const AssetReference& asset) const noexcept = 0;

    // PrepareImport must not mutate live game state. It reserves a single
    // pending import and partitions every portable item into accepted/deferred.
    virtual Result<WorldImportPreview> PrepareImport(
        const PortablePlayerState& state, const WorldDestination& destination) = 0;

    // CommitImport is atomic from the session's point of view: failure leaves
    // the destination unchanged. AbortImport is idempotent and noexcept.
    virtual Result<void> CommitImport() = 0;
    virtual void AbortImport() noexcept = 0;
};

class WorldSession {
  public:
    WorldSession();

    Result<void> RegisterAdapter(std::shared_ptr<IWorldAdapter> adapter);
    Result<void> SetActiveWorld(WorldId world);
    Result<WorldTravelResult> Travel(const WorldDestination& destination);

    std::optional<WorldId> ActiveWorld() const noexcept;
    const std::optional<PortablePlayerState>& CanonicalState() const noexcept;
    std::size_t AdapterCount() const noexcept;

  private:
    Result<void> CheckOwnerThread() const;
    Result<PortablePlayerState> CaptureCanonicalState(IWorldAdapter& source);
    Result<void> ValidatePreview(const PortablePlayerState& state,
                                 const WorldImportPreview& preview,
                                 const IWorldAdapter& target) const;

    std::thread::id mOwnerThread;
    std::map<WorldId, std::shared_ptr<IWorldAdapter>> mAdapters;
    std::optional<WorldId> mActiveWorld;
    std::optional<PortablePlayerState> mCanonicalState;
    std::set<std::string> mActiveAcceptedItems;
    bool mActivePartitionKnown = false;
    bool mTraveling = false;
};

Result<void> ValidatePortablePlayerState(const PortablePlayerState& state);
Result<void> ValidateAssetReference(const AssetReference& asset);
Result<void> ValidateWorldDestination(const WorldDestination& destination);

} // namespace ShipLua
