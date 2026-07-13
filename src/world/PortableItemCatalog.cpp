#include "shiplua/world/PortableItemCatalog.h"

#include <array>
#include <utility>

namespace ShipLua {
namespace {

bool IsCanonicalId(const std::string &id) {
  if (id.empty() || id.size() > 128 || id.front() == '.' || id.back() == '.') {
    return false;
  }
  bool previousDot = false;
  for (const char c : id) {
    const bool valid = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                       c == '_' || c == '-';
    if (c == '.') {
      if (previousDot) {
        return false;
      }
      previousDot = true;
    } else {
      if (!valid) {
        return false;
      }
      previousDot = false;
    }
  }
  return true;
}

PortableItemDefinition Shared(std::string id, PortableItemCategory category,
                              std::uint32_t maxQuantity,
                              bool equipable = false) {
  PortableItemDefinition definition;
  definition.id = std::move(id);
  definition.category = category;
  definition.nativeWorlds = {WorldId::Oot, WorldId::Mm};
  definition.maxQuantity = maxQuantity;
  definition.equipable = equipable;
  return definition;
}

PortableItemDefinition Sword(std::string id, WorldId owner,
                             std::string targetId, WorldId target) {
  PortableItemDefinition definition;
  definition.id = std::move(id);
  definition.category = PortableItemCategory::Weapon;
  definition.nativeWorlds = {owner};
  definition.translations.emplace(target, std::move(targetId));
  definition.maxQuantity = 1;
  definition.equipable = true;
  definition.requiresVisualAssetWhenEquipped = true;
  return definition;
}

Result<void> Add(PortableItemCatalog &catalog,
                 PortableItemDefinition definition) {
  return catalog.Register(std::move(definition));
}

} // namespace

Result<void> PortableItemCatalog::Register(PortableItemDefinition definition) {
  if (!IsCanonicalId(definition.id)) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "ID canônico de item inválido: " + definition.id);
  }
  if (definition.nativeWorlds.empty()) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "item precisa ter ao menos um mundo nativo");
  }
  if (definition.maxQuantity == 0 || definition.maxQuantity > 9999) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "limite de quantidade do item é inválido");
  }
  if (definition.requiresVisualAssetWhenEquipped && !definition.equipable) {
    return Result<void>::err(
        ErrorCode::InvalidArgument,
        "item não equipável não pode exigir asset equipado");
  }
  for (const auto &[world, targetId] : definition.translations) {
    if (definition.nativeWorlds.contains(world) || !IsCanonicalId(targetId) ||
        targetId == definition.id) {
      return Result<void>::err(ErrorCode::InvalidArgument,
                               "tradução de item inválida para " +
                                   definition.id);
    }
  }
  if (mDefinitions.contains(definition.id)) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "item canônico já registrado: " + definition.id);
  }
  mDefinitions.emplace(definition.id, std::move(definition));
  return Result<void>::ok();
}

const PortableItemDefinition *
PortableItemCatalog::Find(const std::string &id) const noexcept {
  const auto it = mDefinitions.find(id);
  return it == mDefinitions.end() ? nullptr : &it->second;
}

Result<PortableItemResolution>
PortableItemCatalog::ResolveForWorld(const std::string &id,
                                     WorldId target) const {
  const PortableItemDefinition *definition = Find(id);
  if (definition == nullptr) {
    return Result<PortableItemResolution>::err(
        ErrorCode::Unsupported, "item canônico desconhecido: " + id);
  }
  if (definition->nativeWorlds.contains(target)) {
    return Result<PortableItemResolution>::ok({id, id, false});
  }
  const auto translation = definition->translations.find(target);
  if (translation == definition->translations.end()) {
    return Result<PortableItemResolution>::err(
        ErrorCode::Unsupported,
        "item " + id + " não possui tradução para " + WorldIdName(target));
  }
  if (Find(translation->second) == nullptr) {
    return Result<PortableItemResolution>::err(
        ErrorCode::InvalidState,
        "destino da tradução não está registrado: " + translation->second);
  }
  return Result<PortableItemResolution>::ok({id, translation->second, true});
}

Result<void> PortableItemCatalog::Validate(const PortableItem &item) const {
  const PortableItemDefinition *definition = Find(item.id);
  if (definition == nullptr) {
    return Result<void>::err(ErrorCode::Unsupported,
                             "item canônico desconhecido: " + item.id);
  }
  if (item.quantity == 0 || item.quantity > definition->maxQuantity) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "quantidade inválida para " + item.id);
  }
  if (item.equipped && !definition->equipable) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "item não pode ser equipado: " + item.id);
  }
  if (item.equipped && definition->requiresVisualAssetWhenEquipped &&
      !item.visualAsset.has_value()) {
    return Result<void>::err(
        ErrorCode::InvalidArgument,
        "item equipado exige referência lógica de asset: " + item.id);
  }
  if (item.visualAsset && (!IsCanonicalId(item.visualAsset->id) ||
                           item.visualAsset->owner != item.origin)) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "referência lógica de asset inválida para " +
                                 item.id);
  }
  return Result<void>::ok();
}

std::size_t PortableItemCatalog::Size() const noexcept {
  return mDefinitions.size();
}

Result<PortableItemCatalog> PortableItemCatalog::CreateDefault() {
  PortableItemCatalog catalog;
  std::vector<PortableItemDefinition> definitions;
  definitions.push_back(
      Shared("shared.bow", PortableItemCategory::Weapon, 99));
  definitions.push_back(
      Shared("shared.bombs", PortableItemCategory::Consumable, 99));
  definitions.push_back(
      Shared("shared.bombchu", PortableItemCategory::Consumable, 99));
  definitions.push_back(
      Shared("shared.hookshot", PortableItemCategory::Tool, 1));
  definitions.push_back(
      Shared("shared.ocarina", PortableItemCategory::Instrument, 1));
  definitions.push_back(
      Sword("oot.sword.kokiri", WorldId::Oot, "mm.sword.kokiri", WorldId::Mm));
  definitions.push_back(
      Sword("oot.sword.master", WorldId::Oot, "mm.sword.razor", WorldId::Mm));
  definitions.push_back(Sword("oot.sword.biggoron", WorldId::Oot,
                              "mm.sword.gilded", WorldId::Mm));
  definitions.push_back(
      Sword("mm.sword.kokiri", WorldId::Mm, "oot.sword.kokiri", WorldId::Oot));
  definitions.push_back(
      Sword("mm.sword.razor", WorldId::Mm, "oot.sword.master", WorldId::Oot));
  definitions.push_back(Sword("mm.sword.gilded", WorldId::Mm,
                              "oot.sword.biggoron", WorldId::Oot));

  PortableItemDefinition dekuMask;
  dekuMask.id = "mm.mask.deku";
  dekuMask.category = PortableItemCategory::Transformation;
  dekuMask.nativeWorlds = {WorldId::Mm};
  dekuMask.equipable = true;
  dekuMask.requiresVisualAssetWhenEquipped = true;
  definitions.push_back(std::move(dekuMask));

  for (PortableItemDefinition &definition : definitions) {
    Result<void> added = Add(catalog, std::move(definition));
    if (!added.isOk()) {
      return Result<PortableItemCatalog>::err(added.code,
                                              std::move(added.message));
    }
  }
  for (const auto &[id, definition] : catalog.mDefinitions) {
    for (const auto &[world, targetId] : definition.translations) {
      const PortableItemDefinition *target = catalog.Find(targetId);
      if (target == nullptr || !target->nativeWorlds.contains(world)) {
        return Result<PortableItemCatalog>::err(
            ErrorCode::InvalidState,
            "tradução padrão aponta para item incompatível: " + id);
      }
    }
  }
  return Result<PortableItemCatalog>::ok(std::move(catalog));
}

} // namespace ShipLua
