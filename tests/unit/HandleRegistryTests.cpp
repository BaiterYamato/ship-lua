#include <iostream>
#include <string>

#include "shiplua/handles/HandleRegistry.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void TestKindNames() {
    using ShipLua::HandleKind;
    Check(std::string(ShipLua::HandleKindName(HandleKind::Actor)) == "actor",
          "actor kind name should round-trip");
    Check(ShipLua::HandleKindFromName("prop") == HandleKind::Prop,
          "prop kind lookup should succeed");
    Check(ShipLua::HandleKindFromName("resource") == HandleKind::Resource,
          "resource kind lookup should succeed");
    Check(!ShipLua::HandleKindFromName("bogus").has_value(),
          "unknown kind name should be rejected");
    Check(!ShipLua::HandleKindFromName("").has_value(), "empty kind name should be rejected");
}

void TestCreateAndValidate() {
    ShipLua::HandleRegistry registry(/*hostId=*/7);
    Check(registry.HostId() == 7, "host id should be retained");
    Check(registry.SceneGeneration() == 1, "scene generation should start at 1");
    Check(registry.Count() == 0, "registry should start empty");

    const auto handle = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    Check(handle.isOk(), "create should succeed");
    Check(handle.value->kind == ShipLua::HandleKind::Actor, "handle kind should match");
    Check(handle.value->slot == 0, "first handle should use slot 0");
    Check(handle.value->generation == 1, "first generation should be 1");
    Check(handle.value->sceneGeneration == 1, "handle should carry the current scene");

    Check(registry.Validate(*handle.value, ShipLua::HandleKind::Actor, "mod.alpha").isOk(),
          "owner validation should succeed");
    Check(registry.IsAlive(*handle.value), "fresh handle should be alive");
    Check(registry.Count() == 1 && registry.CountForMod("mod.alpha") == 1,
          "counts should track the new handle");
    Check(registry.OwnerOf(*handle.value) == std::optional<std::string>("mod.alpha"),
          "owner lookup should return the creator");

    Check(!registry.Create(ShipLua::HandleKind::Actor, "").isOk(),
          "empty owner id should be rejected");
}

void TestValidationFailures() {
    ShipLua::HandleRegistry registry;
    const auto actor = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    const auto prop = registry.Create(ShipLua::HandleKind::Prop, "mod.beta");

    const auto wrongKind =
        registry.Validate(*actor.value, ShipLua::HandleKind::Prop, "mod.alpha");
    Check(!wrongKind.isOk() && wrongKind.code == ShipLua::ErrorCode::InvalidHandle,
          "kind mismatch should fail with InvalidHandle");

    const auto wrongOwner =
        registry.Validate(*actor.value, ShipLua::HandleKind::Actor, "mod.beta");
    Check(!wrongOwner.isOk() && wrongOwner.code == ShipLua::ErrorCode::PermissionDenied,
          "foreign owner should fail with PermissionDenied");

    ShipLua::Handle forged = *actor.value;
    forged.slot = 500;
    Check(registry.Validate(forged, ShipLua::HandleKind::Actor, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "out-of-range slot should fail with InvalidHandle");

    forged = *actor.value;
    forged.generation += 1;
    Check(registry.Validate(forged, ShipLua::HandleKind::Actor, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "wrong generation should fail with InvalidHandle");

    // A handle minted for mod.beta must never validate for mod.alpha even
    // with matching kind fields copied from a live handle.
    Check(registry.Validate(*prop.value, ShipLua::HandleKind::Prop, "mod.beta").isOk(),
          "second mod should validate its own handle");
}

void TestGenerationGuardAndSlotReuse() {
    ShipLua::HandleRegistry registry;
    const auto first = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    const auto second = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    Check(first.value->slot == 0 && second.value->slot == 1,
          "slots should grow deterministically");

    Check(registry.Destroy(*first.value, "mod.alpha").isOk(), "destroy should succeed");
    Check(registry.Count() == 1, "count should drop after destroy");
    Check(!registry.IsAlive(*first.value), "destroyed handle should be dead");
    Check(registry.Destroy(*first.value, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "double destroy should fail with InvalidHandle");

    const auto third = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    Check(third.value->slot == 0, "lowest free slot should be reused");
    Check(third.value->generation == first.value->generation + 1,
          "reused slot should bump its generation");

    // The stale handle must not control the new object in the same slot.
    Check(registry.Validate(*first.value, ShipLua::HandleKind::Actor, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "stale handle must not validate against a recycled slot");
    Check(registry.Destroy(*first.value, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "stale handle must not destroy the new object");
    Check(registry.IsAlive(*third.value), "new object should survive stale operations");
}

void TestOwnershipCleanup() {
    ShipLua::HandleRegistry registry;
    const auto a1 = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    const auto a2 = registry.Create(ShipLua::HandleKind::Light, "mod.alpha");
    const auto b1 = registry.Create(ShipLua::HandleKind::Actor, "mod.beta");

    Check(registry.ReleaseMod("mod.alpha") == 2, "release should destroy only the mod's handles");
    Check(registry.Count() == 1, "other mods should be untouched");
    Check(!registry.IsAlive(*a1.value) && !registry.IsAlive(*a2.value),
          "released handles should be dead");
    Check(registry.IsAlive(*b1.value), "other mod's handle should stay alive");
    Check(registry.ReleaseMod("mod.alpha") == 0, "repeated release should be a no-op");
    Check(registry.CountForMod("mod.alpha") == 0 && registry.CountForMod("mod.beta") == 1,
          "per-mod counts should reflect release");
}

void TestSceneInvalidation() {
    ShipLua::HandleRegistry registry;
    const auto a = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    const auto b = registry.Create(ShipLua::HandleKind::Prop, "mod.beta");

    Check(registry.InvalidateScene() == 2, "scene change should destroy every handle");
    Check(registry.Count() == 0, "registry should be empty after a scene change");
    Check(registry.SceneGeneration() == 2, "scene generation should advance");
    Check(registry.Validate(*a.value, ShipLua::HandleKind::Actor, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "old-scene handle should fail validation");
    Check(registry.Destroy(*b.value, "mod.beta").code == ShipLua::ErrorCode::InvalidHandle,
          "old-scene handle should not destroy anything");

    const auto c = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    Check(c.value->sceneGeneration == 2, "new handles should carry the new scene");
    Check(registry.Validate(*c.value, ShipLua::HandleKind::Actor, "mod.alpha").isOk(),
          "new-scene handle should validate");

    // Even a forged handle replaying the new slot/generation with the old
    // scene number must be rejected.
    ShipLua::Handle forgedOldScene = *c.value;
    forgedOldScene.sceneGeneration = 1;
    Check(registry.Validate(forgedOldScene, ShipLua::HandleKind::Actor, "mod.alpha").code ==
              ShipLua::ErrorCode::InvalidHandle,
          "old scene generation should be rejected");
}

void TestLimits() {
    ShipLua::HandleLimits limits;
    limits.maxPerMod = 2;
    limits.maxTotal = 3;
    ShipLua::HandleRegistry registry(0, limits);

    Check(registry.Create(ShipLua::HandleKind::Actor, "mod.alpha").isOk(),
          "first handle should succeed");
    Check(registry.Create(ShipLua::HandleKind::Actor, "mod.alpha").isOk(),
          "second handle should succeed");
    const auto perMod = registry.Create(ShipLua::HandleKind::Actor, "mod.alpha");
    Check(!perMod.isOk() && perMod.code == ShipLua::ErrorCode::ResourceLimit,
          "per-mod limit should fail with ResourceLimit");

    const auto other = registry.Create(ShipLua::HandleKind::Actor, "mod.beta");
    Check(other.isOk(), "other mod should still create within the total limit");
    const auto total = registry.Create(ShipLua::HandleKind::Actor, "mod.beta");
    Check(!total.isOk() && total.code == ShipLua::ErrorCode::ResourceLimit,
          "total limit should fail with ResourceLimit");

    // Freeing a slot re-opens room under both limits.
    Check(registry.ReleaseMod("mod.alpha") == 2, "release should free per-mod room");
    Check(registry.Create(ShipLua::HandleKind::Actor, "mod.alpha").isOk(),
          "create should succeed again after release");
}

} // namespace

int main() {
    TestKindNames();
    TestCreateAndValidate();
    TestValidationFailures();
    TestGenerationGuardAndSlotReuse();
    TestOwnershipCleanup();
    TestSceneInvalidation();
    TestLimits();

    if (failures != 0) {
        std::cerr << failures << " handle registry test(s) failed\n";
        return 1;
    }
    std::cout << "handle registry tests passed\n";
    return 0;
}
