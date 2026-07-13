#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "shiplua/host/ModHost.h"

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
    manifest.apiRange = ">=0.1 <0.2";
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
assert(ship.api.version() == "0.1.0")
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
            {"api_version", "0.1.0"},
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
assert(ship.api.version() == "0.1.0")
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

} // namespace

int main() {
    TestHelloWorldOnBothHosts();
    TestCapabilitiesAndExplicitOff();
    TestRecursivePayloadConversion();
    TestCallbackFailureIsolationAndDisable();
    TestMissingAndInvalidHostContext();
    TestUnloadAllCopiesOwnedIds();
    TestBindingInstallRespectsMemoryLimit();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All Lua API binding checks passed\n";
    return 0;
}
