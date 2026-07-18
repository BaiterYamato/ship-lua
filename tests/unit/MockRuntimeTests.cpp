#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "shiplua/host/ModHost.h"
#include "shiplua/mock/MockRuntime.h"
#include "shiplua/runtime/LuaRuntime.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::Manifest MakeManifest(std::string id) {
    ShipLua::Manifest manifest;
    manifest.id = std::move(id);
    manifest.name = "Mock test";
    manifest.version = "0.1.0";
    manifest.apiRange = ">=0.1 <0.3";
    manifest.entrypoint = "main.lua";
    return manifest;
}

ShipLua::Result<ShipLua::MockRuntime> CreateLoaded(const ShipLua::Manifest& manifest,
                                                   const std::string& source,
                                                   ShipLua::MockHostOptions options = {}) {
    auto created = ShipLua::MockRuntime::Create(std::move(options));
    if (!created.isOk()) {
        return ShipLua::Result<ShipLua::MockRuntime>::err(created.code, created.message);
    }
    const auto loaded = created.value->LoadModFromManifestAndSource(manifest, source);
    if (!loaded.isOk()) {
        return ShipLua::Result<ShipLua::MockRuntime>::err(loaded.code, loaded.message);
    }
    return created;
}

bool StorageIs(const ShipLua::MockRuntime& mock, const std::string& key,
               const std::int64_t expected) {
    auto value = mock.Storage().Get(mock.ModId(), key);
    return value.isOk() && value.value->has_value() &&
           std::holds_alternative<std::int64_t>(**value.value) &&
           std::get<std::int64_t>(**value.value) == expected;
}

bool HasLog(const ShipLua::MockRuntime& mock, const std::string& needle) {
    for (const ShipLua::MockLogEntry& entry : mock.Logs()) {
        if (entry.message.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void TestCreateValidation() {
    ShipLua::MockHostOptions invalidGame;
    invalidGame.gameId = "snes";
    Check(!ShipLua::MockRuntime::Create(invalidGame).isOk(),
          "mock should reject a game id outside the contract");

    ShipLua::MockHostOptions unknownCapability;
    unknownCapability.extraCapabilities = {"actor.spawn"};
    Check(!ShipLua::MockRuntime::Create(unknownCapability).isOk(),
          "mock should reject capabilities that are not contracted");

    ShipLua::MockHostOptions knownExtra;
    knownExtra.extraCapabilities = {"scene.events"};
    Check(ShipLua::MockRuntime::Create(knownExtra).isOk(),
          "mock should accept contracted extra capabilities");

    ShipLua::MockHostOptions mmGame;
    mmGame.gameId = "mm";
    Check(ShipLua::MockRuntime::Create(mmGame).isOk(), "mock should support game id mm");
}

void TestCoreCapabilitiesAdvertised() {
    const std::string source = R"lua(
local ship = require("ship")
assert(ship.capabilities.has("core.events"), "core.events ausente")
assert(ship.capabilities.has("core.timers"), "core.timers ausente")
assert(ship.capabilities.has("core.storage"), "core.storage ausente")
assert(ship.capabilities.has("core.input"), "core.input ausente")
assert(not ship.capabilities.has("actor.spawn"), "actor.spawn não deveria existir")
)lua";
    auto mock = CreateLoaded(MakeManifest("test.capabilities"), source);
    Check(mock.isOk(), "mod should see the four core capabilities in the mock");
}

void TestTimersThroughLua() {
    const std::string source = R"lua(
local ship = require("ship")
ship.storage.set("after_fired", 0)
ship.storage.set("every_fired", 0)
ship.timer.after(5, function()
    ship.storage.set("after_fired", 1)
end)
handle = ship.timer.every(5, function()
    local fired = tonumber(ship.storage.get("every_fired")) or 0
    ship.storage.set("every_fired", fired + 1)
end)
)lua";
    auto mock = CreateLoaded(MakeManifest("test.timers"), source);
    Check(mock.isOk(), "timer mod should load in the mock");
    if (!mock.isOk()) {
        return;
    }

    Check(mock.value->AdvanceFrames(4).isOk(), "advance 4 should succeed");
    Check(StorageIs(*mock.value, "after_fired", 0), "one-shot timer must not fire before due");
    Check(mock.value->AdvanceFrames(1).isOk(), "advance to due frame should succeed");
    Check(StorageIs(*mock.value, "after_fired", 1), "one-shot timer should fire at frame 5");
    Check(StorageIs(*mock.value, "every_fired", 1), "repeating timer should have fired once");

    mock.value->AdvanceFrames(5);
    Check(StorageIs(*mock.value, "every_fired", 2), "repeating timer should fire again at 10");
    Check(mock.value->CurrentFrame() == 10, "mock should track the absolute frame counter");

    // Cancela o timer repetitivo via Lua e confirma que não dispara mais.
    ShipLua::LuaRuntime* runtime = mock.value->Host().GetRuntime("test.timers");
    Check(runtime != nullptr, "mod runtime should be reachable");
    if (runtime != nullptr) {
        Check(runtime->DoString("local ship = require('ship')\n"
                                "assert(ship.timer.cancel(handle) == true)")
                  .isOk(),
              "ship.timer.cancel should accept the mod's own handle");
        Check(runtime->DoString("local ship = require('ship')\n"
                                "assert(pcall(ship.timer.cancel, handle) == false)")
                  .isOk(),
              "cancelling twice should raise a Lua error");
    }
    mock.value->AdvanceFrames(5);
    Check(StorageIs(*mock.value, "every_fired", 2), "cancelled timer must not fire again");

    // Handles desconhecidos não podem ser cancelados.
    Check(runtime != nullptr &&
              runtime->DoString("local ship = require('ship')\n"
                                "assert(pcall(ship.timer.cancel, 99999) == false)")
                  .isOk(),
          "cancelling an unknown handle should raise a Lua error");
}

void TestTimerArgumentValidation() {
    const std::string source = R"lua(
local ship = require("ship")
assert(pcall(ship.timer.after, 0, function() end) == false, "frames zero deve falhar")
assert(pcall(ship.timer.every, -3, function() end) == false, "frames negativas devem falhar")
assert(pcall(ship.timer.after, 5) == false, "callback ausente deve falhar")
assert(pcall(ship.timer.every, "x", function() end) == false, "frames textual deve falhar")
)lua";
    auto mock = CreateLoaded(MakeManifest("test.timer_args"), source);
    Check(mock.isOk(), "invalid timer arguments should raise Lua errors");
}

void TestEventsLifecycleAndFrames() {
    const std::string source = R"lua(
local ship = require("ship")
ship.storage.set("ready_count", 0)
ship.storage.set("frame_count", 0)
ship.events.on("game.ready", function(event)
    local count = tonumber(ship.storage.get("ready_count")) or 0
    ship.storage.set("ready_count", count + 1)
    ship.log.info("ready em " .. tostring(event.game_id))
end)
ship.events.on("game.frame", function(event)
    local count = tonumber(ship.storage.get("frame_count")) or 0
    ship.storage.set("frame_count", count + 1)
end)
ship.events.on("game.shutdown", function()
    ship.log.info("shutdown recebido")
end)
)lua";
    auto mock = CreateLoaded(MakeManifest("test.lifecycle"), source);
    Check(mock.isOk(), "lifecycle mod should load");
    if (!mock.isOk()) {
        return;
    }

    Check(mock.value->EmitGameReady().isOk(), "game.ready should dispatch");
    Check(StorageIs(*mock.value, "ready_count", 1), "game.ready should reach the mod");
    Check(HasLog(*mock.value, "ready em oot"), "mod log should be captured by the mock");

    mock.value->AdvanceFrames(3);
    Check(StorageIs(*mock.value, "frame_count", 3), "each advanced frame emits game.frame");

    Check(mock.value->EmitGameShutdown().isOk(), "game.shutdown should dispatch");
    Check(HasLog(*mock.value, "shutdown recebido"), "shutdown log should be captured");
}

void TestInputSimulation() {
    const std::string source = R"lua(
local ship = require("ship")
ship.storage.set("press_count", 0)
ship.storage.set("hotkey_events", 0)
ship.hotkeys.register("action", { default = "K", label = "Ação" }, function()
    local count = tonumber(ship.storage.get("press_count")) or 0
    ship.storage.set("press_count", count + 1)
end)
ship.events.on("input.hotkey", function(event)
    local count = tonumber(ship.storage.get("hotkey_events")) or 0
    ship.storage.set("hotkey_events", count + 1)
end)
)lua";
    auto mock = CreateLoaded(MakeManifest("test.input"), source);
    Check(mock.isOk(), "input mod should load");
    if (!mock.isOk()) {
        return;
    }

    Check(mock.value->PressKey("X") == 0, "unknown key should fire nothing");
    Check(mock.value->PressKey("K") == 1, "registered key should fire one hotkey");
    Check(StorageIs(*mock.value, "press_count", 1), "hotkey callback should run on press");
    Check(StorageIs(*mock.value, "hotkey_events", 1),
          "pressing a key should emit input.hotkey");

    Check(mock.value->TriggerHotkey("action"), "trigger by id should find the binding");
    Check(StorageIs(*mock.value, "press_count", 2), "trigger should run the callback");
    Check(!mock.value->TriggerHotkey("missing"), "unknown hotkey id should not trigger");
}

void TestStorageThroughLua() {
    const std::string source = R"lua(
local ship = require("ship")
assert(ship.storage.get("vazio") == nil, "chave ausente deve ser nil")
assert(ship.storage.get("vazio", "padrao") == "padrao", "default deve ser retornado")
assert(ship.storage.set("inteiro", 41) == true)
assert(ship.storage.set("texto", "ola") == true)
assert(ship.storage.set("booleano", true) == true)
assert(ship.storage.set("decimal", 1.5) == true)
assert(ship.storage.get("inteiro") == 41)
assert(ship.storage.get("texto") == "ola")
assert(ship.storage.get("booleano") == true)
assert(ship.storage.get("decimal") == 1.5)
assert(pcall(ship.storage.set, "tabela", {}) == false, "tabela deve ser rejeitada")
assert(pcall(ship.storage.set, 1, "x") == false, "chave não textual deve falhar")
assert(ship.storage.delete("texto") == true, "delete deve confirmar remoção")
assert(ship.storage.delete("texto") == false, "delete repetido deve retornar false")
assert(ship.storage.get("texto") == nil)
assert(ship.storage.clear() == 3, "clear deve remover as 3 chaves restantes")
assert(ship.storage.get("inteiro") == nil, "clear deve esvaziar o namespace")
)lua";
    auto mock = CreateLoaded(MakeManifest("test.storage"), source);
    Check(mock.isOk(), "storage API should roundtrip through Lua with type preservation");
}

void TestResetKeepsWiring() {
    const std::string source = R"lua(
local ship = require("ship")
ship.timer.every(3, function()
    local fired = tonumber(ship.storage.get("fired")) or 0
    ship.storage.set("fired", fired + 1)
end)
)lua";
    auto mock = CreateLoaded(MakeManifest("test.reset"), source);
    Check(mock.isOk(), "reset mod should load");
    if (!mock.isOk()) {
        return;
    }

    mock.value->AdvanceFrames(3);
    Check(StorageIs(*mock.value, "fired", 1), "timer should fire before reset");
    mock.value->ResetSimulation();
    Check(mock.value->Logs().empty(), "reset should clear captured logs");
    auto value = mock.value->Storage().Get("test.reset", "fired");
    Check(value.isOk() && !value.value->has_value(), "reset should clear mod storage");

    mock.value->AdvanceFrames(3);
    Check(StorageIs(*mock.value, "fired", 1), "reset must not cancel the mod's timers");
}

void TestUnloadCancelsTimers() {
    const std::string source = R"lua(
local ship = require("ship")
ship.timer.every(1, function() end)
ship.timer.every(2, function() end)
)lua";
    auto mock = CreateLoaded(MakeManifest("test.unload"), source);
    Check(mock.isOk(), "unload mod should load");
    if (!mock.isOk()) {
        return;
    }
    Check(mock.value->Timers().TimerCount() == 2, "both timers should be scheduled");
    Check(mock.value->UnloadMod().isOk(), "unload should succeed");
    Check(!mock.value->IsLoaded(), "mock should report no mod after unload");
    Check(mock.value->Timers().TimerCount() == 0, "unload should cancel the mod's timers");
}

void TestSingleModEnforcement() {
    auto mock = CreateLoaded(MakeManifest("test.first"), "local ship = require('ship')");
    Check(mock.isOk(), "first mod should load");
    if (!mock.isOk()) {
        return;
    }
    const auto second =
        mock.value->LoadModFromManifestAndSource(MakeManifest("test.second"),
                                                 "local ship = require('ship')");
    Check(!second.isOk(), "mock should refuse a second mod instance");
}

void TestMissingProvidersDegradeGracefully() {
    // Host sem providers de timer/storage: as APIs existem mas falham com
    // 'unsupported' em vez de quebrar o carregamento do mod.
    const std::string source = R"lua(
local ship = require("ship")
assert(type(ship.timer) == "table" and type(ship.timer.after) == "function")
assert(type(ship.storage) == "table" and type(ship.storage.get) == "function")
assert(pcall(ship.timer.after, 1, function() end) == false, "timer sem provider deve falhar")
assert(pcall(ship.storage.get, "k") == false, "storage sem provider deve falhar")
)lua";
    ShipLua::LuaApiHostContext context{"oot", "9.9.9", "0.2.0", {}};
    ShipLua::ModHost host(context);
    const auto loaded =
        host.LoadModFromManifestAndSource(MakeManifest("test.no_providers"), source);
    Check(loaded.isOk(), "mod should load without providers and fail gracefully at call time");
}

} // namespace

int main() {
    TestCreateValidation();
    TestCoreCapabilitiesAdvertised();
    TestTimersThroughLua();
    TestTimerArgumentValidation();
    TestEventsLifecycleAndFrames();
    TestInputSimulation();
    TestStorageThroughLua();
    TestResetKeepsWiring();
    TestUnloadCancelsTimers();
    TestSingleModEnforcement();
    TestMissingProvidersDegradeGracefully();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all mock_runtime tests passed\n";
    return 0;
}
