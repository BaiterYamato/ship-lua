#include <iostream>
#include <string>

#include "shiplua/world/PortableItemCatalog.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

ShipLua::PortableItem EquippedSword() {
  ShipLua::PortableItem item;
  item.id = "oot.sword.master";
  item.origin = ShipLua::WorldId::Oot;
  item.equipped = true;
  item.visualAsset =
      ShipLua::AssetReference{ShipLua::WorldId::Oot, "oot.player.sword.master"};
  return item;
}

void TestDefaultCatalogResolvesSharedAndTranslatedItems() {
  const auto created = ShipLua::PortableItemCatalog::CreateDefault();
  Check(created.isOk() && created.value && created.value->Size() == 12,
        "catálogo padrão deve registrar o recorte portátil inicial");
  if (!created.value) {
    return;
  }
  const auto shared =
      created.value->ResolveForWorld("shared.bombs", ShipLua::WorldId::Mm);
  Check(shared.isOk() && shared.value && !shared.value->translated &&
            shared.value->targetId == "shared.bombs",
        "item comum deve manter o ID canônico nos dois mundos");
  const auto sword =
      created.value->ResolveForWorld("oot.sword.master", ShipLua::WorldId::Mm);
  Check(sword.isOk() && sword.value && sword.value->translated &&
            sword.value->targetId == "mm.sword.razor",
        "espada deve usar tradução explícita, nunca cast de enum");
  const auto deferred =
      created.value->ResolveForWorld("mm.mask.deku", ShipLua::WorldId::Oot);
  Check(!deferred.isOk() && deferred.code == ShipLua::ErrorCode::Unsupported,
        "item exclusivo sem tradução deve permanecer adiado");
}

void TestPortableItemValidation() {
  const auto created = ShipLua::PortableItemCatalog::CreateDefault();
  if (!created.value) {
    Check(false, "catálogo padrão deve existir para validar itens");
    return;
  }
  ShipLua::PortableItem sword = EquippedSword();
  Check(created.value->Validate(sword).isOk(),
        "espada equipada com asset lógico de origem deve ser válida");
  sword.visualAsset.reset();
  Check(
      !created.value->Validate(sword).isOk(),
      "equipamento visual não pode atravessar mundos sem referência de asset");

  ShipLua::PortableItem bombs;
  bombs.id = "shared.bombs";
  bombs.origin = ShipLua::WorldId::Mm;
  bombs.quantity = 1000;
  Check(!created.value->Validate(bombs).isOk(),
        "quantidade portátil acima do contrato deve ser rejeitada");
  bombs.quantity = 40;
  bombs.equipped = true;
  Check(!created.value->Validate(bombs).isOk(),
        "consumível não equipável deve ser rejeitado como equipamento");
}

void TestRegistrationRejectsAmbiguousDefinitions() {
  ShipLua::PortableItemCatalog catalog;
  ShipLua::PortableItemDefinition modItem;
  modItem.id = "community.example.tool";
  modItem.nativeWorlds = {ShipLua::WorldId::Oot, ShipLua::WorldId::Mm};
  Check(catalog.Register(modItem).isOk(),
        "mods podem registrar IDs canônicos namespaced sem alterar o núcleo");
  Check(!catalog.Register(modItem).isOk(),
        "IDs duplicados devem ser rejeitados");

  ShipLua::PortableItemDefinition invalid;
  invalid.id = "Invalid Item";
  invalid.nativeWorlds = {ShipLua::WorldId::Oot};
  Check(!catalog.Register(invalid).isOk(),
        "IDs dependentes de apresentação devem ser rejeitados");

  ShipLua::PortableItemDefinition ambiguous;
  ambiguous.id = "oot.tool.test";
  ambiguous.nativeWorlds = {ShipLua::WorldId::Oot};
  ambiguous.translations[ShipLua::WorldId::Oot] = "oot.tool.other";
  Check(!catalog.Register(ambiguous).isOk(),
        "um mundo nativo não pode também possuir tradução substituta");
}

} // namespace

int main() {
  TestDefaultCatalogResolvesSharedAndTranslatedItems();
  TestPortableItemValidation();
  TestRegistrationRejectsAmbiguousDefinitions();
  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "All portable item catalog checks passed\n";
  return 0;
}
