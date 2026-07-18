#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "shiplua/events/EventDispatcher.h"
#include "shiplua/input/HotkeyRegistry.h"
#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/LuaRuntime.h"
#include "shiplua/runtime/Result.h"
#include "shiplua/storage/KeyValueStorage.h"
#include "shiplua/timer/FrameTimerScheduler.h"

struct lua_State;

namespace ShipLua {

struct LuaApiHostContext {
    std::string gameId;
    std::string hostVersion;
    std::string runtimeVersion = "0.2.0";
    std::vector<std::string> capabilities;
    std::shared_ptr<HotkeyRegistry> hotkeys;  // nullable; null = host without hotkey support
    std::shared_ptr<FrameTimerScheduler> timers;  // nullable; null = host sem ship.timer
    std::shared_ptr<KeyValueStorage> storage;     // nullable; null = host sem ship.storage
};

// Validates a host context before any binding is installed: game id must be
// 'oot', 'mm' or empty, and every advertised capability must be a contracted
// capability of the schema supported by that game. Exported so hosts (real
// adapters and the mock runtime) can fail fast.
Result<void> ValidateLuaApiHostContext(const LuaApiHostContext& context);

class LuaApiBinding {
  public:
    LuaApiBinding(LuaRuntime& runtime, EventDispatcher& events, Logger logger,
                  LuaApiHostContext hostContext, std::size_t modLoadOrder,
                  int modPriority, std::size_t maxConsecutiveFailures = 3);
    ~LuaApiBinding();

    LuaApiBinding(const LuaApiBinding&) = delete;
    LuaApiBinding& operator=(const LuaApiBinding&) = delete;

    Result<void> Install();
    std::size_t SubscriptionCount() const;

  private:
    struct LuaCallback {
        int registryReference = -2;
        Subscription subscription;
        std::size_t consecutiveFailures = 0;
        bool active = true;
    };

    struct HotkeyCallback {
        int registryReference = -2;
        std::size_t consecutiveFailures = 0;
        bool active = true;
    };

    static LuaApiBinding* FromUpvalue(lua_State* state);
    static int InstallProtected(lua_State* state) noexcept;
    static int Require(lua_State* state) noexcept;
    static int GameId(lua_State* state) noexcept;
    static int HostVersion(lua_State* state) noexcept;
    static int RuntimeVersion(lua_State* state) noexcept;
    static int ApiVersion(lua_State* state) noexcept;
    static int CapabilityHas(lua_State* state) noexcept;
    static int CapabilityList(lua_State* state) noexcept;
    static int EventsOn(lua_State* state) noexcept;
    static int EventsOff(lua_State* state) noexcept;
    static int HotkeysRegister(lua_State* state) noexcept;
    static int TimerAfter(lua_State* state) noexcept;
    static int TimerEvery(lua_State* state) noexcept;
    static int TimerCancel(lua_State* state) noexcept;
    static int StorageGet(lua_State* state) noexcept;
    static int StorageSet(lua_State* state) noexcept;
    static int StorageDelete(lua_State* state) noexcept;
    static int StorageClear(lua_State* state) noexcept;
    static int LogDebug(lua_State* state) noexcept;
    static int LogInfo(lua_State* state) noexcept;
    static int LogWarn(lua_State* state) noexcept;
    static int LogError(lua_State* state) noexcept;
    static int Log(lua_State* state, LogLevel level) noexcept;

    Result<Subscription> RegisterEvent(lua_State* state, const std::string& eventName,
                                       int callbackIndex, int callbackPriority);
    Result<void> RemoveEvent(Subscription subscription);
    // lua_error usa longjmp no MSVC. Estes helpers nunca o chamam: retornam o
    // erro ao wrapper somente depois de destruir todo estado C++ não trivial.
    int RegisterEventFromLua(lua_State* state, const char*& error);
    int RemoveEventFromLua(lua_State* state, const char*& error);
    int RegisterHotkey(lua_State* state, const char*& error);
    static int ScheduleTimer(lua_State* state, bool repeating) noexcept;
    int ScheduleTimerFromLua(lua_State* state, bool repeating, const char*& error);
    Result<TimerHandle> DoScheduleTimer(lua_State* state, bool repeating, std::uint64_t frames,
                                        int callbackIndex);
    int CancelTimer(lua_State* state, const char*& error);
    int GetStorage(lua_State* state, const char*& error);
    int SetStorage(lua_State* state, const char*& error);
    int DeleteStorage(lua_State* state, const char*& error);
    int ClearStorage(lua_State* state, const char*& error);
    EventFlow InvokeCallback(const std::shared_ptr<LuaCallback>& callback,
                             EventPayload& payload);
    int WriteLog(lua_State* state, LogLevel level, const char*& error);
    void BuildModule(lua_State* state);
    void Uninstall() noexcept;

    LuaRuntime& mRuntime;
    EventDispatcher& mEvents;
    Logger mLogger;
    LuaApiHostContext mHostContext;
    std::shared_ptr<HotkeyRegistry> mHotkeys;
    std::shared_ptr<FrameTimerScheduler> mTimers;
    std::shared_ptr<KeyValueStorage> mStorage;
    std::map<std::string, std::shared_ptr<HotkeyCallback>> mHotkeyCallbacks;
    std::size_t mModLoadOrder = 0;
    int mModPriority = 50;
    std::size_t mMaxConsecutiveFailures = 3;
    std::size_t mMaxTimersPerMod = 64; // plan-sdk.md [limits] timers = 64
    std::map<std::uint64_t, std::shared_ptr<LuaCallback>> mCallbacks;
    std::map<std::uint64_t, std::shared_ptr<LuaCallback>> mTimerCallbacks;
    int mShipReference = -2;
    bool mInstalled = false;
};

} // namespace ShipLua
