#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "shiplua/runtime/Result.h"
#include "shiplua/world/WorldSession.h"

namespace ShipLua {

enum class WorldAssetKind {
    EquipmentVisual,
    Scene,
    Texture,
    Audio,
};

struct WorldAssetDescriptor {
    AssetReference asset;
    WorldAssetKind kind = WorldAssetKind::EquipmentVisual;
    std::string factoryContract;
    std::uint32_t contractVersion = 1;
    std::set<WorldId> compatibleHosts;
    bool requiresIsolatedNamespace = true;

    bool operator==(const WorldAssetDescriptor&) const = default;
};

struct WorldAssetProbe {
    AssetReference asset;
    WorldId host = WorldId::Oot;
    std::string factoryContract;
    std::uint32_t contractVersion = 1;
    bool archiveValidated = false;
    bool namespaceIsolated = false;
    bool bundleLoaded = false;
};

struct WorldAssetResolution {
    AssetReference asset;
    WorldId host = WorldId::Oot;
    WorldAssetKind kind = WorldAssetKind::EquipmentVisual;
    std::string factoryContract;
    std::uint32_t contractVersion = 1;

    bool operator==(const WorldAssetResolution&) const = default;
};

class WorldAssetCatalog {
  public:
    Result<void> Register(WorldAssetDescriptor descriptor);
    Result<WorldAssetResolution> RecordProbe(const WorldAssetProbe& probe);

    bool CanResolve(const AssetReference& asset, WorldId host) const noexcept;
    std::optional<WorldAssetResolution> Resolution(const AssetReference& asset, WorldId host) const;
    void ClearHost(WorldId host) noexcept;

    std::size_t DescriptorCount() const noexcept;
    std::size_t ResolutionCount() const noexcept;

  private:
    struct AssetKey {
        WorldId owner = WorldId::Oot;
        std::string id;

        bool operator<(const AssetKey& other) const noexcept;
    };

    struct ResolutionKey {
        AssetKey asset;
        WorldId host = WorldId::Oot;

        bool operator<(const ResolutionKey& other) const noexcept;
    };

    static AssetKey Key(const AssetReference& asset);

    std::map<AssetKey, WorldAssetDescriptor> mDescriptors;
    std::map<ResolutionKey, WorldAssetResolution> mResolutions;
};

WorldAssetCatalog CreateDefaultWorldAssetCatalog();

} // namespace ShipLua
