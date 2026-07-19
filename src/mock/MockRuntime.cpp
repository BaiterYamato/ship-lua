#include "shiplua/mock/MockRuntime.h"

#include <algorithm>
#include <filesystem>
#include <utility>

#include "shiplua/api/LuaApiBinding.h"
#include "shiplua/generated/ApiBindings.h"
#include "shiplua/manifest/ManifestParser.h"
#include "shiplua/runtime/LuaRuntime.h"

namespace ShipLua {

// ---------------------------------------------------------------------------
// MockHotkeyRegistry
// ---------------------------------------------------------------------------

bool MockHotkeyRegistry::Register(const HotkeyBinding& binding,
                                  std::function<void()> onFire) {
    if (binding.id.empty() || binding.modId.empty()) {
        return false;
    }
    mEntries[binding.modId][binding.id] = Entry{binding, std::move(onFire)};
    return true;
}

void MockHotkeyRegistry::UnregisterMod(const std::string& modId) {
    mEntries.erase(modId);
}

void MockHotkeyRegistry::Fire(const std::string& modId, const std::string& id) {
    const auto mod = mEntries.find(modId);
    if (mod == mEntries.end()) {
        return;
    }
    const auto entry = mod->second.find(id);
    if (entry == mod->second.end() || !entry->second.onFire) {
        return;
    }
    entry->second.onFire();
}

std::vector<HotkeyBinding> MockHotkeyRegistry::Registered() const {
    std::vector<HotkeyBinding> bindings;
    for (const auto& [modId, entries] : mEntries) {
        (void)modId;
        for (const auto& [id, entry] : entries) {
            (void)id;
            bindings.push_back(entry.binding);
        }
    }
    return bindings;
}

std::vector<HotkeyBinding> MockHotkeyRegistry::PressKey(const std::string& key) {
    std::vector<HotkeyBinding> matched;
    for (auto& [modId, entries] : mEntries) {
        (void)modId;
        for (auto& [id, entry] : entries) {
            (void)id;
            if (entry.binding.defaultKey == key) {
                matched.push_back(entry.binding);
                if (entry.onFire) {
                    entry.onFire();
                }
            }
        }
    }
    return matched;
}

std::optional<HotkeyBinding> MockHotkeyRegistry::BindingFor(const std::string& modId,
                                                            const std::string& id) const {
    const auto mod = mEntries.find(modId);
    if (mod == mEntries.end()) {
        return std::nullopt;
    }
    const auto entry = mod->second.find(id);
    if (entry == mod->second.end()) {
        return std::nullopt;
    }
    return entry->second.binding;
}

std::size_t MockHotkeyRegistry::Count() const {
    std::size_t count = 0;
    for (const auto& [modId, entries] : mEntries) {
        (void)modId;
        count += entries.size();
    }
    return count;
}

// ---------------------------------------------------------------------------
// MockRuntime
// ---------------------------------------------------------------------------

namespace {

Logger CaptureLogger(std::shared_ptr<std::vector<MockLogEntry>> logs) {
    return Logger([logs = std::move(logs)](LogLevel level, const std::string& modId,
                                           const std::string& message) {
        logs->push_back(MockLogEntry{level, modId, message});
    });
}

LuaApiHostContext BuildHostContext(const MockHostOptions& options,
                                   std::shared_ptr<MockHotkeyRegistry> hotkeys,
                                   std::shared_ptr<FrameTimerScheduler> timers,
                                   std::shared_ptr<KeyValueStorage> storage,
                                   std::shared_ptr<MockActorProvider> actors) {
    LuaApiHostContext context;
    context.gameId = options.gameId;
    context.hostVersion = options.hostVersion;
    context.runtimeVersion = options.runtimeVersion;
    context.capabilities = MockRuntime::CoreCapabilities();
    context.capabilities.insert(context.capabilities.end(),
                                options.extraCapabilities.begin(),
                                options.extraCapabilities.end());
    context.hotkeys = std::move(hotkeys);
    context.timers = std::move(timers);
    context.storage = std::move(storage);
    context.actors = std::move(actors);
    return context;
}

} // namespace

MockRuntime::MockRuntime(MockHostOptions options)
    : mOptions(std::move(options)),
      mLogs(std::make_shared<std::vector<MockLogEntry>>()),
      mLogger(CaptureLogger(mLogs)),
      mTimers(std::make_shared<FrameTimerScheduler>()),
      mStorage(std::make_shared<KeyValueStorage>()),
      mHotkeys(std::make_shared<MockHotkeyRegistry>()),
      mActors(std::make_shared<MockActorProvider>(mOptions.gameId)),
      mHost(BuildHostContext(mOptions, mHotkeys, mTimers, mStorage, mActors), mLogger) {}

std::vector<std::string> MockRuntime::CoreCapabilities() {
    return {"core.events", "core.timers", "core.storage", "core.input",
            "actor.spawn", "actor.destroy", "actor.exists"};
}

Result<MockRuntime> MockRuntime::Create(MockHostOptions options) {
    if (options.gameId != "oot" && options.gameId != "mm") {
        return Result<MockRuntime>::err(ErrorCode::InvalidArgument,
                                        "mock exige game id 'oot' ou 'mm'");
    }
    const auto valid = ValidateLuaApiHostContext(
        BuildHostContext(options, nullptr, nullptr, nullptr,
                         std::make_shared<MockActorProvider>(options.gameId)));
    if (!valid.isOk()) {
        return Result<MockRuntime>::err(valid.code, valid.message);
    }
    return Result<MockRuntime>::ok(MockRuntime(std::move(options)));
}

Result<void> MockRuntime::LoadModFromDirectory(const std::string& dir) {
    if (IsLoaded()) {
        return Result<void>::err(
            ErrorCode::InvalidState,
            "mock já hospeda um mod; crie um MockRuntime novo por arquivo de teste");
    }
    auto parsed = ParseManifestFile((std::filesystem::path(dir) / "manifest.toml").string());
    if (!parsed.isOk()) {
        return Result<void>::err(parsed.code, parsed.message);
    }
    const std::string id = parsed.value->id;
    auto loaded = mHost.LoadModFromDirectory(dir, mOptions.memoryLimitBytes);
    if (!loaded.isOk()) {
        return loaded;
    }
    mModId = id;
    return Result<void>::ok();
}

Result<void> MockRuntime::LoadModFromManifestAndSource(const Manifest& manifest,
                                                       const std::string& luaSource) {
    if (IsLoaded()) {
        return Result<void>::err(
            ErrorCode::InvalidState,
            "mock já hospeda um mod; crie um MockRuntime novo por arquivo de teste");
    }
    auto loaded = mHost.LoadModFromManifestAndSource(manifest, luaSource,
                                                     mOptions.memoryLimitBytes);
    if (!loaded.isOk()) {
        return loaded;
    }
    mModId = manifest.id;
    return Result<void>::ok();
}

Result<void> MockRuntime::UnloadMod() {
    if (!IsLoaded()) {
        return Result<void>::err(ErrorCode::InvalidHandle, "nenhum mod carregado no mock");
    }
    auto unloaded = mHost.UnloadMod(mModId);
    if (!unloaded.isOk()) {
        return unloaded;
    }
    mModId.clear();
    return Result<void>::ok();
}

bool MockRuntime::IsLoaded() const {
    return !mModId.empty();
}

const std::string& MockRuntime::ModId() const {
    return mModId;
}

lua_State* MockRuntime::ModState() {
    if (!IsLoaded()) {
        return nullptr;
    }
    LuaRuntime* runtime = mHost.GetRuntime(mModId);
    return runtime != nullptr ? runtime->State() : nullptr;
}

Result<DispatchOutcome> MockRuntime::EmitGameReady() {
    return EmitEvent("game.ready",
                     EventPayload{
                         {"game_id", mOptions.gameId},
                         {"host_version", mOptions.hostVersion},
                         {"runtime_version", mOptions.runtimeVersion},
                         {"api_version", std::string(Generated::kApiVersion)},
                     });
}

Result<DispatchOutcome> MockRuntime::EmitGameShutdown() {
    return EmitEvent("game.shutdown", EventPayload{});
}

Result<DispatchOutcome> MockRuntime::EmitEvent(const std::string& name, EventPayload payload) {
    auto outcome = mHost.DispatchEvent(name, payload);
    if (outcome.isOk()) {
        for (const CallbackFailure& failure : outcome.value->failures) {
            mLogger.error(failure.modId,
                          "callback do evento '" + name + "' falhou: " + failure.message);
        }
    }
    return outcome;
}

Result<std::uint64_t> MockRuntime::AdvanceFrames(std::uint64_t frames) {
    for (std::uint64_t i = 0; i < frames; ++i) {
        auto ticked = mTimers->Tick();
        if (!ticked.isOk()) {
            return Result<std::uint64_t>::err(ticked.code, ticked.message);
        }
        for (const TimerFailure& failure : ticked.value->failures) {
            mLogger.error(failure.modId, "timer falhou: " + failure.message);
        }
        auto dispatched = EmitEvent(
            "game.frame",
            EventPayload{{"frame", static_cast<std::int64_t>(ticked.value->frame)}});
        if (!dispatched.isOk()) {
            return Result<std::uint64_t>::err(dispatched.code, dispatched.message);
        }
    }
    return Result<std::uint64_t>::ok(mTimers->CurrentFrame());
}

std::uint64_t MockRuntime::CurrentFrame() const {
    return mTimers->CurrentFrame();
}

std::size_t MockRuntime::PressKey(const std::string& key) {
    const std::vector<HotkeyBinding> matched = mHotkeys->PressKey(key);
    for (const HotkeyBinding& binding : matched) {
        EmitEvent("input.hotkey", EventPayload{{"action", binding.id}, {"key", key}});
    }
    return matched.size();
}

bool MockRuntime::TriggerHotkey(const std::string& id) {
    if (!IsLoaded()) {
        return false;
    }
    const auto binding = mHotkeys->BindingFor(mModId, id);
    if (!binding.has_value()) {
        return false;
    }
    mHotkeys->Fire(mModId, id);
    EmitEvent("input.hotkey", EventPayload{{"action", id}, {"key", binding->defaultKey}});
    return true;
}

void MockRuntime::ResetSimulation() {
    ClearLogs();
    if (IsLoaded()) {
        (void)mStorage->Clear(mModId);
    }
}

const std::vector<MockLogEntry>& MockRuntime::Logs() const {
    return *mLogs;
}

void MockRuntime::ClearLogs() {
    mLogs->clear();
}

KeyValueStorage& MockRuntime::Storage() {
    return *mStorage;
}

const KeyValueStorage& MockRuntime::Storage() const {
    return *mStorage;
}

FrameTimerScheduler& MockRuntime::Timers() {
    return *mTimers;
}

const FrameTimerScheduler& MockRuntime::Timers() const {
    return *mTimers;
}

MockHotkeyRegistry& MockRuntime::Hotkeys() {
    return *mHotkeys;
}

const MockHotkeyRegistry& MockRuntime::Hotkeys() const {
    return *mHotkeys;
}

MockActorProvider& MockRuntime::Actors() {
    return *mActors;
}

const MockActorProvider& MockRuntime::Actors() const {
    return *mActors;
}

ModHost& MockRuntime::Host() {
    return mHost;
}

const MockHostOptions& MockRuntime::Options() const {
    return mOptions;
}

} // namespace ShipLua
