#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "shiplua/runtime/Result.h"
#include "shiplua/world/WorldSession.h"

namespace ShipLua {

enum class PortableItemCategory {
  Weapon,
  Tool,
  Consumable,
  Instrument,
  Transformation,
};

struct PortableItemDefinition {
  std::string id;
  PortableItemCategory category = PortableItemCategory::Tool;
  std::set<WorldId> nativeWorlds;
  std::map<WorldId, std::string> translations;
  std::uint32_t maxQuantity = 1;
  bool equipable = false;
  bool requiresVisualAssetWhenEquipped = false;
};

struct PortableItemResolution {
  std::string canonicalId;
  std::string targetId;
  bool translated = false;
};

class PortableItemCatalog {
public:
  Result<void> Register(PortableItemDefinition definition);

  const PortableItemDefinition *Find(const std::string &id) const noexcept;
  Result<PortableItemResolution> ResolveForWorld(const std::string &id,
                                                 WorldId target) const;
  Result<void> Validate(const PortableItem &item) const;
  std::size_t Size() const noexcept;

  static Result<PortableItemCatalog> CreateDefault();

private:
  std::map<std::string, PortableItemDefinition> mDefinitions;
};

} // namespace ShipLua
