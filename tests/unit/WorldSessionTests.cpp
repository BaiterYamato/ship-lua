#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "shiplua/world/WorldSession.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::PortableItem Item(std::string id, ShipLua::WorldId origin, bool equipped = false,
                           std::optional<ShipLua::AssetReference> asset = std::nullopt) {
    ShipLua::PortableItem item;
    item.id = std::move(id);
    item.origin = origin;
    item.equipped = equipped;
    item.visualAsset = std::move(asset);
    return item;
}

bool ContainsItem(const ShipLua::PortablePlayerState& state, const std::string& id) {
    return std::any_of(state.items.begin(), state.items.end(),
                       [&](const ShipLua::PortableItem& item) { return item.id == id; });
}

class FakeWorldAdapter final : public ShipLua::IWorldAdapter {
  public:
    explicit FakeWorldAdapter(ShipLua::WorldId world) : world(world) {}

    ShipLua::WorldId Id() const noexcept override { return world; }

    ShipLua::Result<ShipLua::PortablePlayerState> CapturePlayerState() override {
        if (failCapture) {
            return ShipLua::Result<ShipLua::PortablePlayerState>::err(
                ShipLua::ErrorCode::HostFailure, "synthetic capture failure");
        }
        return ShipLua::Result<ShipLua::PortablePlayerState>::ok(liveState);
    }

    bool CanResolveAsset(const ShipLua::AssetReference& asset) const noexcept override {
        return asset.owner == world || mountedAssetWorlds.contains(asset.owner);
    }

    ShipLua::Result<ShipLua::WorldImportPreview> PrepareImport(
        const ShipLua::PortablePlayerState& state,
        const ShipLua::WorldDestination& destination) override {
        ++prepareCalls;
        if (failPrepare || destination.world != world || !destinations.contains(destination.id)) {
            return ShipLua::Result<ShipLua::WorldImportPreview>::err(
                ShipLua::ErrorCode::Unsupported, "synthetic destination unavailable");
        }

        ShipLua::WorldImportPreview preview;
        ShipLua::PortablePlayerState next = state;
        next.items.clear();
        for (const ShipLua::PortableItem& item : state.items) {
            if (supportedItems.contains(item.id)) {
                preview.acceptedItemIds.push_back(item.id);
                if (translatedItems.contains(item.id)) {
                    preview.translatedItemIds.push_back(item.id);
                }
                next.items.push_back(item);
            } else {
                preview.deferredItemIds.push_back(item.id);
            }
        }
        preview.translatedItemIds.insert(preview.translatedItemIds.end(), extraTranslatedItems.begin(),
                                         extraTranslatedItems.end());
        pendingState = std::move(next);
        pendingDestination = destination;
        return ShipLua::Result<ShipLua::WorldImportPreview>::ok(std::move(preview));
    }

    ShipLua::Result<void> CommitImport() override {
        ++commitCalls;
        if (!pendingState) {
            return ShipLua::Result<void>::err(ShipLua::ErrorCode::InvalidState,
                                              "synthetic import was not prepared");
        }
        if (failCommit) {
            return ShipLua::Result<void>::err(ShipLua::ErrorCode::HostFailure,
                                              "synthetic commit failure");
        }
        liveState = std::move(*pendingState);
        lastDestination = pendingDestination;
        pendingState.reset();
        pendingDestination.reset();
        return ShipLua::Result<void>::ok();
    }

    void AbortImport() noexcept override {
        ++abortCalls;
        pendingState.reset();
        pendingDestination.reset();
    }

    ShipLua::WorldId world;
    ShipLua::PortablePlayerState liveState;
    std::set<std::string> destinations;
    std::set<std::string> supportedItems;
    std::set<std::string> translatedItems;
    std::set<std::string> extraTranslatedItems;
    std::set<ShipLua::WorldId> mountedAssetWorlds;
    std::optional<ShipLua::WorldDestination> lastDestination;
    bool failCapture = false;
    bool failPrepare = false;
    bool failCommit = false;
    int prepareCalls = 0;
    int commitCalls = 0;
    int abortCalls = 0;

  private:
    std::optional<ShipLua::PortablePlayerState> pendingState;
    std::optional<ShipLua::WorldDestination> pendingDestination;
};

std::shared_ptr<FakeWorldAdapter> MakeOotAdapter() {
    auto adapter = std::make_shared<FakeWorldAdapter>(ShipLua::WorldId::Oot);
    adapter->destinations = { "oot.kakariko", "oot.market" };
    adapter->supportedItems = { "shared.bow", "shared.sword", "oot.hookshot" };
    adapter->mountedAssetWorlds = { ShipLua::WorldId::Mm };
    adapter->liveState.health = 40;
    adapter->liveState.healthCapacity = 64;
    adapter->liveState.rupees = 123;
    adapter->liveState.items = {
        Item("shared.sword", ShipLua::WorldId::Oot, true,
             ShipLua::AssetReference{ ShipLua::WorldId::Oot, "oot.player.sword" }),
        Item("shared.bow", ShipLua::WorldId::Oot),
        Item("oot.hookshot", ShipLua::WorldId::Oot),
    };
    return adapter;
}

std::shared_ptr<FakeWorldAdapter> MakeMmAdapter() {
    auto adapter = std::make_shared<FakeWorldAdapter>(ShipLua::WorldId::Mm);
    adapter->destinations = { "mm.clock_town", "mm.goron_village" };
    adapter->supportedItems = { "shared.bow", "shared.sword", "mm.mask.deku" };
    adapter->mountedAssetWorlds = { ShipLua::WorldId::Oot };
    return adapter;
}

void TestRoundTripPreservesDeferredEquipmentAndCrossWorldAsset() {
    auto oot = MakeOotAdapter();
    auto mm = MakeMmAdapter();
    ShipLua::WorldSession session;
    Check(session.RegisterAdapter(oot).isOk() && session.RegisterAdapter(mm).isOk(),
          "two world adapters should register in one session");
    Check(session.SetActiveWorld(ShipLua::WorldId::Oot).isOk(),
          "OoT should become the initial active world");

    const auto toMm = session.Travel({ ShipLua::WorldId::Mm, "mm.clock_town" });
    Check(toMm.isOk(), "OoT to Clock Town travel should commit");
    Check(session.ActiveWorld() == ShipLua::WorldId::Mm,
          "MM should become active after commit");
    Check(toMm.value && toMm.value->deferredItemIds == std::vector<std::string>{ "oot.hookshot" },
          "MM should explicitly defer the OoT-only hookshot");
    Check(ContainsItem(mm->liveState, "shared.sword") && !ContainsItem(mm->liveState, "oot.hookshot"),
          "MM live state should receive supported equipment only");
    Check(session.CanonicalState() && ContainsItem(*session.CanonicalState(), "oot.hookshot"),
          "canonical state should retain deferred OoT equipment");

    mm->liveState.items.push_back(Item(
        "mm.mask.deku", ShipLua::WorldId::Mm, true,
        ShipLua::AssetReference{ ShipLua::WorldId::Mm, "mm.player.mask.deku" }));
    mm->liveState.rupees = 321;
    const auto toOot = session.Travel({ ShipLua::WorldId::Oot, "oot.kakariko" });
    Check(toOot.isOk(), "MM to Kakariko return should commit");
    Check(toOot.value && toOot.value->deferredItemIds == std::vector<std::string>{ "mm.mask.deku" },
          "OoT should defer the MM-only mask without deleting it");
    Check(ContainsItem(oot->liveState, "oot.hookshot"),
          "deferred hookshot should reappear when returning to OoT");
    Check(oot->liveState.rupees == 321,
          "portable resources should follow the player across both worlds");
    Check(session.CanonicalState() && ContainsItem(*session.CanonicalState(), "mm.mask.deku"),
          "canonical state should retain MM-only equipment while OoT is active");
}

void TestFailedCommitLeavesSourceActiveAndDestinationUnchanged() {
    auto oot = MakeOotAdapter();
    auto mm = MakeMmAdapter();
    const ShipLua::PortablePlayerState before = mm->liveState;
    mm->failCommit = true;
    ShipLua::WorldSession session;
    session.RegisterAdapter(oot);
    session.RegisterAdapter(mm);
    session.SetActiveWorld(ShipLua::WorldId::Oot);

    const auto travel = session.Travel({ ShipLua::WorldId::Mm, "mm.clock_town" });
    Check(!travel.isOk() && travel.code == ShipLua::ErrorCode::HostFailure,
          "destination commit failure should be reported");
    Check(session.ActiveWorld() == ShipLua::WorldId::Oot,
          "failed travel must keep the source world active");
    Check(mm->liveState == before && mm->abortCalls == 1,
          "failed commit must leave destination state unchanged and abort pending import");
    Check(!session.CanonicalState(),
          "failed initial travel must not publish a partial canonical snapshot");
}

void TestAcceptedEquipmentRequiresResolvableAsset() {
    auto oot = MakeOotAdapter();
    auto mm = MakeMmAdapter();
    mm->mountedAssetWorlds.clear();
    ShipLua::WorldSession session;
    session.RegisterAdapter(oot);
    session.RegisterAdapter(mm);
    session.SetActiveWorld(ShipLua::WorldId::Oot);

    const auto travel = session.Travel({ ShipLua::WorldId::Mm, "mm.clock_town" });
    Check(!travel.isOk() && travel.code == ShipLua::ErrorCode::Unsupported,
          "target cannot accept equipped item before resolving its source-world asset");
    Check(session.ActiveWorld() == ShipLua::WorldId::Oot && mm->abortCalls == 1,
          "asset validation failure must abort before changing active world");
}

void TestTranslatedEquipmentUsesNativeAsset() {
    auto oot = MakeOotAdapter();
    auto mm = MakeMmAdapter();
    mm->mountedAssetWorlds.clear();
    mm->translatedItems.insert("shared.sword");
    ShipLua::WorldSession session;
    session.RegisterAdapter(oot);
    session.RegisterAdapter(mm);
    session.SetActiveWorld(ShipLua::WorldId::Oot);

    const auto travel = session.Travel({ ShipLua::WorldId::Mm, "mm.clock_town" });
    Check(travel.isOk() && travel.value &&
              travel.value->translatedItemIds == std::vector<std::string>{ "shared.sword" },
          "translated equipment should use a target-native asset without mounting the source asset");
}

void TestTranslatedDecisionMustAlsoBeAccepted() {
    auto oot = MakeOotAdapter();
    auto mm = MakeMmAdapter();
    mm->extraTranslatedItems.insert("oot.hookshot");
    ShipLua::WorldSession session;
    session.RegisterAdapter(oot);
    session.RegisterAdapter(mm);
    session.SetActiveWorld(ShipLua::WorldId::Oot);

    const auto travel = session.Travel({ ShipLua::WorldId::Mm, "mm.clock_town" });
    Check(!travel.isOk() && travel.code == ShipLua::ErrorCode::InvalidState,
          "adapter cannot mark a deferred item as translated");
}

void TestWorldAcceptingZeroItemsStillPreservesCanonicalInventory() {
    auto oot = MakeOotAdapter();
    auto mm = MakeMmAdapter();
    mm->supportedItems.clear();
    ShipLua::WorldSession session;
    session.RegisterAdapter(oot);
    session.RegisterAdapter(mm);
    session.SetActiveWorld(ShipLua::WorldId::Oot);

    const auto toMm = session.Travel({ ShipLua::WorldId::Mm, "mm.clock_town" });
    Check(toMm.isOk() && toMm.value && toMm.value->acceptedItemIds.empty(),
          "MM may explicitly defer the complete inventory");
    const auto toOot = session.Travel({ ShipLua::WorldId::Oot, "oot.market" });
    Check(toOot.isOk() && ContainsItem(oot->liveState, "shared.sword") &&
              ContainsItem(oot->liveState, "oot.hookshot"),
          "zero accepted items must not erase canonical inventory on return");
}

void TestValidationAndOwnerThread() {
    ShipLua::PortablePlayerState invalid;
    invalid.items = { Item("shared.sword", ShipLua::WorldId::Oot),
                      Item("shared.sword", ShipLua::WorldId::Mm) };
    Check(!ShipLua::ValidatePortablePlayerState(invalid).isOk(),
          "duplicate canonical item IDs should be rejected");
    Check(!ShipLua::ValidateWorldDestination({ ShipLua::WorldId::Mm, "Clock Town" }).isOk(),
          "destination IDs should be canonical and path-independent");

    auto oot = MakeOotAdapter();
    ShipLua::WorldSession session;
    ShipLua::ErrorCode workerCode = ShipLua::ErrorCode::Ok;
    std::thread worker([&] { workerCode = session.RegisterAdapter(oot).code; });
    worker.join();
    Check(workerCode == ShipLua::ErrorCode::InvalidState,
          "world adapters should only mutate the session on its owner thread");
}

} // namespace

int main() {
    TestRoundTripPreservesDeferredEquipmentAndCrossWorldAsset();
    TestFailedCommitLeavesSourceActiveAndDestinationUnchanged();
    TestAcceptedEquipmentRequiresResolvableAsset();
    TestTranslatedEquipmentUsesNativeAsset();
    TestTranslatedDecisionMustAlsoBeAccepted();
    TestWorldAcceptingZeroItemsStillPreservesCanonicalInventory();
    TestValidationAndOwnerThread();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All cross-world session checks passed\n";
    return 0;
}
