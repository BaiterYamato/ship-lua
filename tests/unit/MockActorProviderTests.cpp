#include <iostream>
#include <string>

#include "shiplua/mock/MockActorProvider.h"

namespace {

int failures = 0;

void Check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

ShipLua::ActorSpawnRequest Dog(double x = 0.0) {
  ShipLua::ActorSpawnRequest request;
  request.actor = "oot.en_dog";
  request.x = x;
  request.y = 2.0;
  request.z = 3.0;
  request.rotationY = 90.0;
  return request;
}

void TestAllowlistOwnershipAndLifecycle() {
  ShipLua::MockActorProvider provider("oot", {2, 4});
  auto first = provider.Spawn("mod.a", Dog(1.0));
  auto second = provider.Spawn("mod.a", Dog(2.0));
  Check(first.isOk() && second.isOk(), "allowlisted actors should spawn");
  Check(provider.CountForMod("mod.a") == 2,
        "provider should count actor ownership");

  auto limited = provider.Spawn("mod.a", Dog(3.0));
  Check(!limited.isOk() && limited.code == ShipLua::ErrorCode::ResourceLimit,
        "provider handle limit should be enforced");
  auto unsupported =
      provider.Spawn("mod.b", ShipLua::ActorSpawnRequest{"oot.actor_id_7"});
  Check(!unsupported.isOk() &&
            unsupported.code == ShipLua::ErrorCode::Unsupported,
        "native-looking actor id should not bypass the allowlist");

  if (first.isOk()) {
    auto foreign = provider.Destroy("mod.b", *first.value);
    Check(!foreign.isOk() &&
              foreign.code == ShipLua::ErrorCode::PermissionDenied,
          "a mod must not destroy another mod's actor");
    Check(provider.Destroy("mod.a", *first.value).isOk(),
          "owner should destroy its actor");
    auto stale = provider.Exists("mod.a", *first.value);
    Check(!stale.isOk() && stale.code == ShipLua::ErrorCode::InvalidHandle,
          "destroyed handle should be stale");
  }

  auto other = provider.Spawn("mod.b", Dog(4.0));
  Check(other.isOk(), "freed slot should be reusable");
  Check(provider.ReleaseMod("mod.a").isOk(), "mod cleanup should succeed");
  Check(provider.CountForMod("mod.a") == 0 &&
            provider.CountForMod("mod.b") == 1,
        "mod cleanup must preserve actors owned by other mods");

  const std::size_t invalidated = provider.InvalidateScene();
  Check(invalidated == 1 && provider.Count() == 0,
        "scene change should invalidate every remaining actor");
  if (other.isOk()) {
    auto previousScene = provider.Exists("mod.b", *other.value);
    Check(!previousScene.isOk() &&
              previousScene.code == ShipLua::ErrorCode::InvalidHandle,
          "previous-scene handle should never resolve");
  }
}

void TestGameSpecificAllowlist() {
  ShipLua::MockActorProvider mm("mm");
  ShipLua::ActorSpawnRequest mmDog;
  mmDog.actor = "mm.en_dg";
  Check(mm.Spawn("mod.mm", mmDog).isOk(),
        "MM logical dog should be allowlisted in MM");
  Check(!mm.Spawn("mod.mm", Dog()).isOk(),
        "OoT logical actor should be rejected in MM");
}

} // namespace

int main() {
  TestAllowlistOwnershipAndLifecycle();
  TestGameSpecificAllowlist();
  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "All mock actor provider checks passed\n";
  return 0;
}
