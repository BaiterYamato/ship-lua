#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "shiplua/events/EventDispatcher.h"
#include "shiplua/input/HotkeyRegistry.h"
#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/LuaRuntime.h"
#include "shiplua/runtime/Result.h"
#include "shiplua/world/WorldSession.h"

struct lua_State;

namespace ShipLua {

struct LuaApiHostContext {
    std::string gameId;
    std::string hostVersion;
    std::string runtimeVersion = "0.2.0";
    std::vector<std::string> capabilities;
    std::shared_ptr<HotkeyRegistry> hotkeys;  // nullable; null = host without hotkey support
    std::function<Result<void>(const WorldDestination&)> worldTravel;
    // Games whose assets/hosts are available to the supervising Link-Span
    // process. Empty keeps standalone compatibility and means only gameId.
    std::vector<std::string> availableGames;
};

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
    static int WorldTravel(lua_State* state) noexcept;
    static int LogDebug(lua_State* state) noexcept;
    static int LogInfo(lua_State* state) noexcept;
    static int LogWarn(lua_State* state) noexcept;
    static int LogError(lua_State* state) noexcept;

    Result<Subscription> RegisterEvent(lua_State* state, const std::string& eventName,
                                       int callbackIndex, int callbackPriority);
    Result<void> RemoveEvent(Subscription subscription);
    int RegisterHotkey(lua_State* state, const char*& error);
    EventFlow InvokeCallback(const std::shared_ptr<LuaCallback>& callback,
                             EventPayload& payload);
    int WriteLog(lua_State* state, LogLevel level) noexcept;
    void BuildModule(lua_State* state);
    void Uninstall() noexcept;

    LuaRuntime& mRuntime;
    EventDispatcher& mEvents;
    Logger mLogger;
    LuaApiHostContext mHostContext;
    std::shared_ptr<HotkeyRegistry> mHotkeys;
    std::map<std::string, std::shared_ptr<HotkeyCallback>> mHotkeyCallbacks;
    std::size_t mModLoadOrder = 0;
    int mModPriority = 50;
    std::size_t mMaxConsecutiveFailures = 3;
    std::map<std::uint64_t, std::shared_ptr<LuaCallback>> mCallbacks;
    int mShipReference = -2;
    bool mInstalled = false;
};

} // namespace ShipLua
