#include "shiplua/world/WorldAssetCatalog.h"

#include <algorithm>
#include <cctype>
#include <tuple>
#include <utility>

namespace ShipLua {
namespace {

bool IsCanonicalId(const std::string& value) {
    if (value.empty() || value.size() > 128 ||
        !std::islower(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::islower(character) || std::isdigit(character) || character == '.' ||
               character == '_' || character == '-';
    });
}

bool IsValidWorld(WorldId world) noexcept { return world == WorldId::Oot || world == WorldId::Mm; }

WorldAssetDescriptor Equipment(std::string id, WorldId owner) {
    return {
        .asset = {owner, std::move(id)},
        .kind = WorldAssetKind::EquipmentVisual,
        .factoryContract = "fast.display_list.bundle",
        .contractVersion = 1,
        .compatibleHosts = {WorldId::Oot, WorldId::Mm},
        .requiresIsolatedNamespace = true,
    };
}

} // namespace

bool WorldAssetCatalog::AssetKey::operator<(const AssetKey& other) const noexcept {
    return std::tie(owner, id) < std::tie(other.owner, other.id);
}

bool WorldAssetCatalog::ResolutionKey::operator<(const ResolutionKey& other) const noexcept {
    return std::tie(asset, host) < std::tie(other.asset, other.host);
}

WorldAssetCatalog::AssetKey WorldAssetCatalog::Key(const AssetReference& asset) {
    return {asset.owner, asset.id};
}

Result<void> WorldAssetCatalog::Register(WorldAssetDescriptor descriptor) {
    const auto asset = ValidateAssetReference(descriptor.asset);
    if (!asset.isOk()) {
        return asset;
    }
    if (!IsCanonicalId(descriptor.factoryContract) || descriptor.contractVersion == 0) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "world asset descriptor has an invalid factory contract");
    }
    if (descriptor.compatibleHosts.empty() ||
        std::any_of(descriptor.compatibleHosts.begin(), descriptor.compatibleHosts.end(),
                    [](WorldId host) { return !IsValidWorld(host); })) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "world asset descriptor has invalid compatible hosts");
    }

    const AssetKey key = Key(descriptor.asset);
    if (mDescriptors.contains(key)) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "world asset descriptor is already registered");
    }
    mDescriptors.emplace(key, std::move(descriptor));
    return Result<void>::ok();
}

Result<WorldAssetResolution> WorldAssetCatalog::RecordProbe(const WorldAssetProbe& probe) {
    const auto asset = ValidateAssetReference(probe.asset);
    if (!asset.isOk()) {
        return Result<WorldAssetResolution>::err(asset.code, asset.message);
    }
    if (!IsValidWorld(probe.host)) {
        return Result<WorldAssetResolution>::err(ErrorCode::InvalidArgument,
                                                 "world asset probe has an unknown host");
    }

    const AssetKey assetKey = Key(probe.asset);
    const auto descriptorIt = mDescriptors.find(assetKey);
    if (descriptorIt == mDescriptors.end()) {
        return Result<WorldAssetResolution>::err(ErrorCode::Unsupported,
                                                 "world asset is not registered");
    }
    const ResolutionKey resolutionKey{assetKey, probe.host};
    mResolutions.erase(resolutionKey);
    const WorldAssetDescriptor& descriptor = descriptorIt->second;

    if (!descriptor.compatibleHosts.contains(probe.host)) {
        return Result<WorldAssetResolution>::err(ErrorCode::Unsupported,
                                                 "world asset is not compatible with this host");
    }
    if (probe.factoryContract != descriptor.factoryContract ||
        probe.contractVersion != descriptor.contractVersion) {
        return Result<WorldAssetResolution>::err(ErrorCode::Unsupported,
                                                 "world asset factory contract does not match");
    }
    if (!probe.archiveValidated) {
        return Result<WorldAssetResolution>::err(ErrorCode::Unsupported,
                                                 "world asset archive was not validated");
    }
    if (descriptor.requiresIsolatedNamespace && !probe.namespaceIsolated) {
        return Result<WorldAssetResolution>::err(ErrorCode::Unsupported,
                                                 "world asset namespace is not isolated");
    }
    if (!probe.bundleLoaded) {
        return Result<WorldAssetResolution>::err(
            ErrorCode::Unsupported, "world asset bundle was not loaded by its factory");
    }

    WorldAssetResolution resolution{
        .asset = descriptor.asset,
        .host = probe.host,
        .kind = descriptor.kind,
        .factoryContract = descriptor.factoryContract,
        .contractVersion = descriptor.contractVersion,
    };
    mResolutions.emplace(resolutionKey, resolution);
    return Result<WorldAssetResolution>::ok(std::move(resolution));
}

bool WorldAssetCatalog::CanResolve(const AssetReference& asset, WorldId host) const noexcept {
    return mResolutions.contains({Key(asset), host});
}

std::optional<WorldAssetResolution> WorldAssetCatalog::Resolution(const AssetReference& asset,
                                                                  WorldId host) const {
    const auto found = mResolutions.find({Key(asset), host});
    if (found == mResolutions.end()) {
        return std::nullopt;
    }
    return found->second;
}

void WorldAssetCatalog::ClearHost(WorldId host) noexcept {
    std::erase_if(mResolutions, [host](const auto& entry) { return entry.first.host == host; });
}

std::size_t WorldAssetCatalog::DescriptorCount() const noexcept { return mDescriptors.size(); }

std::size_t WorldAssetCatalog::ResolutionCount() const noexcept { return mResolutions.size(); }

WorldAssetCatalog CreateDefaultWorldAssetCatalog() {
    WorldAssetCatalog catalog;
    (void)catalog.Register(Equipment("oot.player.sword.kokiri", WorldId::Oot));
    (void)catalog.Register(Equipment("oot.player.sword.master", WorldId::Oot));
    (void)catalog.Register(Equipment("oot.player.sword.biggoron", WorldId::Oot));
    (void)catalog.Register(Equipment("mm.player.sword.kokiri", WorldId::Mm));
    (void)catalog.Register(Equipment("mm.player.sword.razor", WorldId::Mm));
    (void)catalog.Register(Equipment("mm.player.sword.gilded", WorldId::Mm));
    return catalog;
}

} // namespace ShipLua
