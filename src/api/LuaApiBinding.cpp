#include "shiplua/api/LuaApiBinding.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

#include "shiplua/generated/ApiBindings.h"

#if __has_include(<lua.hpp>)
#include <lua.hpp>
#else
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#endif

namespace ShipLua {

namespace {

constexpr int kNoReference = LUA_NOREF;

int TracebackHandler(lua_State* state) {
    const char* message = lua_tostring(state, 1);
    if (message == nullptr) {
        message = "erro Lua sem mensagem";
    }
    luaL_traceback(state, state, message, 1);
    return 1;
}

int Fail(lua_State* state, const char* message) {
    lua_pushstring(state, message);
    return lua_error(state);
}

std::string TopToString(lua_State* state) {
    size_t length = 0;
    const char* message = lua_tolstring(state, -1, &length);
    return message != nullptr ? std::string(message, length)
                              : "erro Lua sem mensagem textual";
}

void SetFunction(lua_State* state, int tableIndex, const char* name, lua_CFunction function,
                 LuaApiBinding* binding) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_pushlightuserdata(state, binding);
    lua_pushcclosure(state, function, 1);
    lua_setfield(state, tableIndex, name);
}

void PushEventValue(lua_State* state, const EventValue& value);

void PushEventObject(lua_State* state, const EventValue::Object& object) {
    lua_createtable(state, 0, static_cast<int>(object.size()));
    for (const auto& [name, item] : object) {
        PushEventValue(state, item);
        lua_setfield(state, -2, name.c_str());
    }
}

void PushEventValue(lua_State* state, const EventValue& value) {
    std::visit(
        [state](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                lua_pushnil(state);
            } else if constexpr (std::is_same_v<T, bool>) {
                lua_pushboolean(state, item);
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                lua_pushinteger(state, static_cast<lua_Integer>(item));
            } else if constexpr (std::is_same_v<T, double>) {
                lua_pushnumber(state, static_cast<lua_Number>(item));
            } else if constexpr (std::is_same_v<T, std::string>) {
                lua_pushlstring(state, item.data(), item.size());
            } else if constexpr (std::is_same_v<T, EventValue::Array>) {
                lua_createtable(state, static_cast<int>(item.size()), 0);
                lua_Integer index = 1;
                for (const EventValue& child : item) {
                    PushEventValue(state, child);
                    lua_seti(state, -2, index++);
                }
            } else {
                PushEventObject(state, item);
            }
        },
        value.value);
}

void PushEventPayload(lua_State* state, const EventPayload& payload) {
    PushEventObject(state, payload);
}

const char* ErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::InvalidArgument: return "argumento inválido para a API ship";
        case ErrorCode::InvalidHandle: return "handle inválido para a API ship";
        case ErrorCode::Unsupported: return "recurso não suportado pela API ship";
        case ErrorCode::InvalidState: return "estado inválido da API ship";
        case ErrorCode::PermissionDenied: return "permissão negada pela API ship";
        case ErrorCode::ResourceLimit: return "limite de recurso da API ship excedido";
        case ErrorCode::HostFailure: return "falha do host na API ship";
        case ErrorCode::ScriptFailure: return "falha de script na API ship";
        case ErrorCode::Ok: return "";
    }
    return "falha desconhecida da API ship";
}

bool IsValidHotkeyId(std::string_view id) {
    if (id.empty() || id.size() > 64 || id.front() < 'a' || id.front() > 'z') {
        return false;
    }
    return std::all_of(id.begin() + 1, id.end(), [](unsigned char character) {
        return (character >= 'a' && character <= 'z') ||
               (character >= '0' && character <= '9') ||
               character == '_' || character == '.' || character == '-';
    });
}

Result<void> ValidateHostContext(const LuaApiHostContext& context) {
    if (!context.gameId.empty() && context.gameId != "oot" && context.gameId != "mm") {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "host game id must be 'oot', 'mm', or empty");
    }
    for (const std::string& requested : context.capabilities) {
        const auto found = std::find_if(
            Generated::kCapabilities.begin(), Generated::kCapabilities.end(),
            [&](const Generated::CapabilityBinding& capability) {
                return capability.name == requested;
            });
        if (found == Generated::kCapabilities.end() || found->status != "contract") {
            return Result<void>::err(ErrorCode::Unsupported,
                                     "host advertised an unknown or unavailable capability");
        }
        const bool supported = context.gameId == "oot" ? found->supportsOot
                               : context.gameId == "mm" ? found->supportsMm
                                                        : false;
        if (!supported) {
            return Result<void>::err(ErrorCode::Unsupported,
                                     "host advertised a capability incompatible with its game");
        }
    }
    return Result<void>::ok();
}

} // namespace

LuaApiBinding::LuaApiBinding(LuaRuntime& runtime, EventDispatcher& events, Logger logger,
                             LuaApiHostContext hostContext, std::size_t modLoadOrder,
                             int modPriority, std::size_t maxConsecutiveFailures)
    : mRuntime(runtime),
      mEvents(events),
      mLogger(std::move(logger)),
      mHostContext(std::move(hostContext)),
      mHotkeys(mHostContext.hotkeys),
      mModLoadOrder(modLoadOrder),
      mModPriority(modPriority),
      mMaxConsecutiveFailures(std::max<std::size_t>(1, maxConsecutiveFailures)) {
    std::sort(mHostContext.capabilities.begin(), mHostContext.capabilities.end());
    mHostContext.capabilities.erase(
        std::unique(mHostContext.capabilities.begin(), mHostContext.capabilities.end()),
        mHostContext.capabilities.end());
}

LuaApiBinding::~LuaApiBinding() {
    Uninstall();
}

LuaApiBinding* LuaApiBinding::FromUpvalue(lua_State* state) {
    return static_cast<LuaApiBinding*>(lua_touserdata(state, lua_upvalueindex(1)));
}

Result<void> LuaApiBinding::Install() {
    lua_State* state = mRuntime.State();
    if (state == nullptr) {
        return Result<void>::err(ErrorCode::InvalidState, "runtime has no lua_State");
    }
    if (mInstalled) {
        return Result<void>::err(ErrorCode::InvalidState, "ship API is already installed");
    }
    const auto validContext = ValidateHostContext(mHostContext);
    if (!validContext.isOk()) {
        return validContext;
    }

    lua_pushlightuserdata(state, this);
    lua_pushcclosure(state, &LuaApiBinding::InstallProtected, 1);
    const int status = lua_pcall(state, 0, 0, 0);
    if (status != LUA_OK) {
        const std::string message = TopToString(state);
        lua_pop(state, 1);
        if (mShipReference != kNoReference) {
            luaL_unref(state, LUA_REGISTRYINDEX, mShipReference);
            mShipReference = kNoReference;
        }
        const ErrorCode code = status == LUA_ERRMEM ? ErrorCode::ResourceLimit
                                                    : ErrorCode::HostFailure;
        return Result<void>::err(code,
                                 "failed to install ship API: " + message);
    }
    mInstalled = true;
    return Result<void>::ok();
}

int LuaApiBinding::InstallProtected(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr) {
        return Fail(state, "contexto da API ship indisponível");
    }
    bool failed = false;
    try {
        binding->BuildModule(state);
    } catch (...) {
        failed = true;
    }
    return failed ? Fail(state, "exceção C++ durante instalação da API ship") : 0;
}

void LuaApiBinding::BuildModule(lua_State* state) {
    lua_newtable(state);
    const int ship = lua_gettop(state);

    lua_newtable(state);
    SetFunction(state, -1, "id", &LuaApiBinding::GameId, this);
    SetFunction(state, -1, "host_version", &LuaApiBinding::HostVersion, this);
    lua_setfield(state, ship, "game");

    lua_newtable(state);
    SetFunction(state, -1, "version", &LuaApiBinding::RuntimeVersion, this);
    lua_setfield(state, ship, "runtime");

    lua_newtable(state);
    SetFunction(state, -1, "version", &LuaApiBinding::ApiVersion, this);
    lua_setfield(state, ship, "api");

    lua_newtable(state);
    SetFunction(state, -1, "has", &LuaApiBinding::CapabilityHas, this);
    SetFunction(state, -1, "list", &LuaApiBinding::CapabilityList, this);
    lua_setfield(state, ship, "capabilities");

    lua_newtable(state);
    SetFunction(state, -1, "on", &LuaApiBinding::EventsOn, this);
    SetFunction(state, -1, "off", &LuaApiBinding::EventsOff, this);
    lua_setfield(state, ship, "events");

    lua_newtable(state);
    SetFunction(state, -1, "debug", &LuaApiBinding::LogDebug, this);
    SetFunction(state, -1, "info", &LuaApiBinding::LogInfo, this);
    SetFunction(state, -1, "warn", &LuaApiBinding::LogWarn, this);
    SetFunction(state, -1, "error", &LuaApiBinding::LogError, this);
    lua_setfield(state, ship, "log");

    lua_newtable(state);
    SetFunction(state, -1, "register", &LuaApiBinding::HotkeysRegister, this);
    lua_setfield(state, ship, "hotkeys");

    lua_pushvalue(state, ship);
    mShipReference = luaL_ref(state, LUA_REGISTRYINDEX);
    lua_pop(state, 1);

    lua_pushlightuserdata(state, this);
    lua_pushcclosure(state, &LuaApiBinding::Require, 1);
    lua_setglobal(state, "require");
}

void LuaApiBinding::Uninstall() noexcept {
    lua_State* state = mRuntime.State();
    for (auto& [id, callback] : mHotkeyCallbacks) {
        (void)id;
        callback->active = false;
    }
    if (mHotkeys != nullptr) {
        try {
            mHotkeys->UnregisterMod(mRuntime.ModId());
        } catch (...) {
            // Callbacks are already inactive, so a broken host registry cannot
            // retain a callable reference to the Lua state.
        }
    }
    for (auto& [id, callback] : mCallbacks) {
        (void)id;
        callback->active = false;
        try {
            mEvents.Unsubscribe(callback->subscription);
        } catch (...) {
            // Destructors must not propagate host allocation failures.
        }
        if (state != nullptr && callback->registryReference != kNoReference) {
            luaL_unref(state, LUA_REGISTRYINDEX, callback->registryReference);
            callback->registryReference = kNoReference;
        }
    }
    mCallbacks.clear();
    if (state != nullptr) {
        for (auto& [id, callback] : mHotkeyCallbacks) {
            (void)id;
            if (callback->registryReference != kNoReference) {
                luaL_unref(state, LUA_REGISTRYINDEX, callback->registryReference);
                callback->registryReference = kNoReference;
            }
        }
        if (mShipReference != kNoReference) {
            luaL_unref(state, LUA_REGISTRYINDEX, mShipReference);
            mShipReference = kNoReference;
        }
        lua_pushnil(state);
        lua_setglobal(state, "require");
    }
    mHotkeyCallbacks.clear();
    mInstalled = false;
}

std::size_t LuaApiBinding::SubscriptionCount() const {
    return mCallbacks.size();
}

int LuaApiBinding::Require(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (lua_type(state, 1) != LUA_TSTRING) {
        return Fail(state, "somente o módulo interno 'ship' pode ser importado");
    }
    size_t length = 0;
    const char* name = lua_tolstring(state, 1, &length);
    if (binding == nullptr || name == nullptr || std::string_view(name, length) != "ship") {
        return Fail(state, "somente o módulo interno 'ship' pode ser importado");
    }
    lua_rawgeti(state, LUA_REGISTRYINDEX, binding->mShipReference);
    return 1;
}

int LuaApiBinding::GameId(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr || binding->mHostContext.gameId.empty()) {
        return Fail(state, "identificador do jogo host ainda não foi configurado");
    }
    lua_pushlstring(state, binding->mHostContext.gameId.data(), binding->mHostContext.gameId.size());
    return 1;
}

int LuaApiBinding::HostVersion(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr || binding->mHostContext.hostVersion.empty()) {
        return Fail(state, "versão do host ainda não foi configurada");
    }
    lua_pushlstring(state, binding->mHostContext.hostVersion.data(),
                    binding->mHostContext.hostVersion.size());
    return 1;
}

int LuaApiBinding::RuntimeVersion(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr || binding->mHostContext.runtimeVersion.empty()) {
        return Fail(state, "versão do runtime ainda não foi configurada");
    }
    lua_pushlstring(state, binding->mHostContext.runtimeVersion.data(),
                    binding->mHostContext.runtimeVersion.size());
    return 1;
}

int LuaApiBinding::ApiVersion(lua_State* state) noexcept {
    (void)FromUpvalue(state);
    lua_pushlstring(state, Generated::kApiVersion.data(), Generated::kApiVersion.size());
    return 1;
}

int LuaApiBinding::CapabilityHas(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (lua_type(state, 1) != LUA_TSTRING) {
        return Fail(state, "ship.capabilities.has exige um nome textual");
    }
    size_t length = 0;
    const char* name = lua_tolstring(state, 1, &length);
    if (binding == nullptr || name == nullptr) {
        return Fail(state, "ship.capabilities.has exige um nome textual");
    }
    const std::string_view requested(name, length);
    const bool present = std::binary_search(binding->mHostContext.capabilities.begin(),
                                            binding->mHostContext.capabilities.end(), requested);
    lua_pushboolean(state, present);
    return 1;
}

int LuaApiBinding::CapabilityList(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr) {
        return Fail(state, "contexto da API ship indisponível");
    }
    lua_createtable(state, static_cast<int>(binding->mHostContext.capabilities.size()), 0);
    lua_Integer index = 1;
    for (const std::string& capability : binding->mHostContext.capabilities) {
        lua_pushlstring(state, capability.data(), capability.size());
        lua_seti(state, -2, index++);
    }
    return 1;
}

Result<Subscription> LuaApiBinding::RegisterEvent(lua_State* state,
                                                  const std::string& eventName,
                                                  int callbackIndex,
                                                  int callbackPriority) {
    callbackIndex = lua_absindex(state, callbackIndex);
    lua_pushvalue(state, callbackIndex);
    const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
    std::shared_ptr<LuaCallback> callback;
    try {
        callback = std::make_shared<LuaCallback>();
    } catch (...) {
        luaL_unref(state, LUA_REGISTRYINDEX, reference);
        throw;
    }
    callback->registryReference = reference;
    auto subscription = mEvents.Subscribe(
        eventName, mRuntime.ModId(),
        {mModLoadOrder, mModPriority, callbackPriority},
        [this, callback](EventPayload& payload) { return InvokeCallback(callback, payload); });
    if (!subscription.isOk()) {
        luaL_unref(state, LUA_REGISTRYINDEX, reference);
        return subscription;
    }
    callback->subscription = *subscription.value;
    try {
        const auto inserted = mCallbacks.emplace(callback->subscription.id, callback);
        if (!inserted.second) {
            mEvents.Unsubscribe(callback->subscription);
            luaL_unref(state, LUA_REGISTRYINDEX, reference);
            return Result<Subscription>::err(ErrorCode::InvalidState,
                                              "duplicate subscription handle");
        }
    } catch (...) {
        mEvents.Unsubscribe(callback->subscription);
        luaL_unref(state, LUA_REGISTRYINDEX, reference);
        throw;
    }
    return subscription;
}

Result<void> LuaApiBinding::RemoveEvent(Subscription subscription) {
    const auto found = mCallbacks.find(subscription.id);
    if (found == mCallbacks.end() || !found->second->active) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "subscription does not belong to this mod");
    }
    const auto removed = mEvents.Unsubscribe(subscription);
    if (!removed.isOk()) {
        return removed;
    }
    const auto callback = found->second;
    callback->active = false;
    if (lua_State* state = mRuntime.State(); state != nullptr &&
        callback->registryReference != kNoReference) {
        luaL_unref(state, LUA_REGISTRYINDEX, callback->registryReference);
        callback->registryReference = kNoReference;
    }
    mCallbacks.erase(found);
    return Result<void>::ok();
}

int LuaApiBinding::EventsOn(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (lua_type(state, 1) != LUA_TSTRING) {
        return Fail(state, "ship.events.on exige um nome de evento textual");
    }
    size_t length = 0;
    const char* name = lua_tolstring(state, 1, &length);
    if (binding == nullptr || name == nullptr) {
        return Fail(state, "ship.events.on exige um nome de evento textual");
    }
    int callbackIndex = 0;
    int priority = 0;
    if (lua_isfunction(state, 2)) {
        callbackIndex = 2;
    } else if (lua_istable(state, 2) && lua_isfunction(state, 3)) {
        callbackIndex = 3;
        lua_getfield(state, 2, "priority");
        if (!lua_isnil(state, -1)) {
            if (!lua_isinteger(state, -1)) {
                lua_pop(state, 1);
                return Fail(state, "priority deve ser um inteiro");
            }
            const lua_Integer rawPriority = lua_tointeger(state, -1);
            if (rawPriority < std::numeric_limits<int>::min() ||
                rawPriority > std::numeric_limits<int>::max()) {
                lua_pop(state, 1);
                return Fail(state, "priority está fora do intervalo inteiro suportado");
            }
            priority = static_cast<int>(rawPriority);
        }
        lua_pop(state, 1);
    } else {
        return Fail(state, "ship.events.on exige callback ou opções e callback");
    }

    ErrorCode error = ErrorCode::Ok;
    Subscription subscription;
    try {
        auto result = binding->RegisterEvent(state, std::string(name, length), callbackIndex, priority);
        if (result.isOk()) {
            subscription = *result.value;
        } else {
            error = result.code;
        }
    } catch (...) {
        error = ErrorCode::HostFailure;
    }
    if (error != ErrorCode::Ok) {
        return Fail(state, ErrorMessage(error));
    }
    lua_pushinteger(state, static_cast<lua_Integer>(subscription.id));
    return 1;
}

int LuaApiBinding::EventsOff(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr || !lua_isinteger(state, 1)) {
        return Fail(state, "ship.events.off exige uma inscrição inteira");
    }
    const lua_Integer raw = lua_tointeger(state, 1);
    if (raw <= 0) {
        return Fail(state, "inscrição inválida");
    }
    ErrorCode error = ErrorCode::Ok;
    try {
        const auto result = binding->RemoveEvent({static_cast<std::uint64_t>(raw)});
        error = result.code;
    } catch (...) {
        error = ErrorCode::HostFailure;
    }
    if (error != ErrorCode::Ok) {
        return Fail(state, ErrorMessage(error));
    }
    lua_pushboolean(state, 1);
    return 1;
}

int LuaApiBinding::HotkeysRegister(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr) {
        return Fail(state, "contexto da API ship indisponível");
    }
    const char* error = nullptr;
    int result = 0;
    try {
        result = binding->RegisterHotkey(state, error);
    } catch (...) {
        error = "exceção C++ durante registro de hotkey";
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::RegisterHotkey(lua_State* state, const char*& error) {
    LuaApiBinding* binding = this;
    if (mHotkeys == nullptr) {
        error = ErrorMessage(ErrorCode::Unsupported);
        return 0;
    }
    if (lua_type(state, 1) != LUA_TSTRING) {
        error = "ship.hotkeys.register exige um id textual";
        return 0;
    }
    std::size_t idLen = 0;
    const char* idRaw = lua_tolstring(state, 1, &idLen);
    if (idRaw == nullptr || idLen == 0) {
        error = "ship.hotkeys.register exige um id não vazio";
        return 0;
    }
    const std::string id(idRaw, idLen);
    if (!IsValidHotkeyId(id)) {
        error = "id de hotkey deve corresponder a [a-z][a-z0-9_.-]{0,63}";
        return 0;
    }

    // Optional options table at arg 2 ({default=, label=}), callback at arg 3.
    // Or callback directly at arg 2.
    int callbackIndex = 0;
    std::string defaultKey;
    std::string label;
    if (lua_istable(state, 2) && lua_isfunction(state, 3)) {
        callbackIndex = 3;
        lua_getfield(state, 2, "default");
        if (!lua_isnil(state, -1) && lua_type(state, -1) != LUA_TSTRING) {
            lua_pop(state, 1);
            error = "default de hotkey deve ser textual";
            return 0;
        }
        if (lua_type(state, -1) == LUA_TSTRING) {
            std::size_t n = 0;
            const char* s = lua_tolstring(state, -1, &n);
            if (s != nullptr) {
                defaultKey.assign(s, n);
            }
        }
        lua_pop(state, 1);
        lua_getfield(state, 2, "label");
        if (!lua_isnil(state, -1) && lua_type(state, -1) != LUA_TSTRING) {
            lua_pop(state, 1);
            error = "label de hotkey deve ser textual";
            return 0;
        }
        if (lua_type(state, -1) == LUA_TSTRING) {
            std::size_t n = 0;
            const char* s = lua_tolstring(state, -1, &n);
            if (s != nullptr) {
                label.assign(s, n);
            }
        }
        lua_pop(state, 1);
    } else if (lua_isfunction(state, 2)) {
        callbackIndex = 2;
    } else {
        error = "ship.hotkeys.register exige callback ou opções e callback";
        return 0;
    }
    if (defaultKey.size() > 32) {
        error = "default de hotkey excede 32 bytes";
        return 0;
    }
    if (label.size() > 128) {
        error = "label de hotkey excede 128 bytes";
        return 0;
    }

    auto callback = std::make_shared<HotkeyCallback>();

    // Store the Lua callback as a registry ref (mirrors RegisterEvent).
    lua_pushvalue(state, callbackIndex);
    const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
    if (reference == LUA_NOREF) {
        error = "não foi possível reter o callback de hotkey";
        return 0;
    }

    callback->registryReference = reference;
    LuaRuntime* runtime = &binding->mRuntime;
    const std::string modId = binding->mRuntime.ModId();
    Logger logger = binding->mLogger;
    std::shared_ptr<HotkeyRegistry> registry = binding->mHotkeys;

    const std::size_t maxFailures = binding->mMaxConsecutiveFailures;
    std::function<void()> onFire = [runtime, callback, logger, modId, id, maxFailures]() {
        if (!callback->active || callback->registryReference == kNoReference) {
            return;
        }
        lua_State* s = runtime->State();
        if (s == nullptr) {
            return;
        }
        lua_pushcfunction(s, TracebackHandler);
        const int handlerIndex = lua_gettop(s);
        lua_rawgeti(s, LUA_REGISTRYINDEX, callback->registryReference);
        const int status = lua_pcall(s, 0, 0, handlerIndex);
        if (status != LUA_OK) {
            std::size_t n = 0;
            const char* msg = lua_tolstring(s, -1, &n);
            const std::string failure = msg != nullptr ? std::string(msg, n)
                                                       : "callback de hotkey falhou sem mensagem";
            lua_pop(s, 2);
            logger.error(modId, failure);
            ++callback->consecutiveFailures;
            if (callback->consecutiveFailures >= maxFailures) {
                callback->active = false;
                logger.error(modId, "hotkey '" + id + "' desativada após falhas repetidas");
            }
        } else {
            lua_pop(s, 1);
            callback->consecutiveFailures = 0;
        }
    };

    HotkeyBinding hb;
    hb.id = id;
    hb.modId = modId;
    hb.defaultKey = defaultKey;
    hb.label = label;

    bool registered = false;
    try {
        registered = registry->Register(hb, onFire);
    } catch (...) {
        registered = false;
    }
    if (registered) {
        const auto previous = binding->mHotkeyCallbacks.find(id);
        if (previous != binding->mHotkeyCallbacks.end()) {
            previous->second->active = false;
            if (previous->second->registryReference != kNoReference) {
                luaL_unref(state, LUA_REGISTRYINDEX, previous->second->registryReference);
                previous->second->registryReference = kNoReference;
            }
        }
        binding->mHotkeyCallbacks[id] = std::move(callback);
    } else {
        callback->active = false;
        callback->registryReference = kNoReference;
        luaL_unref(state, LUA_REGISTRYINDEX, reference);
    }
    lua_pushboolean(state, registered ? 1 : 0);
    return 1;
}

EventFlow LuaApiBinding::InvokeCallback(const std::shared_ptr<LuaCallback>& callback,
                                        EventPayload& payload) {
    if (!callback->active || callback->registryReference == kNoReference) {
        return EventFlow::Continue;
    }
    lua_State* state = mRuntime.State();
    if (state == nullptr) {
        throw std::runtime_error("runtime Lua indisponível durante callback");
    }

    lua_pushcfunction(state, TracebackHandler);
    const int handlerIndex = lua_gettop(state);
    lua_rawgeti(state, LUA_REGISTRYINDEX, callback->registryReference);
    PushEventPayload(state, payload);
    const int status = lua_pcall(state, 1, 0, handlerIndex);
    if (status == LUA_OK) {
        lua_pop(state, 1);
        callback->consecutiveFailures = 0;
        return EventFlow::Continue;
    }

    size_t length = 0;
    const char* message = lua_tolstring(state, -1, &length);
    std::string failure = message != nullptr ? std::string(message, length)
                                             : "callback Lua falhou sem mensagem";
    lua_pop(state, 2);
    ++callback->consecutiveFailures;
    if (callback->consecutiveFailures >= mMaxConsecutiveFailures) {
        RemoveEvent(callback->subscription);
    }
    throw std::runtime_error(failure);
}

int LuaApiBinding::WriteLog(lua_State* state, LogLevel level) noexcept {
    if (lua_type(state, 1) != LUA_TSTRING) {
        return Fail(state, "ship.log exige uma mensagem textual");
    }
    size_t length = 0;
    const char* message = lua_tolstring(state, 1, &length);
    if (message == nullptr) {
        return Fail(state, "ship.log exige uma mensagem textual");
    }
    bool failed = false;
    try {
        mLogger.Log(level, mRuntime.ModId(), std::string(message, length));
    } catch (...) {
        failed = true;
    }
    if (failed) {
        return Fail(state, "falha ao registrar mensagem do mod");
    }
    return 0;
}

int LuaApiBinding::LogDebug(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    return binding != nullptr ? binding->WriteLog(state, LogLevel::Debug)
                              : Fail(state, "contexto da API ship indisponível");
}

int LuaApiBinding::LogInfo(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    return binding != nullptr ? binding->WriteLog(state, LogLevel::Info)
                              : Fail(state, "contexto da API ship indisponível");
}

int LuaApiBinding::LogWarn(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    return binding != nullptr ? binding->WriteLog(state, LogLevel::Warn)
                              : Fail(state, "contexto da API ship indisponível");
}

int LuaApiBinding::LogError(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    return binding != nullptr ? binding->WriteLog(state, LogLevel::Error)
                              : Fail(state, "contexto da API ship indisponível");
}

} // namespace ShipLua
