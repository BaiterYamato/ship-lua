#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "shiplua/lifecycle/ModLifecycle.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void TestPhaseNames() {
    using ShipLua::ModLifecyclePhase;
    Check(std::string(ShipLua::ModLifecyclePhaseName(ModLifecyclePhase::Init)) == "init",
          "init phase name should round-trip");
    Check(std::string(ShipLua::ModLifecyclePhaseName(ModLifecyclePhase::Unloaded)) == "unloaded",
          "unloaded phase name should round-trip");
    Check(std::string(ShipLua::ModLifecyclePhaseName(ModLifecyclePhase::Failed)) == "failed",
          "failed phase name should round-trip");
}

void TestHappyPathLoadUnload() {
    ShipLua::ModLifecycleManager manager;
    Check(manager.RegisterMod("mod.alpha").isOk(), "register should succeed");
    Check(manager.Count() == 1, "manager should track the mod");

    auto* lifecycle = manager.Find("mod.alpha");
    Check(lifecycle != nullptr, "registered mod should be found");
    Check(lifecycle->Phase() == ShipLua::ModLifecyclePhase::Init,
          "new mod should start in init");
    Check(!lifecycle->CanMintHandles(), "init must not mint handles");
    Check(lifecycle->MintHandle(ShipLua::HandleKind::Actor).code ==
              ShipLua::ErrorCode::InvalidState,
          "minting in init should fail with InvalidState");

    Check(lifecycle->BeginLoad().isOk(), "init -> loading should succeed");
    Check(lifecycle->Phase() == ShipLua::ModLifecyclePhase::Loading,
          "phase should be loading");
    Check(lifecycle->CanMintHandles(), "loading may mint handles");

    const auto handle = lifecycle->MintHandle(ShipLua::HandleKind::Actor);
    Check(handle.isOk(), "mint in loading should succeed");
    Check(manager.Handles().OwnerOf(*handle.value) ==
              std::optional<std::string>("mod.alpha"),
          "minted handle should belong to the mod");

    Check(lifecycle->CompleteLoad().isOk(), "loading -> active should succeed");
    Check(lifecycle->Phase() == ShipLua::ModLifecyclePhase::Active,
          "phase should be active");
    Check(lifecycle->CanMintHandles(), "active may mint handles");

    Check(manager.UnloadMod("mod.alpha").isOk(), "unload should succeed");
    Check(lifecycle->Phase() == ShipLua::ModLifecyclePhase::Unloaded,
          "phase should be unloaded");
    Check(manager.Handles().Count() == 0, "unload must release every handle");
    Check(!manager.Handles().IsAlive(*handle.value),
          "minted handle must be dead after unload");
    Check(!lifecycle->CanMintHandles(), "unloaded must not mint handles");
    Check(lifecycle->MintHandle(ShipLua::HandleKind::Actor).code ==
              ShipLua::ErrorCode::InvalidState,
          "minting after unload should fail with InvalidState");

    Check(manager.UnloadMod("mod.alpha").isOk(), "unload should be idempotent");
    Check(manager.UnregisterMod("mod.alpha").isOk(), "unregister should succeed");
    Check(manager.Find("mod.alpha") == nullptr, "unregistered mod should be gone");
    Check(manager.Count() == 0, "manager should be empty");
}

void TestIllegalTransitions() {
    ShipLua::ModLifecycleManager manager;
    manager.RegisterMod("mod.alpha");
    auto* lifecycle = manager.Find("mod.alpha");

    Check(lifecycle->CompleteLoad().code == ShipLua::ErrorCode::InvalidState,
          "init -> active should be illegal");
    Check(lifecycle->Fail("boom").code == ShipLua::ErrorCode::InvalidState,
          "init -> failed should be illegal");
    Check(lifecycle->BeginLoad().isOk(), "init -> loading should succeed");
    Check(lifecycle->BeginLoad().code == ShipLua::ErrorCode::InvalidState,
          "loading -> loading should be illegal");
    Check(lifecycle->CompleteLoad().isOk(), "loading -> active should succeed");
    Check(lifecycle->CompleteUnload().code == ShipLua::ErrorCode::InvalidState,
          "active -> unloaded without unloading should be illegal");

    Check(manager.UnregisterMod("missing").code == ShipLua::ErrorCode::InvalidHandle,
          "unregistering a missing mod should fail with InvalidHandle");
    Check(manager.UnloadMod("missing").code == ShipLua::ErrorCode::InvalidHandle,
          "unloading a missing mod should fail with InvalidHandle");
    Check(manager.RegisterMod("mod.alpha").code == ShipLua::ErrorCode::InvalidState,
          "duplicate registration should fail with InvalidState");
    Check(manager.RegisterMod("").code == ShipLua::ErrorCode::InvalidArgument,
          "empty mod id should fail with InvalidArgument");
}

void TestFailurePathStillCleansUp() {
    ShipLua::ModLifecycleManager manager;
    manager.RegisterMod("mod.alpha");
    auto* lifecycle = manager.Find("mod.alpha");
    lifecycle->BeginLoad();

    const auto handle = lifecycle->MintHandle(ShipLua::HandleKind::Prop);
    Check(handle.isOk(), "mint in loading should succeed");

    Check(lifecycle->Fail("entrypoint error").isOk(), "loading -> failed should succeed");
    Check(lifecycle->Phase() == ShipLua::ModLifecyclePhase::Failed,
          "phase should be failed");
    Check(lifecycle->LastError() == "entrypoint error", "failure reason should be recorded");
    Check(!lifecycle->CanMintHandles(), "failed must not mint handles");
    Check(manager.Handles().IsAlive(*handle.value),
          "failed mod's handles stay tracked until unload");

    // A failed mod must still get deterministic cleanup on unload.
    Check(manager.UnloadMod("mod.alpha").isOk(), "unloading a failed mod should succeed");
    Check(lifecycle->Phase() == ShipLua::ModLifecyclePhase::Unloaded,
          "failed mod should reach unloaded");
    Check(manager.Handles().Count() == 0, "failed mod's handles must be released");
}

void TestUnloadFromLoadingAndInit() {
    ShipLua::ModLifecycleManager manager;
    manager.RegisterMod("mod.abort-load");
    manager.Find("mod.abort-load")->BeginLoad();
    Check(manager.UnloadMod("mod.abort-load").isOk(),
          "unload from loading (aborted load) should succeed");
    Check(manager.Find("mod.abort-load")->Phase() == ShipLua::ModLifecyclePhase::Unloaded,
          "aborted load should reach unloaded");

    manager.RegisterMod("mod.never-loaded");
    Check(manager.UnloadMod("mod.never-loaded").isOk(),
          "unload from init should succeed");
    Check(manager.Find("mod.never-loaded")->Phase() == ShipLua::ModLifecyclePhase::Unloaded,
          "never-loaded mod should reach unloaded");
}

void TestOwnershipAcrossMods() {
    ShipLua::ModLifecycleManager manager;
    manager.RegisterMod("mod.alpha");
    manager.RegisterMod("mod.beta");
    manager.Find("mod.alpha")->BeginLoad();
    manager.Find("mod.alpha")->CompleteLoad();
    manager.Find("mod.beta")->BeginLoad();
    manager.Find("mod.beta")->CompleteLoad();

    const auto alpha = manager.Find("mod.alpha")->MintHandle(ShipLua::HandleKind::Actor);
    const auto beta = manager.Find("mod.beta")->MintHandle(ShipLua::HandleKind::Actor);
    Check(alpha.isOk() && beta.isOk(), "both mods should mint");

    // Each mod controls only its own handles (plan-sdk.md §6 ownership rule).
    Check(manager.Handles().Validate(*alpha.value, ShipLua::HandleKind::Actor, "mod.beta")
              .code == ShipLua::ErrorCode::PermissionDenied,
          "mod.beta must not control mod.alpha's handle");
    Check(manager.Handles().Destroy(*alpha.value, "mod.beta").code ==
              ShipLua::ErrorCode::PermissionDenied,
          "mod.beta must not destroy mod.alpha's handle");

    // Unloading one mod never touches the other's objects.
    Check(manager.UnloadMod("mod.alpha").isOk(), "unloading alpha should succeed");
    Check(!manager.Handles().IsAlive(*alpha.value), "alpha's handle should be dead");
    Check(manager.Handles().IsAlive(*beta.value), "beta's handle should survive");
}

void TestSceneChangeThroughManager() {
    ShipLua::ModLifecycleManager manager;
    manager.RegisterMod("mod.alpha");
    manager.RegisterMod("mod.beta");
    manager.Find("mod.alpha")->BeginLoad();
    manager.Find("mod.alpha")->CompleteLoad();
    manager.Find("mod.beta")->BeginLoad();
    manager.Find("mod.beta")->CompleteLoad();

    const auto alpha = manager.Find("mod.alpha")->MintHandle(ShipLua::HandleKind::Actor);
    const auto beta = manager.Find("mod.beta")->MintHandle(ShipLua::HandleKind::Prop);

    Check(manager.OnSceneChange() == 2, "scene change should destroy both mods' handles");
    Check(manager.Handles().Count() == 0, "registry should be empty after scene change");
    Check(!manager.Handles().IsAlive(*alpha.value) &&
              !manager.Handles().IsAlive(*beta.value),
          "all handles should be dead after scene change");

    // Mods stay active and can mint fresh handles in the new scene.
    const auto fresh = manager.Find("mod.alpha")->MintHandle(ShipLua::HandleKind::Actor);
    Check(fresh.isOk() && fresh.value->sceneGeneration == 2,
          "fresh handles should carry the new scene generation");
}

void TestHotReloadCycle() {
    ShipLua::ModLifecycleManager manager;
    manager.RegisterMod("mod.alpha");
    manager.Find("mod.alpha")->BeginLoad();
    manager.Find("mod.alpha")->CompleteLoad();
    const auto oldHandle = manager.Find("mod.alpha")->MintHandle(ShipLua::HandleKind::Actor);

    // Hot reload = unregister (full teardown) + register (fresh load cycle).
    Check(manager.UnregisterMod("mod.alpha").isOk(), "teardown should succeed");
    Check(manager.RegisterMod("mod.alpha").isOk(), "re-register should succeed");
    Check(manager.Find("mod.alpha")->Phase() == ShipLua::ModLifecyclePhase::Init,
          "reloaded mod should start a fresh load cycle");

    manager.Find("mod.alpha")->BeginLoad();
    const auto newHandle = manager.Find("mod.alpha")->MintHandle(ShipLua::HandleKind::Actor);
    Check(newHandle.isOk(), "reloaded mod should mint again");
    Check(!(*newHandle.value == *oldHandle.value),
          "fresh handles must differ from the previous cycle's dead handles");
    Check(manager.Handles().Validate(*oldHandle.value, ShipLua::HandleKind::Actor,
                                     "mod.alpha")
              .code == ShipLua::ErrorCode::InvalidHandle,
          "previous cycle's handles must stay dead");
}

void TestTransitionLogging() {
    std::vector<std::pair<ShipLua::LogLevel, std::string>> lines;
    ShipLua::Logger logger([&](ShipLua::LogLevel level, const std::string& modId,
                               const std::string& message) {
        if (modId == "mod.alpha") {
            lines.emplace_back(level, message);
        }
    });
    ShipLua::ModLifecycleManager manager(0, {}, logger);

    manager.RegisterMod("mod.alpha");
    auto* lifecycle = manager.Find("mod.alpha");
    lifecycle->BeginLoad();
    lifecycle->CompleteLoad();
    lifecycle->MintHandle(ShipLua::HandleKind::Actor);
    manager.UnloadMod("mod.alpha");

    bool sawInitToLoading = false;
    bool sawLoadingToActive = false;
    bool sawUnloadToUnloaded = false;
    bool sawRelease = false;
    for (const auto& line : lines) {
        sawInitToLoading = sawInitToLoading || line.second.find("init -> loading") != std::string::npos;
        sawLoadingToActive =
            sawLoadingToActive || line.second.find("loading -> active") != std::string::npos;
        sawUnloadToUnloaded = sawUnloadToUnloaded ||
                              line.second.find("unloading -> unloaded") != std::string::npos;
        sawRelease = sawRelease || line.second.find("released") != std::string::npos;
    }
    Check(sawInitToLoading && sawLoadingToActive && sawUnloadToUnloaded,
          "lifecycle transitions should be logged");
    Check(sawRelease, "unload cleanup should be logged");
}

} // namespace

int main() {
    TestPhaseNames();
    TestHappyPathLoadUnload();
    TestIllegalTransitions();
    TestFailurePathStillCleansUp();
    TestUnloadFromLoadingAndInit();
    TestOwnershipAcrossMods();
    TestSceneChangeThroughManager();
    TestHotReloadCycle();
    TestTransitionLogging();

    if (failures != 0) {
        std::cerr << failures << " mod lifecycle test(s) failed\n";
        return 1;
    }
    std::cout << "mod lifecycle tests passed\n";
    return 0;
}
