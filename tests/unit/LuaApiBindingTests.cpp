#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "shiplua/host/ModHost.h"
#include "shiplua/input/HotkeyRegistry.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::Manifest MakeManifest(std::string id, int priority = 50) {
    ShipLua::Manifest manifest;
    manifest.id = std::move(id);
    manifest.name = "Binding test";
    manifest.version = "0.1.0";
    manifest.apiRange = ">=0.1 <0.4";
    manifest.entrypoint = "main.lua";
    manifest.loadPriority = priority;
    return manifest;
}

struct CapturedLog {
    ShipLua::LogLevel level;
    std::string modId;
    std::string message;
};

ShipLua::Logger CaptureLogger(std::vector<CapturedLog>& logs) {
    return ShipLua::Logger([&logs](ShipLua::LogLevel level, const std::string& modId,
                                  const std::string& message) {
        logs.push_back({level, modId, message});
    });
}

bool ContainsLog(const std::vector<CapturedLog>& logs, const std::string& modId,
                 const std::string& message) {
    return std::any_of(logs.begin(), logs.end(), [&](const CapturedLog& log) {
        return log.modId == modId && log.message == message;
    });
}

void TestHelloWorldOnBothHosts() {
    const std::string source = R"lua(
assert(package == nil)
local ship = require("ship")
assert(require("ship") == ship)
local forbidden = pcall(require, "filesystem")
assert(not forbidden)
assert(ship.api.version() == "0.3.0")
assert(ship.runtime.version() == "0.1.0")

ship.events.on("game.ready", function(event)
    ship.log.info("Hello from " .. ship.game.id() .. " host=" .. ship.game.host_version())
    assert(event.game_id == ship.game.id())
    assert(event.host_version == ship.game.host_version())
    ready_count = (ready_count or 0) + 1
end)
)lua";

    for (const std::string game : {"oot", "mm"}) {
        std::vector<CapturedLog> logs;
        const std::string hostVersion = game == "oot" ? "9.1.0" : "4.2.0";
        ShipLua::LuaApiHostContext context{game, hostVersion, "0.1.0", {"scene.events"}};
        ShipLua::ModHost host(context, CaptureLogger(logs));
        const std::string modId = "example.hello_" + game;
        const auto loaded = host.LoadModFromManifestAndSource(MakeManifest(modId), source);
        Check(loaded.isOk(), "same hello-world source should load on " + game);
        Check(host.SubscriptionCount() == 1, "hello-world should register one callback on " + game);

        ShipLua::EventPayload payload{
            {"game_id", game},
            {"host_version", hostVersion},
            {"runtime_version", "0.1.0"},
            {"api_version", "0.3.0"},
        };
        const auto dispatched = host.DispatchEvent("game.ready", payload);
        Check(dispatched.isOk() && dispatched.value->callbacksInvoked == 1 &&
                  dispatched.value->failures.empty(),
              "game.ready should invoke hello-world safely on " + game);
        Check(ContainsLog(logs, modId, "Hello from " + game + " host=" + hostVersion),
              "hello-world should log host identity on " + game);
        if (auto* runtime = host.GetRuntime(modId); runtime != nullptr) {
            Check(runtime->DoString("assert(ready_count == 1)").isOk(),
                  "callback should update only its own Lua state on " + game);
        }
        Check(host.UnloadMod(modId).isOk(), "hello-world should unload on " + game);
        Check(host.SubscriptionCount() == 0,
              "unload should remove hello-world callback on " + game);
    }
}

void TestCapabilitiesAndExplicitOff() {
    ShipLua::LuaApiHostContext context{
        "oot", "9.1.0", "0.1.0", {"scene.events", "actor.events", "scene.events"}};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(
        MakeManifest("test.capabilities"), R"lua(
local ship = require("ship")
local capabilities = ship.capabilities.list()
assert(#capabilities == 2)
assert(capabilities[1] == "actor.events")
assert(capabilities[2] == "scene.events")
assert(ship.capabilities.has("scene.events"))
assert(not ship.capabilities.has("mm.cycle"))
local subscription = ship.events.on("game.frame", { priority = -10 }, function()
    unexpected_call = true
end)
assert(ship.events.off(subscription))
local ok = pcall(ship.events.off, subscription)
assert(not ok)
)lua");
    Check(loaded.isOk(), "capabilities and explicit off script should load");
    Check(host.SubscriptionCount() == 0, "explicit off should remove the callback");
    ShipLua::EventPayload frame{{"frame", std::int64_t{1}}};
    const auto dispatched = host.DispatchEvent("game.frame", frame);
    Check(dispatched.isOk() && dispatched.value->callbacksInvoked == 0,
          "removed callback should not run");
}

void TestRecursivePayloadConversion() {
    ShipLua::LuaApiHostContext context{"oot", "9.1.0", "0.1.0", {"actor.events"}};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(
        MakeManifest("test.payload"), R"lua(
local ship = require("ship")
ship.events.on("actor.init", function(event)
    assert(event.actor.actor_id == 42)
    assert(event.actor.handle.slot == 7)
    assert(event.actor.handle.game == "oot")
    assert(event.actor.tags[1] == "enemy")
    payload_seen = true
end)
)lua");
    Check(loaded.isOk(), "recursive payload script should load");

    ShipLua::EventValue::Object handle{
        {"slot", std::int64_t{7}}, {"generation", std::int64_t{3}}, {"game", "oot"}};
    ShipLua::EventValue::Array tags{ShipLua::EventValue{"enemy"}, ShipLua::EventValue{"flying"}};
    ShipLua::EventValue::Object actor{
        {"handle", std::move(handle)},
        {"actor_id", std::int64_t{42}},
        {"category", std::int64_t{5}},
        {"tags", std::move(tags)},
    };
    ShipLua::EventPayload payload{{"actor", std::move(actor)}};
    const auto dispatched = host.DispatchEvent("actor.init", payload);
    Check(dispatched.isOk() && dispatched.value->failures.empty(),
          "recursive object and array payload should reach Lua");
    Check(host.GetRuntime("test.payload")->DoString("assert(payload_seen)").isOk(),
          "Lua callback should observe the recursive payload");
}

void TestCallbackFailureIsolationAndDisable() {
    ShipLua::LuaApiHostContext context{"mm", "4.2.0", "0.1.0", {}};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    Check(host.LoadModFromManifestAndSource(MakeManifest("a.broken"), R"lua(
local ship = require("ship")
ship.events.on("game.frame", function() error("callback boom") end)
)lua").isOk(), "broken callback mod should load before its callback runs");
    Check(host.LoadModFromManifestAndSource(MakeManifest("b.healthy"), R"lua(
local ship = require("ship")
ship.events.on("game.frame", function() healthy_calls = (healthy_calls or 0) + 1 end)
)lua").isOk(), "healthy callback mod should load");

    for (std::int64_t frame = 1; frame <= 3; ++frame) {
        ShipLua::EventPayload payload{{"frame", frame}};
        const auto outcome = host.DispatchEvent("game.frame", payload);
        Check(outcome.isOk() && outcome.value->callbacksInvoked == 2 &&
                  outcome.value->failures.size() == 1,
              "broken callback should be isolated until disable threshold");
    }
    Check(host.SubscriptionCount() == 1,
          "broken callback should be removed after three consecutive failures");
    ShipLua::EventPayload fourth{{"frame", std::int64_t{4}}};
    const auto outcome = host.DispatchEvent("game.frame", fourth);
    Check(outcome.isOk() && outcome.value->callbacksInvoked == 1 && outcome.value->failures.empty(),
          "healthy callback should remain after broken callback is disabled");
    Check(host.GetRuntime("b.healthy")->DoString("assert(healthy_calls == 4)").isOk(),
          "healthy mod should receive every frame");
}

void TestMissingAndInvalidHostContext() {
    ShipLua::ModHost host{ShipLua::Logger([](auto, const auto&, const auto&) {})};
    const auto loaded = host.LoadModFromManifestAndSource(MakeManifest("test.no_host"), R"lua(
local ship = require("ship")
assert(ship.api.version() == "0.3.0")
local game_ok = pcall(ship.game.id)
local version_ok = pcall(ship.game.host_version)
assert(not game_ok and not version_ok)
local log_ok = pcall(ship.log.info, 123)
assert(not log_ok)
)lua");
    Check(loaded.isOk(), "API-independent functions should work without host identity: " +
                             loaded.message);

    ShipLua::LuaApiHostContext invalid{"invalid", "1.0.0", "0.1.0", {}};
    ShipLua::ModHost invalidHost(invalid, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto rejected = invalidHost.LoadModFromManifestAndSource(
        MakeManifest("test.invalid_host"), "return true");
    Check(!rejected.isOk() && rejected.code == ShipLua::ErrorCode::InvalidArgument,
          "invalid host game id should reject binding installation");

    ShipLua::LuaApiHostContext planned{"mm", "4.2.0", "0.1.0", {"mm.cycle"}};
    ShipLua::ModHost plannedHost(planned, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto plannedRejected = plannedHost.LoadModFromManifestAndSource(
        MakeManifest("test.planned_capability"), "return true");
    Check(!plannedRejected.isOk() && plannedRejected.code == ShipLua::ErrorCode::Unsupported,
          "planned capability should not be advertised by a host binding");
}

void TestUnloadAllCopiesOwnedIds() {
    std::vector<CapturedLog> logs;
    ShipLua::LuaApiHostContext context{"oot", "9.1.0", "0.1.0", {}};
    ShipLua::ModHost host(context, CaptureLogger(logs));
    Check(host.LoadModFromManifestAndSource(MakeManifest("unload.first"), "return true").isOk(),
          "first unload regression mod should load");
    Check(host.LoadModFromManifestAndSource(MakeManifest("unload.second"), "return true").isOk(),
          "second unload regression mod should load");
    host.UnloadAll();
    Check(host.Count() == 0, "UnloadAll should erase every mod without dangling IDs");
    Check(ContainsLog(logs, "unload.first", "unloaded mod") &&
              ContainsLog(logs, "unload.second", "unloaded mod"),
          "UnloadAll should log stable copied IDs after map erasure");
}

void TestBindingInstallRespectsMemoryLimit() {
    ShipLua::LuaApiHostContext context{"oot", "9.1.0", "0.1.0", {}};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(
        MakeManifest("test.low_memory"), "local ship = require('ship')", 30000);
    Check(loaded.isOk() || loaded.code == ShipLua::ErrorCode::HostFailure ||
              loaded.code == ShipLua::ErrorCode::ResourceLimit ||
              loaded.code == ShipLua::ErrorCode::ScriptFailure,
          "low-memory binding installation should return a protected result instead of crashing");
    host.UnloadAll();
}

struct FakeHotkey : ShipLua::HotkeyBinding {
    std::function<void()> onFire;
};

struct FakeHotkeyRegistry : ShipLua::HotkeyRegistry {
    bool Register(const ShipLua::HotkeyBinding& b, std::function<void()> onFire) override {
        for (auto& existing : registered) {
            if (existing.modId == b.modId && existing.id == b.id) {
                static_cast<ShipLua::HotkeyBinding&>(existing) = b;
                existing.onFire = std::move(onFire);
                return true;
            }
        }
        FakeHotkey f;
        static_cast<ShipLua::HotkeyBinding&>(f) = b;
        f.onFire = std::move(onFire);
        registered.push_back(std::move(f));
        return true;
    }
    void UnregisterMod(const std::string& modId) override {
        registered.erase(
            std::remove_if(registered.begin(), registered.end(), [&](const FakeHotkey& hotkey) {
                return hotkey.modId == modId;
            }),
            registered.end());
    }
    void Fire(const std::string& modId, const std::string& id) override {
        for (auto& f : registered) {
            if (f.modId == modId && f.id == id && f.onFire) {
                f.onFire();
            }
        }
    }
    std::vector<ShipLua::HotkeyBinding> Registered() const override {
        std::vector<ShipLua::HotkeyBinding> out;
        for (const auto& f : registered) {
            out.push_back(f);
        }
        return out;
    }
    std::vector<FakeHotkey> registered;
};

void TestHotkeysRegisterFiresCallbackOnBothHosts() {
    for (const std::string game : {"oot", "mm"}) {
        std::vector<CapturedLog> logs;
        auto registry = std::make_shared<FakeHotkeyRegistry>();
        const std::string hostVersion = game == "oot" ? "9.1.0" : "4.2.0";
        ShipLua::LuaApiHostContext context{game, hostVersion, "0.2.0", {}, registry};
        ShipLua::ModHost host(context, CaptureLogger(logs));

    const std::string source = R"lua(
local ship = require("ship")
FIRED = 0
assert(ship.hotkeys ~= nil)
local ok = ship.hotkeys.register("jump", {default="K", label="Pulo"}, function()
    FIRED = FIRED + 1
end)
assert(ok == true)
)lua";
        Check(host.LoadModFromManifestAndSource(MakeManifest("community.jump"), source).isOk(),
              "jump mod should load with a hotkey registry on " + game);
        Check(registry->registered.size() == 1, "one hotkey registered on " + game);
        if (!registry->registered.empty()) {
            Check(registry->registered[0].id == "jump", "id stored on " + game);
            Check(registry->registered[0].defaultKey == "K", "defaultKey stored on " + game);
            Check(registry->registered[0].label == "Pulo", "label stored on " + game);
            ShipLua::LuaRuntime* runtime = host.GetRuntime("community.jump");
            Check(runtime != nullptr, "jump runtime should exist on " + game);
            registry->Fire("community.jump", "jump");
            if (runtime != nullptr) {
                const auto assertFired = runtime->DoString("assert(FIRED == 1)");
                Check(assertFired.isOk(), "hotkey callback should fire once on " + game);
            }
        }
        Check(host.UnloadMod("community.jump").isOk(), "hotkey mod should unload on " + game);
        Check(registry->registered.empty(), "unload should remove host binding on " + game);
        registry->Fire("community.jump", "jump");
    }
}

void TestHotkeyReplacementAndValidation() {
    auto registry = std::make_shared<FakeHotkeyRegistry>();
    ShipLua::LuaApiHostContext context{"oot", "9.1.0", "0.2.0", {}, registry};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(MakeManifest("test.hotkeys"), R"lua(
local ship = require("ship")
COUNT = 0
assert(ship.hotkeys.register("action", {default="F8", label="Primeira"}, function() COUNT = 1 end))
assert(ship.hotkeys.register("action", {default="F9", label="Segunda"}, function() COUNT = 2 end))
assert(not pcall(ship.hotkeys.register, "Invalid ID", function() end))
assert(not pcall(ship.hotkeys.register, "valid", {default=8}, function() end))
)lua");
    Check(loaded.isOk(), "replacement and validation script should load");
    Check(registry->registered.size() == 1, "replacement should keep one host binding");
    if (!registry->registered.empty()) {
        Check(registry->registered[0].defaultKey == "F9", "replacement should update metadata");
        registry->Fire("test.hotkeys", "action");
        Check(host.GetRuntime("test.hotkeys")->DoString("assert(COUNT == 2)").isOk(),
              "replacement should invoke only the latest callback");
    }
    host.UnloadAll();
    Check(registry->registered.empty(), "UnloadAll should remove replaced hotkey binding");
}

void TestHotkeyFailureIsolationAndDisable() {
    std::vector<CapturedLog> logs;
    auto registry = std::make_shared<FakeHotkeyRegistry>();
    ShipLua::LuaApiHostContext context{"mm", "4.2.0", "0.2.0", {}, registry};
    ShipLua::ModHost host(context, CaptureLogger(logs));
    const auto loaded = host.LoadModFromManifestAndSource(MakeManifest("test.broken_hotkey"), R"lua(
local ship = require("ship")
ATTEMPTS = 0
assert(ship.hotkeys.register("broken", {default="F10"}, function()
    ATTEMPTS = ATTEMPTS + 1
    error("hotkey boom")
end))
)lua");
    Check(loaded.isOk(), "broken hotkey mod should load before callback runs");
    for (int attempt = 0; attempt < 4; ++attempt) {
        registry->Fire("test.broken_hotkey", "broken");
    }
    Check(host.GetRuntime("test.broken_hotkey")->DoString("assert(ATTEMPTS == 3)").isOk(),
          "hotkey callback should become inactive after three failures");
    Check(ContainsLog(logs, "test.broken_hotkey",
                      "hotkey 'broken' desativada após falhas repetidas"),
          "failure threshold should be logged with the mod id");
    host.UnloadAll();
}

void TestHotkeysNullRegistryIsNoOp() {
    std::vector<CapturedLog> logs;
    ShipLua::LuaApiHostContext context{"mm", "4.2.0", "0.1.0"};  // hotkeys left null
    ShipLua::ModHost host(context, CaptureLogger(logs));

    const std::string source = R"lua(
local ship = require("ship")
assert(ship.hotkeys ~= nil, "ship.hotkeys is common API surface")
local ok = pcall(ship.hotkeys.register, "jump", function() end)
assert(not ok, "registration without a host registry must report unsupported")
)lua";
    Check(host.LoadModFromManifestAndSource(MakeManifest("community.jump"), source).isOk(),
          "mod loads even without a hotkey registry");
    host.UnloadAll();
}

ShipLua::SemVersion MakeVersion(std::uint64_t major, std::uint64_t minor, std::uint64_t patch) {
    ShipLua::SemVersion version;
    version.major = major;
    version.minor = minor;
    version.patch = patch;
    return version;
}

std::shared_ptr<ShipLua::CapabilityRegistry> MakeTestRegistry() {
    auto registry = std::make_shared<ShipLua::CapabilityRegistry>();

    ShipLua::CapabilityProvider spawnNative;
    spawnNative.name = "shipwright-native";
    spawnNative.providerVersion = MakeVersion(0, 4, 0);
    spawnNative.capabilityVersion = MakeVersion(1, 1, 0);
    spawnNative.games = {"oot"};
    spawnNative.stability = ShipLua::CapabilityStability::Preview;
    spawnNative.permissions = {"world.entities.create"};
    spawnNative.limits.perMod = 16;
    spawnNative.description = "Spawn nativo de atores";
    Check(registry->Register("actor.spawn", spawnNative).isOk(), "native spawn offer registers");

    ShipLua::CapabilityProvider spawnMock;
    spawnMock.name = "mock-runtime";
    spawnMock.providerVersion = MakeVersion(0, 4, 3);
    spawnMock.capabilityVersion = MakeVersion(1, 0, 0);
    spawnMock.games = {"oot", "mm"};
    spawnMock.stability = ShipLua::CapabilityStability::Experimental;
    Check(registry->Register("actor.spawn", spawnMock).isOk(), "mock spawn offer registers");

    ShipLua::CapabilityProvider cycle;
    cycle.name = "2ship-native";
    cycle.providerVersion = MakeVersion(0, 3, 1);
    cycle.capabilityVersion = MakeVersion(1, 0, 0);
    cycle.games = {"mm"};
    cycle.stability = ShipLua::CapabilityStability::Stable;
    Check(registry->Register("mm.cycle", cycle).isOk(), "mm cycle offer registers");

    ShipLua::CapabilityProvider events;
    events.name = "shipwright-native";
    events.providerVersion = MakeVersion(0, 4, 0);
    events.capabilityVersion = MakeVersion(1, 0, 0);
    events.games = {"oot", "mm"};
    events.stability = ShipLua::CapabilityStability::Stable;
    Check(registry->Register("core.events", events).isOk(), "core events offer registers");

    return registry;
}

void TestCapabilityRegistryLuaApi() {
    auto registry = MakeTestRegistry();
    ShipLua::LuaApiHostContext context{"oot", "9.1.0", "0.1.0", {}, nullptr, registry};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(
        MakeManifest("test.capability_registry"), "local ship = require('ship')");
    Check(loaded.isOk(), "capability registry mod should load: " + loaded.message);
    auto* runtime = host.GetRuntime("test.capability_registry");
    Check(runtime != nullptr, "capability registry runtime should exist");
    if (runtime == nullptr) {
        return;
    }
    struct Section {
        const char* name;
        const char* source;
    };
    const std::vector<Section> sections = {
        {"has", R"lua(
local ship = require("ship")
assert(ship.capabilities.has("actor.spawn"))
assert(ship.capabilities.has("actor.spawn", ">=1.0"))
assert(ship.capabilities.has("actor.spawn", ">=1.1 <2.0"))
assert(not ship.capabilities.has("actor.spawn", ">=2.0"))
assert(ship.capabilities.has("core.events"))
assert(not ship.capabilities.has("mm.cycle"))          -- exclusiva de MM em host OoT
assert(not ship.capabilities.has("missing.thing"))
)lua"},
        {"info", R"lua(
local ship = require("ship")
local info = ship.capabilities.info("actor.spawn")
assert(info.id == "actor.spawn")
assert(info.version == "1.1.0")
assert(info.provider == "shipwright-native")
assert(info.provider_version == "0.4.0")
assert(info.stability == "preview")
assert(info.permissions[1] == "world.entities.create")
assert(info.limits.per_mod == 16)
assert(#info.games == 1 and info.games[1] == "oot")
assert(info.description == "Spawn nativo de atores")
assert(ship.capabilities.info("mm.cycle") == nil)      -- filtro de jogo
assert(ship.capabilities.info("missing.thing") == nil)
)lua"},
        {"providers", R"lua(
local ship = require("ship")
local providers = ship.capabilities.providers("actor.spawn")
assert(#providers == 2)
assert(providers[1] == "mock-runtime")
assert(providers[2] == "shipwright-native")
assert(#ship.capabilities.providers("missing.thing") == 0)
)lua"},
        {"list", R"lua(
local ship = require("ship")
local all = ship.capabilities.list()
assert(#all == 2 and all[1] == "actor.spawn" and all[2] == "core.events")
local mm = ship.capabilities.list({ game = "mm" })
assert(#mm == 3 and mm[1] == "actor.spawn" and mm[2] == "core.events" and mm[3] == "mm.cycle")
local stable = ship.capabilities.list({ stability = "stable" })
assert(#stable == 1 and stable[1] == "core.events")
local preview = ship.capabilities.list({ stability = "preview" })
assert(#preview == 1 and preview[1] == "actor.spawn")
)lua"},
        {"err_has_range", R"lua(
local ship = require("ship")
assert(not pcall(ship.capabilities.has, "actor.spawn", "banana"))
)lua"},
        {"err_has_type", R"lua(
local ship = require("ship")
assert(not pcall(ship.capabilities.has, 123))
)lua"},
        {"err_list_type", R"lua(
local ship = require("ship")
assert(not pcall(ship.capabilities.list, "oot"))
)lua"},
        {"err_list_game", R"lua(
local ship = require("ship")
assert(not pcall(ship.capabilities.list, { game = "n64" }))
)lua"},
        {"err_list_stability", R"lua(
local ship = require("ship")
assert(not pcall(ship.capabilities.list, { stability = "beta" }))
)lua"},
        {"err_info_noarg", R"lua(
local ship = require("ship")
assert(not pcall(ship.capabilities.info))
)lua"},
    };
    for (const Section& section : sections) {
        const auto result = runtime->DoString(section.source);
        Check(result.isOk(), std::string("section '") + section.name +
                                 "' should pass: " + result.message);
    }
    host.UnloadAll();
}

void TestLegacyContextSynthesizesCapabilityDescriptors() {
    ShipLua::LuaApiHostContext context{"oot", "9.1.0", "0.1.0", {"scene.events", "actor.events"}};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(
        MakeManifest("test.legacy_capabilities"), R"lua(
local ship = require("ship")
assert(ship.capabilities.has("scene.events"))
assert(ship.capabilities.has("scene.events", ">=0.2"))
assert(not ship.capabilities.has("scene.events", ">=1.0"))
assert(not ship.capabilities.has("mm.cycle"))
local all = ship.capabilities.list()
assert(#all == 2 and all[1] == "actor.events" and all[2] == "scene.events")
local info = ship.capabilities.info("scene.events")
assert(info ~= nil)
assert(info.provider == "legacy-host")
assert(info.version == "0.3.0")
assert(info.provider_version == "9.1.0")
assert(info.stability == "stable")
assert(#info.games == 2)
local providers = ship.capabilities.providers("scene.events")
assert(#providers == 1 and providers[1] == "legacy-host")
assert(ship.capabilities.info("mm.cycle") == nil)
)lua");
    Check(loaded.isOk(),
          "legacy flat context should synthesize descriptors: " + loaded.message);
    host.UnloadAll();
}

void TestCapabilityRegistrySharedAcrossMods() {
    auto registry = MakeTestRegistry();
    ShipLua::LuaApiHostContext context{"mm", "4.2.0", "0.1.0", {}, nullptr, registry};
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    Check(host.LoadModFromManifestAndSource(MakeManifest("cap.first"), R"lua(
local ship = require("ship")
assert(ship.capabilities.has("mm.cycle"))
assert(ship.capabilities.info("mm.cycle").stability == "stable")
)lua").isOk(), "first mod should read the shared registry");
    Check(host.LoadModFromManifestAndSource(MakeManifest("cap.second"), R"lua(
local ship = require("ship")
local providers = ship.capabilities.providers("mm.cycle")
assert(#providers == 1 and providers[1] == "2ship-native")
assert(not ship.capabilities.has("actor.spawn", ">=1.1"))  -- oferta 1.1 é exclusiva de OoT
assert(ship.capabilities.has("actor.spawn"))               -- mock-runtime cobre MM
)lua").isOk(), "second mod should see the same registry with mm filtering");
    host.UnloadAll();
}

void TestWorldTravelCapabilityUsesHostHandler() {
    std::optional<ShipLua::WorldDestination> requested;
    ShipLua::LuaApiHostContext context;
    context.gameId = "oot";
    context.hostVersion = "9.1.0";
    context.capabilities = {"world.travel"};
    context.worldTravel = [&requested](const ShipLua::WorldDestination& destination) {
        requested = destination;
        return ShipLua::Result<void>::ok();
    };
    ShipLua::ModHost host(context, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto loaded = host.LoadModFromManifestAndSource(
        MakeManifest("test.world_travel"), R"lua(
local ship = require("ship")
assert(ship.capabilities.has("world.travel"))
assert(not pcall(ship.world.travel))
assert(ship.world.travel("mm", "mm.clock_tower.entrance"))
)lua");
    Check(loaded.isOk(), "world.travel should load with an injected host handler: " +
                             loaded.message);
    Check(requested.has_value() && requested->world == ShipLua::WorldId::Mm &&
              requested->id == "mm.clock_tower.entrance",
          "world.travel should pass a validated logical destination to the host");

    ShipLua::LuaApiHostContext invalid;
    invalid.gameId = "oot";
    invalid.hostVersion = "9.1.0";
    invalid.capabilities = {"world.travel"};
    ShipLua::ModHost invalidHost(invalid, ShipLua::Logger([](auto, const auto&, const auto&) {}));
    const auto rejected = invalidHost.LoadModFromManifestAndSource(
        MakeManifest("test.world_travel_without_handler"), "return true");
    Check(!rejected.isOk() && rejected.code == ShipLua::ErrorCode::InvalidState,
          "a host must not advertise world.travel without a handler");
}

} // namespace

int main() {
    TestHelloWorldOnBothHosts();
    TestCapabilitiesAndExplicitOff();
    TestRecursivePayloadConversion();
    TestCallbackFailureIsolationAndDisable();
    TestMissingAndInvalidHostContext();
    TestUnloadAllCopiesOwnedIds();
    TestBindingInstallRespectsMemoryLimit();
    TestHotkeysRegisterFiresCallbackOnBothHosts();
    TestHotkeyReplacementAndValidation();
    TestHotkeyFailureIsolationAndDisable();
    TestHotkeysNullRegistryIsNoOp();
    TestCapabilityRegistryLuaApi();
    TestLegacyContextSynthesizesCapabilityDescriptors();
    TestCapabilityRegistrySharedAcrossMods();
    TestWorldTravelCapabilityUsesHostHandler();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All Lua API binding checks passed\n";
    return 0;
}
