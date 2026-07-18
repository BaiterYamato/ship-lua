#include <iostream>
#include <string>
#include <vector>

#include "shiplua/capability/CapabilityRegistry.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::SemVersion Version(std::uint64_t major, std::uint64_t minor, std::uint64_t patch) {
    ShipLua::SemVersion version;
    version.major = major;
    version.minor = minor;
    version.patch = patch;
    return version;
}

ShipLua::CapabilityProvider MakeProvider(std::string name, ShipLua::SemVersion capabilityVersion,
                                         std::vector<std::string> games) {
    ShipLua::CapabilityProvider provider;
    provider.name = std::move(name);
    provider.providerVersion = Version(0, 4, 0);
    provider.capabilityVersion = capabilityVersion;
    provider.games = std::move(games);
    provider.stability = ShipLua::CapabilityStability::Preview;
    return provider;
}

ShipLua::VersionRange Range(const std::string& text) {
    auto range = ShipLua::VersionRange::Parse(text);
    Check(range.isOk(), "range should parse: " + text);
    return range.isOk() ? *range.value : ShipLua::VersionRange{};
}

void TestRegisterAndHas() {
    ShipLua::CapabilityRegistry registry;
    Check(!registry.Has("actor.spawn"), "unknown capability must be absent");
    Check(registry.Count() == 0, "empty registry should have zero capabilities");

    Check(registry.Register("actor.spawn", MakeProvider("shipwright-native", Version(1, 1, 0),
                                                        {"oot"}))
              .isOk(),
          "valid offer should register");
    Check(registry.Has("actor.spawn"), "registered capability must be present without game filter");
    Check(registry.Has("actor.spawn", "oot"), "offer must be visible for its game");
    Check(!registry.Has("actor.spawn", "mm"), "offer must be hidden for other games");
    Check(!registry.Has("actor.destroy", "oot"), "unknown capability must stay absent");
    Check(registry.Count() == 1, "registry should count one capability");
}

void TestRegisterValidation() {
    ShipLua::CapabilityRegistry registry;
    auto valid = MakeProvider("mock-runtime", Version(1, 0, 0), {"oot", "mm"});

    auto badId = valid;
    Check(!registry.Register("Actor.spawn", badId).isOk(), "uppercase id must be rejected");
    Check(!registry.Register("actor", badId).isOk(), "single-segment id must be rejected");
    Check(!registry.Register("actor..spawn", badId).isOk(), "empty segment must be rejected");
    Check(!registry.Register("actor.spawn ", badId).isOk(), "trailing space must be rejected");

    auto badProvider = valid;
    badProvider.name = "Mock.Runtime";
    Check(!registry.Register("actor.spawn", badProvider).isOk(),
          "invalid provider name must be rejected");
    badProvider.name = "";
    Check(!registry.Register("actor.spawn", badProvider).isOk(),
          "empty provider name must be rejected");

    auto digitProvider = valid;
    digitProvider.name = "2ship-native";
    Check(registry.Register("actor.destroy", digitProvider).isOk(),
          "provider name with leading digit should register (2ship-native)");
    Check(registry.Unregister("actor.destroy", "2ship-native").isOk(),
          "digit provider cleanup should succeed");

    auto noGames = valid;
    noGames.games.clear();
    Check(!registry.Register("actor.spawn", noGames).isOk(), "empty games must be rejected");

    auto badGame = valid;
    badGame.games = {"switch"};
    Check(!registry.Register("actor.spawn", badGame).isOk(), "unknown game must be rejected");

    auto dupGames = valid;
    dupGames.games = {"oot", "oot"};
    Check(!registry.Register("actor.spawn", dupGames).isOk(), "duplicated games must be rejected");

    auto badPermission = valid;
    badPermission.permissions = {"world create"};
    Check(!registry.Register("actor.spawn", badPermission).isOk(),
          "malformed permission must be rejected");

    Check(registry.Register("core.events", valid).isOk(), "multi-game offer should register");
    Check(registry.Count() == 1, "only valid offers should land in the registry");
}

void TestDuplicateAndUnregister() {
    ShipLua::CapabilityRegistry registry;
    Check(registry.Register("actor.spawn", MakeProvider("shipwright-native", Version(1, 0, 0),
                                                        {"oot"}))
              .isOk(),
          "first offer should register");
    const auto duplicate = registry.Register(
        "actor.spawn", MakeProvider("shipwright-native", Version(1, 2, 0), {"mm"}));
    Check(!duplicate.isOk() && duplicate.code == ShipLua::ErrorCode::InvalidState,
          "duplicate (id, provider) must be rejected with invalid_state");

    const auto missing = registry.Unregister("actor.spawn", "2ship-native");
    Check(!missing.isOk() && missing.code == ShipLua::ErrorCode::InvalidHandle,
          "unregister of unknown provider must fail with invalid_handle");

    Check(registry.Register("actor.spawn", MakeProvider("mock-runtime", Version(1, 3, 0), {"mm"}))
              .isOk(),
          "second provider should register");
    Check(registry.Unregister("actor.spawn", "shipwright-native").isOk(),
          "unregister of existing offer should succeed");
    Check(registry.Has("actor.spawn"), "capability must survive while one provider remains");
    Check(registry.Unregister("actor.spawn", "mock-runtime").isOk(),
          "last provider should unregister");
    Check(!registry.Has("actor.spawn"), "capability must disappear with no providers");
    Check(registry.Count() == 0, "empty capability entries must be pruned");
}

void TestVersionRanges() {
    ShipLua::CapabilityRegistry registry;
    Check(registry.Register("actor.spawn", MakeProvider("shipwright-native", Version(1, 1, 0),
                                                        {"oot"}))
              .isOk(),
          "offer should register");

    Check(registry.Has("actor.spawn", Range(">=1.0")), "lower bound range should match");
    Check(registry.Has("actor.spawn", Range(">=1.1 <2.0")), "combined range should match");
    Check(registry.Has("actor.spawn", Range("=1.1.0")), "exact range should match");
    Check(!registry.Has("actor.spawn", Range(">=2.0")), "higher range should not match");
    Check(!registry.Has("actor.spawn", Range("<1.0")), "lower exclusive range should not match");
    Check(!registry.Has("actor.spawn", Range(">=1.0"), "mm"),
          "range must respect the game filter");
    Check(!registry.Has("actor.missing", Range(">=1.0")), "missing capability must not match");
}

void TestMultiProviderSelection() {
    ShipLua::CapabilityRegistry registry;
    Check(registry.Register("actor.spawn", MakeProvider("alpha-native", Version(1, 0, 0), {"oot"}))
              .isOk(),
          "alpha offer should register");
    Check(registry.Register("actor.spawn", MakeProvider("beta-native", Version(1, 2, 0), {"oot"}))
              .isOk(),
          "beta offer should register");

    auto info = registry.Info("actor.spawn", "oot");
    Check(info.has_value(), "info should resolve a provider");
    if (info.has_value()) {
        Check(info->provider == "beta-native", "highest capability version should win");
        Check(info->version.major == 1 && info->version.minor == 2,
              "descriptor should carry the winning version");
    }

    // Empate de versão da capability: vence a maior versão do provider.
    auto olderProvider = MakeProvider("gamma-native", Version(1, 2, 0), {"oot"});
    olderProvider.providerVersion = Version(0, 5, 1);
    Check(registry.Register("actor.spawn", olderProvider).isOk(), "gamma offer should register");
    info = registry.Info("actor.spawn", "oot");
    Check(info.has_value() && info->provider == "gamma-native",
          "provider version should break capability version ties");

    // Empate total: vence o nome lexicograficamente menor (determinístico).
    auto tiedA = MakeProvider("delta-native", Version(9, 9, 9), {"mm"});
    tiedA.providerVersion = Version(1, 0, 0);
    auto tiedB = MakeProvider("cetra-native", Version(9, 9, 9), {"mm"});
    tiedB.providerVersion = Version(1, 0, 0);
    Check(registry.Register("scene.patch", tiedA).isOk(), "delta offer should register");
    Check(registry.Register("scene.patch", tiedB).isOk(), "cetra offer should register");
    info = registry.Info("scene.patch", "mm");
    Check(info.has_value() && info->provider == "cetra-native",
          "full tie should resolve lexicographically");

    Check(!registry.Info("actor.spawn", "mm").has_value(),
          "selection must respect the game filter");
    Check(!registry.Info("actor.missing").has_value(), "unknown capability has no descriptor");
}

void TestInfoDescriptor() {
    ShipLua::CapabilityRegistry registry;
    auto offer = MakeProvider("shipwright-native", Version(1, 1, 0), {"oot"});
    offer.providerVersion = Version(0, 4, 0);
    offer.stability = ShipLua::CapabilityStability::Preview;
    offer.permissions = {"world.entities.create"};
    offer.limits.perMod = 16;
    offer.description = "Spawn de atores genéricos";
    Check(registry.Register("actor.spawn", offer).isOk(), "full offer should register");

    const auto info = registry.Info("actor.spawn", "oot");
    Check(info.has_value(), "info should resolve");
    if (info.has_value()) {
        Check(info->id == "actor.spawn", "descriptor id should match");
        Check(info->version == Version(1, 1, 0), "descriptor version should match");
        Check(info->provider == "shipwright-native", "descriptor provider should match");
        Check(info->providerVersion == Version(0, 4, 0), "descriptor provider version should match");
        Check(info->games.size() == 1 && info->games[0] == "oot", "descriptor games should match");
        Check(info->stability == ShipLua::CapabilityStability::Preview,
              "descriptor stability should match");
        Check(info->permissions.size() == 1 &&
                  info->permissions[0] == "world.entities.create",
              "descriptor permissions should match");
        Check(info->limits.perMod.has_value() && *info->limits.perMod == 16,
              "descriptor limits should match");
        Check(info->description == "Spawn de atores genéricos",
              "descriptor description should match");
    }
}

void TestProvidersAndList() {
    ShipLua::CapabilityRegistry registry;
    Check(registry.Register("actor.spawn", MakeProvider("zeta-native", Version(1, 0, 0), {"oot"}))
              .isOk(),
          "zeta offer should register");
    Check(registry.Register("actor.spawn", MakeProvider("alpha-native", Version(1, 1, 0),
                                                        {"oot", "mm"}))
              .isOk(),
          "alpha offer should register");
    auto stableOffer = MakeProvider("shipwright-native", Version(2, 0, 0), {"oot"});
    stableOffer.stability = ShipLua::CapabilityStability::Stable;
    Check(registry.Register("scene.query", stableOffer).isOk(), "stable offer should register");
    auto deprecatedOffer = MakeProvider("2ship-native", Version(1, 0, 0), {"mm"});
    deprecatedOffer.stability = ShipLua::CapabilityStability::Deprecated;
    Check(registry.Register("mm.clock", deprecatedOffer).isOk(), "deprecated offer should register");

    const auto ootProviders = registry.Providers("actor.spawn", "oot");
    Check(ootProviders.size() == 2 && ootProviders[0] == "alpha-native" &&
              ootProviders[1] == "zeta-native",
          "providers should be game-filtered and sorted");
    const auto mmProviders = registry.Providers("actor.spawn", "mm");
    Check(mmProviders.size() == 1 && mmProviders[0] == "alpha-native",
          "mm providers should exclude oot-only offers");
    Check(registry.Providers("actor.missing").empty(), "unknown capability has no providers");

    const auto ootList = registry.List("oot");
    Check(ootList.size() == 2 && ootList[0] == "actor.spawn" && ootList[1] == "scene.query",
          "list should be sorted and game-filtered");
    const auto mmList = registry.List("mm");
    Check(mmList.size() == 2 && mmList[0] == "actor.spawn" && mmList[1] == "mm.clock",
          "mm list should include mm-only capabilities");
    Check(registry.Has("mm.clock", "mm"), "deprecated capability must remain detectable");

    const auto preview = registry.List("", ShipLua::CapabilityStability::Preview);
    Check(preview.size() == 1 && preview[0] == "actor.spawn", "stability filter should apply");
    const auto deprecated = registry.List("", ShipLua::CapabilityStability::Deprecated);
    Check(deprecated.size() == 1 && deprecated[0] == "mm.clock",
          "deprecated filter should list deprecated capabilities");
}

void TestStabilityParsing() {
    for (const auto& [text, expected] : std::vector<std::pair<std::string, ShipLua::CapabilityStability>>{
             {"internal", ShipLua::CapabilityStability::Internal},
             {"experimental", ShipLua::CapabilityStability::Experimental},
             {"preview", ShipLua::CapabilityStability::Preview},
             {"stable", ShipLua::CapabilityStability::Stable},
             {"deprecated", ShipLua::CapabilityStability::Deprecated}}) {
        const auto parsed = ShipLua::ParseCapabilityStability(text);
        Check(parsed.isOk() && *parsed.value == expected, "stability should parse: " + text);
        Check(ShipLua::CapabilityStabilityName(expected) == text,
              "stability name should round-trip: " + text);
    }
    Check(!ShipLua::ParseCapabilityStability("beta").isOk(), "unknown stability must be rejected");
    Check(!ShipLua::ParseCapabilityStability("").isOk(), "empty stability must be rejected");
}

} // namespace

int main() {
    TestRegisterAndHas();
    TestRegisterValidation();
    TestDuplicateAndUnregister();
    TestVersionRanges();
    TestMultiProviderSelection();
    TestInfoDescriptor();
    TestProvidersAndList();
    TestStabilityParsing();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all capability registry tests passed\n";
    return 0;
}
