#include "shiplua/api/LuaApiBinding.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
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

void PushStorageValue(lua_State* state, const KeyValueStorage::Value& value) {
    std::visit(
        [state](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, bool>) {
                lua_pushboolean(state, item);
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                lua_pushinteger(state, static_cast<lua_Integer>(item));
            } else if constexpr (std::is_same_v<T, double>) {
                lua_pushnumber(state, static_cast<lua_Number>(item));
            } else {
                lua_pushlstring(state, item.data(), item.size());
            }
        },
        value);
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

const char* ErrorCodeName(ErrorCode code) {
    switch (code) {
        case ErrorCode::Unsupported: return "unsupported";
        case ErrorCode::InvalidArgument: return "invalid_argument";
        case ErrorCode::InvalidHandle: return "invalid_handle";
        case ErrorCode::InvalidState: return "invalid_state";
        case ErrorCode::PermissionDenied: return "permission_denied";
        case ErrorCode::ResourceLimit: return "resource_limit";
        case ErrorCode::HostFailure: return "host_failure";
        case ErrorCode::ScriptFailure: return "script_failure";
        case ErrorCode::Ok: return "ok";
    }
    return "host_failure";
}

int PushApiError(lua_State* state, ErrorCode code, const std::string& message) {
    lua_pushnil(state);
    lua_createtable(state, 0, 2);
    lua_pushstring(state, ErrorCodeName(code));
    lua_setfield(state, -2, "code");
    lua_pushlstring(state, message.data(), message.size());
    lua_setfield(state, -2, "message");
    return 2;
}

void PushActorHandle(lua_State* state, const Handle& handle) {
    lua_createtable(state, 0, 4);
    lua_pushstring(state, "actor");
    lua_setfield(state, -2, "kind");
    lua_pushinteger(state, static_cast<lua_Integer>(handle.slot));
    lua_setfield(state, -2, "slot");
    lua_pushinteger(state, static_cast<lua_Integer>(handle.generation));
    lua_setfield(state, -2, "generation");
    lua_pushinteger(state, static_cast<lua_Integer>(handle.sceneGeneration));
    lua_setfield(state, -2, "scene_generation");
}

Result<std::uint32_t> ReadHandleField(lua_State* state, int tableIndex,
                                      const char* field, bool allowZero) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_getfield(state, tableIndex, field);
    if (!lua_isinteger(state, -1)) {
        lua_pop(state, 1);
        return Result<std::uint32_t>::err(
            ErrorCode::InvalidArgument,
            std::string("actor handle field '") + field + "' must be an integer");
    }
    const lua_Integer value = lua_tointeger(state, -1);
    lua_pop(state, 1);
    if (value < (allowZero ? 0 : 1) ||
        static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max()) {
        return Result<std::uint32_t>::err(
            ErrorCode::InvalidArgument,
            std::string("actor handle field '") + field + "' is out of range");
    }
    return Result<std::uint32_t>::ok(static_cast<std::uint32_t>(value));
}

Result<Handle> ReadActorHandle(lua_State* state, int index) {
    if (lua_type(state, index) != LUA_TTABLE) {
        return Result<Handle>::err(ErrorCode::InvalidArgument,
                                   "actor handle must be a table");
    }
    index = lua_absindex(state, index);
    lua_getfield(state, index, "kind");
    size_t kindLength = 0;
    const char* kind = lua_tolstring(state, -1, &kindLength);
    const bool actorKind = kind != nullptr && std::string_view(kind, kindLength) == "actor";
    lua_pop(state, 1);
    if (!actorKind) {
        return Result<Handle>::err(ErrorCode::InvalidArgument,
                                   "actor handle kind must be 'actor'");
    }
    auto slot = ReadHandleField(state, index, "slot", true);
    auto generation = ReadHandleField(state, index, "generation", false);
    auto scene = ReadHandleField(state, index, "scene_generation", false);
    if (!slot.isOk()) {
        return Result<Handle>::err(slot.code, slot.message);
    }
    if (!generation.isOk()) {
        return Result<Handle>::err(generation.code, generation.message);
    }
    if (!scene.isOk()) {
        return Result<Handle>::err(scene.code, scene.message);
    }
    return Result<Handle>::ok(
        Handle{HandleKind::Actor, *slot.value, *generation.value, *scene.value});
}

Result<double> ReadNumberField(lua_State* state, int tableIndex, const char* field) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_getfield(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TNUMBER) {
        lua_pop(state, 1);
        return Result<double>::err(
            ErrorCode::InvalidArgument,
            std::string("actor transform field '") + field + "' must be a number");
    }
    const double value = static_cast<double>(lua_tonumber(state, -1));
    lua_pop(state, 1);
    if (!std::isfinite(value)) {
        return Result<double>::err(
            ErrorCode::InvalidArgument,
            std::string("actor transform field '") + field + "' must be finite");
    }
    return Result<double>::ok(value);
}

Result<void> ReadVector(lua_State* state, int tableIndex, const char* field,
                        bool required, double& x, double& y, double& z) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_getfield(state, tableIndex, field);
    if (lua_isnil(state, -1) && !required) {
        lua_pop(state, 1);
        return Result<void>::ok();
    }
    if (lua_type(state, -1) != LUA_TTABLE) {
        lua_pop(state, 1);
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 std::string("actor option '") + field + "' must be a table");
    }
    const int vectorIndex = lua_gettop(state);
    auto readX = ReadNumberField(state, vectorIndex, "x");
    auto readY = ReadNumberField(state, vectorIndex, "y");
    auto readZ = ReadNumberField(state, vectorIndex, "z");
    lua_pop(state, 1);
    if (!readX.isOk()) {
        return Result<void>::err(readX.code, readX.message);
    }
    if (!readY.isOk()) {
        return Result<void>::err(readY.code, readY.message);
    }
    if (!readZ.isOk()) {
        return Result<void>::err(readZ.code, readZ.message);
    }
    x = *readX.value;
    y = *readY.value;
    z = *readZ.value;
    return Result<void>::ok();
}

Result<ActorSpawnRequest> ReadActorSpawnRequest(lua_State* state) {
    if (lua_type(state, 1) != LUA_TSTRING || lua_type(state, 2) != LUA_TTABLE) {
        return Result<ActorSpawnRequest>::err(
            ErrorCode::InvalidArgument,
            "ship.actor.spawn expects actor type string and options table");
    }
    size_t actorLength = 0;
    const char* actor = lua_tolstring(state, 1, &actorLength);
    if (actor == nullptr || actorLength == 0 || actorLength > 128) {
        return Result<ActorSpawnRequest>::err(
            ErrorCode::InvalidArgument,
            "actor type must be a non-empty logical id");
    }
    ActorSpawnRequest request;
    request.actor.assign(actor, actorLength);
    auto position = ReadVector(state, 2, "position", true,
                               request.x, request.y, request.z);
    if (!position.isOk()) {
        return Result<ActorSpawnRequest>::err(position.code, position.message);
    }
    auto rotation = ReadVector(state, 2, "rotation", false,
                               request.rotationX, request.rotationY, request.rotationZ);
    if (!rotation.isOk()) {
        return Result<ActorSpawnRequest>::err(rotation.code, rotation.message);
    }
    return Result<ActorSpawnRequest>::ok(std::move(request));
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

} // namespace

Result<void> ValidateLuaApiHostContext(const LuaApiHostContext& context) {
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
        if (requested == "world.travel" && !context.worldTravel) {
            return Result<void>::err(ErrorCode::InvalidState,
                                     "host advertised world.travel without a travel handler");
        }
        if ((requested == "actor.spawn" || requested == "actor.destroy" ||
             requested == "actor.exists") && context.actors == nullptr) {
            return Result<void>::err(
                ErrorCode::InvalidState,
                "host advertised actor capability without an actor provider");
        }
    }
    if (context.capabilityRegistry != nullptr && context.actors == nullptr &&
        (context.capabilityRegistry->Has("actor.spawn", context.gameId) ||
         context.capabilityRegistry->Has("actor.destroy", context.gameId) ||
         context.capabilityRegistry->Has("actor.exists", context.gameId))) {
        return Result<void>::err(
            ErrorCode::InvalidState,
            "host registered actor capabilities without an actor provider");
    }
    return Result<void>::ok();
}

namespace {

std::string FormatVersion(const SemVersion& version) {
    std::string text = std::to_string(version.major) + "." + std::to_string(version.minor) +
                       "." + std::to_string(version.patch);
    if (!version.prerelease.empty()) {
        text += '-';
        for (std::size_t i = 0; i < version.prerelease.size(); ++i) {
            text += version.prerelease[i];
            if (i + 1 < version.prerelease.size()) {
                text += '.';
            }
        }
    }
    if (!version.build.empty()) {
        text += '+';
        for (std::size_t i = 0; i < version.build.size(); ++i) {
            text += version.build[i];
            if (i + 1 < version.build.size()) {
                text += '.';
            }
        }
    }
    return text;
}

void PushStringArray(lua_State* state, const std::vector<std::string>& values) {
    lua_createtable(state, static_cast<int>(values.size()), 0);
    lua_Integer index = 1;
    for (const std::string& value : values) {
        lua_pushlstring(state, value.data(), value.size());
        lua_seti(state, -2, index++);
    }
}

void SetTableString(lua_State* state, const char* name, const std::string& value) {
    lua_pushlstring(state, value.data(), value.size());
    lua_setfield(state, -2, name);
}

void PushCapabilityDescriptor(lua_State* state, const CapabilityDescriptor& descriptor) {
    lua_createtable(state, 0, 10);
    SetTableString(state, "id", descriptor.id);
    SetTableString(state, "version", FormatVersion(descriptor.version));
    SetTableString(state, "provider", descriptor.provider);
    SetTableString(state, "provider_version", FormatVersion(descriptor.providerVersion));
    PushStringArray(state, descriptor.games);
    lua_setfield(state, -2, "games");
    const std::string_view stability = CapabilityStabilityName(descriptor.stability);
    lua_pushlstring(state, stability.data(), stability.size());
    lua_setfield(state, -2, "stability");
    PushStringArray(state, descriptor.permissions);
    lua_setfield(state, -2, "permissions");
    lua_createtable(state, 0, 1);
    if (descriptor.limits.perMod.has_value()) {
        lua_pushinteger(state, static_cast<lua_Integer>(*descriptor.limits.perMod));
        lua_setfield(state, -2, "per_mod");
    }
    lua_setfield(state, -2, "limits");
    SetTableString(state, "description", descriptor.description);
}

// Hosts legados anunciam apenas uma lista plana de nomes; os descritores são
// sintetizados do catálogo gerado (API-003) para que has/list/info/providers
// tenham um único caminho de consulta (RFC 0008).
std::shared_ptr<CapabilityRegistry> SynthesizeLegacyRegistry(const LuaApiHostContext& context) {
    auto registry = std::make_shared<CapabilityRegistry>();
    const auto capabilityVersion = SemVersion::Parse(std::string(Generated::kApiVersion));
    const auto providerVersion = SemVersion::Parse(context.hostVersion);
    if (!capabilityVersion.isOk()) {
        return registry;
    }
    for (const std::string& name : context.capabilities) {
        const auto found = std::find_if(
            Generated::kCapabilities.begin(), Generated::kCapabilities.end(),
            [&](const Generated::CapabilityBinding& capability) {
                return capability.name == name;
            });
        if (found == Generated::kCapabilities.end()) {
            continue; // ValidateHostContext rejeita nomes desconhecidos no Install
        }
        CapabilityProvider offer;
        offer.name = "legacy-host";
        offer.providerVersion = providerVersion.isOk() ? *providerVersion.value : SemVersion{};
        offer.capabilityVersion = *capabilityVersion.value;
        if (found->supportsOot) {
            offer.games.emplace_back("oot");
        }
        if (found->supportsMm) {
            offer.games.emplace_back("mm");
        }
        offer.stability = found->status == "contract"   ? CapabilityStability::Stable
                          : found->status == "deprecated" ? CapabilityStability::Deprecated
                                                          : CapabilityStability::Internal;
        if (name == "actor.spawn") {
            offer.permissions = {"world.entities.create"};
            offer.limits.perMod = 256;
        } else if (name == "actor.destroy") {
            offer.permissions = {"world.entities.destroy"};
        } else if (name == "actor.exists") {
            offer.permissions = {"world.entities.read"};
        }
        (void)registry->Register(name, std::move(offer));
    }
    return registry;
}

} // namespace

LuaApiBinding::LuaApiBinding(LuaRuntime& runtime, EventDispatcher& events, Logger logger,
                             LuaApiHostContext hostContext, std::size_t modLoadOrder,
                             int modPriority, std::size_t maxConsecutiveFailures,
                             LuaApiModPolicy modPolicy)
    : mRuntime(runtime),
      mEvents(events),
      mLogger(std::move(logger)),
      mHostContext(std::move(hostContext)),
      mHotkeys(mHostContext.hotkeys),
      mTimers(mHostContext.timers),
      mStorage(mHostContext.storage),
      mActors(mHostContext.actors),
      mModPolicy(std::move(modPolicy)),
      mModLoadOrder(modLoadOrder),
      mModPriority(modPriority),
      mMaxConsecutiveFailures(std::max<std::size_t>(1, maxConsecutiveFailures)) {
    std::sort(mHostContext.capabilities.begin(), mHostContext.capabilities.end());
    mHostContext.capabilities.erase(
        std::unique(mHostContext.capabilities.begin(), mHostContext.capabilities.end()),
        mHostContext.capabilities.end());
    mCapabilities = mHostContext.capabilityRegistry != nullptr
                        ? mHostContext.capabilityRegistry
                        : SynthesizeLegacyRegistry(mHostContext);
    std::sort(mModPolicy.permissions.begin(), mModPolicy.permissions.end());
    mModPolicy.permissions.erase(
        std::unique(mModPolicy.permissions.begin(), mModPolicy.permissions.end()),
        mModPolicy.permissions.end());
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
    const auto validContext = ValidateLuaApiHostContext(mHostContext);
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
    SetFunction(state, -1, "info", &LuaApiBinding::CapabilityInfo, this);
    SetFunction(state, -1, "providers", &LuaApiBinding::CapabilityProviders, this);
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

    lua_newtable(state);
    SetFunction(state, -1, "after", &LuaApiBinding::TimerAfter, this);
    SetFunction(state, -1, "every", &LuaApiBinding::TimerEvery, this);
    SetFunction(state, -1, "cancel", &LuaApiBinding::TimerCancel, this);
    lua_setfield(state, ship, "timer");

    lua_newtable(state);
    SetFunction(state, -1, "get", &LuaApiBinding::StorageGet, this);
    SetFunction(state, -1, "set", &LuaApiBinding::StorageSet, this);
    SetFunction(state, -1, "delete", &LuaApiBinding::StorageDelete, this);
    SetFunction(state, -1, "clear", &LuaApiBinding::StorageClear, this);
    lua_setfield(state, ship, "storage");

    lua_newtable(state);
    SetFunction(state, -1, "travel", &LuaApiBinding::WorldTravel, this);
    lua_setfield(state, ship, "world");

    lua_newtable(state);
    SetFunction(state, -1, "spawn", &LuaApiBinding::ActorSpawn, this);
    SetFunction(state, -1, "destroy", &LuaApiBinding::ActorDestroy, this);
    SetFunction(state, -1, "exists", &LuaApiBinding::ActorExists, this);
    lua_setfield(state, ship, "actor");

    lua_pushvalue(state, ship);
    mShipReference = luaL_ref(state, LUA_REGISTRYINDEX);
    lua_pop(state, 1);

    lua_pushlightuserdata(state, this);
    lua_pushcclosure(state, &LuaApiBinding::Require, 1);
    lua_setglobal(state, "require");
}

void LuaApiBinding::Uninstall() noexcept {
    lua_State* state = mRuntime.State();
    if (mActors != nullptr) {
        try {
            (void)mActors->ReleaseMod(mRuntime.ModId());
        } catch (...) {
            // Unload must continue even when a host cleanup hook is broken.
        }
    }
    mActorHandles.clear();
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
    for (auto& [id, callback] : mTimerCallbacks) {
        (void)id;
        callback->active = false;
    }
    if (mTimers != nullptr) {
        try {
            mTimers->CancelAll(mRuntime.ModId());
        } catch (...) {
            // O scheduler não pode reter referência chamável para o estado Lua.
        }
    }
    if (state != nullptr) {
        for (auto& [id, callback] : mTimerCallbacks) {
            (void)id;
            if (callback->registryReference != kNoReference) {
                luaL_unref(state, LUA_REGISTRYINDEX, callback->registryReference);
                callback->registryReference = kNoReference;
            }
        }
    }
    mTimerCallbacks.clear();
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

#if defined(_MSC_VER)
#define SHIPLUA_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define SHIPLUA_NOINLINE __attribute__((noinline))
#else
#define SHIPLUA_NOINLINE
#endif

int LuaApiBinding::CapabilityHas(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    const int resultCount = binding == nullptr ? 0 : binding->CapabilityHasFromLua(state, error);
    if (binding == nullptr) {
        error = "contexto da API ship indisponível";
    }
    return error == nullptr ? resultCount : Fail(state, error);
}

SHIPLUA_NOINLINE int LuaApiBinding::CapabilityHasFromLua(lua_State* state,
                                                         const char*& error) {
    try {
        if (lua_type(state, 1) != LUA_TSTRING) {
            error = "ship.capabilities.has exige um id textual";
            return 0;
        }
        size_t length = 0;
        const char* id = lua_tolstring(state, 1, &length);
        if (id == nullptr || mCapabilities == nullptr) {
            error = "contexto da API ship indisponível";
            return 0;
        }
        const bool hasRange = lua_gettop(state) >= 2 && !lua_isnil(state, 2);
        if (hasRange && lua_type(state, 2) != LUA_TSTRING) {
            error = "ship.capabilities.has exige um intervalo de versão textual";
            return 0;
        }
        size_t rangeLength = 0;
        const char* rangeText = hasRange ? lua_tolstring(state, 2, &rangeLength) : nullptr;
        if (hasRange && rangeText == nullptr) {
            error = "ship.capabilities.has exige um intervalo de versão textual";
            return 0;
        }
        bool present = false;
        if (hasRange) {
            auto range = VersionRange::Parse(std::string(rangeText, rangeLength));
            if (!range.isOk()) {
                error = "intervalo de versão inválido em ship.capabilities.has";
                return 0;
            }
            present = mCapabilities->Has(std::string(id, length), *range.value,
                                         mHostContext.gameId);
        } else {
            present = mCapabilities->Has(std::string(id, length), mHostContext.gameId);
        }
        lua_pushboolean(state, present);
        return 1;
    } catch (...) {
        error = "falha do host ao consultar capabilities";
        return 0;
    }
}

int LuaApiBinding::CapabilityList(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    const int resultCount = binding == nullptr ? 0 : binding->CapabilityListFromLua(state, error);
    if (binding == nullptr) {
        error = "contexto da API ship indisponível";
    }
    return error == nullptr ? resultCount : Fail(state, error);
}

SHIPLUA_NOINLINE int LuaApiBinding::CapabilityListFromLua(lua_State* state,
                                                          const char*& error) {
    try {
        if (mCapabilities == nullptr) {
            error = "contexto da API ship indisponível";
            return 0;
        }
        const bool hasFilter = lua_gettop(state) >= 1 && !lua_isnil(state, 1);
        if (hasFilter && !lua_istable(state, 1)) {
            error = "ship.capabilities.list exige uma tabela de filtro";
            return 0;
        }
        std::string game = mHostContext.gameId;
        std::optional<CapabilityStability> stability;
        if (hasFilter) {
            lua_getfield(state, 1, "game");
            if (!lua_isnil(state, -1)) {
                size_t gameLength = 0;
                const char* gameText = lua_type(state, -1) == LUA_TSTRING
                                           ? lua_tolstring(state, -1, &gameLength)
                                           : nullptr;
                if (gameText == nullptr ||
                    (std::string_view(gameText, gameLength) != "oot" &&
                     std::string_view(gameText, gameLength) != "mm")) {
                    lua_pop(state, 1);
                    error = "filtro game de ship.capabilities.list deve ser 'oot' ou 'mm'";
                    return 0;
                }
                game.assign(gameText, gameLength);
            }
            lua_pop(state, 1);
            lua_getfield(state, 1, "stability");
            if (!lua_isnil(state, -1)) {
                size_t stabilityLength = 0;
                const char* stabilityText = lua_type(state, -1) == LUA_TSTRING
                                                ? lua_tolstring(state, -1, &stabilityLength)
                                                : nullptr;
                if (stabilityText == nullptr) {
                    lua_pop(state, 1);
                    error = "filtro stability de ship.capabilities.list é inválido";
                    return 0;
                }
                auto parsed =
                    ParseCapabilityStability(std::string(stabilityText, stabilityLength));
                if (!parsed.isOk()) {
                    lua_pop(state, 1);
                    error = "filtro stability de ship.capabilities.list é inválido";
                    return 0;
                }
                stability = *parsed.value;
            }
            lua_pop(state, 1);
        }
        const std::vector<std::string> ids = mCapabilities->List(game, stability);
        PushStringArray(state, ids);
        return 1;
    } catch (...) {
        error = "falha do host ao listar capabilities";
        return 0;
    }
}

int LuaApiBinding::CapabilityInfo(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    const int resultCount = binding == nullptr ? 0 : binding->CapabilityInfoFromLua(state, error);
    if (binding == nullptr) {
        error = "contexto da API ship indisponível";
    }
    return error == nullptr ? resultCount : Fail(state, error);
}

SHIPLUA_NOINLINE int LuaApiBinding::CapabilityInfoFromLua(lua_State* state,
                                                          const char*& error) {
    try {
        if (lua_type(state, 1) != LUA_TSTRING) {
            error = "ship.capabilities.info exige um id textual";
            return 0;
        }
        size_t length = 0;
        const char* id = lua_tolstring(state, 1, &length);
        if (id == nullptr || mCapabilities == nullptr) {
            error = "contexto da API ship indisponível";
            return 0;
        }
        const std::optional<CapabilityDescriptor> descriptor =
            mCapabilities->Info(std::string(id, length), mHostContext.gameId);
        if (descriptor.has_value()) {
            PushCapabilityDescriptor(state, *descriptor);
        } else {
            lua_pushnil(state);
        }
        return 1;
    } catch (...) {
        error = "falha do host ao consultar capability";
        return 0;
    }
}

int LuaApiBinding::CapabilityProviders(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    const int resultCount =
        binding == nullptr ? 0 : binding->CapabilityProvidersFromLua(state, error);
    if (binding == nullptr) {
        error = "contexto da API ship indisponível";
    }
    return error == nullptr ? resultCount : Fail(state, error);
}

SHIPLUA_NOINLINE int LuaApiBinding::CapabilityProvidersFromLua(lua_State* state,
                                                               const char*& error) {
    try {
        if (lua_type(state, 1) != LUA_TSTRING) {
            error = "ship.capabilities.providers exige um id textual";
            return 0;
        }
        size_t length = 0;
        const char* id = lua_tolstring(state, 1, &length);
        if (id == nullptr || mCapabilities == nullptr) {
            error = "contexto da API ship indisponível";
            return 0;
        }
        const std::vector<std::string> providers =
            mCapabilities->Providers(std::string(id, length), mHostContext.gameId);
        PushStringArray(state, providers);
        return 1;
    } catch (...) {
        error = "falha do host ao consultar providers";
        return 0;
    }
}

#undef SHIPLUA_NOINLINE

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
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->RegisterEventFromLua(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::RegisterEventFromLua(lua_State* state, const char*& error) {
    if (lua_type(state, 1) != LUA_TSTRING) {
        error = "ship.events.on exige um nome de evento textual";
        return 0;
    }
    size_t length = 0;
    const char* name = lua_tolstring(state, 1, &length);
    if (name == nullptr) {
        error = "ship.events.on exige um nome de evento textual";
        return 0;
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
                error = "priority deve ser um inteiro";
                return 0;
            }
            const lua_Integer rawPriority = lua_tointeger(state, -1);
            if (rawPriority < std::numeric_limits<int>::min() ||
                rawPriority > std::numeric_limits<int>::max()) {
                lua_pop(state, 1);
                error = "priority está fora do intervalo inteiro suportado";
                return 0;
            }
            priority = static_cast<int>(rawPriority);
        }
        lua_pop(state, 1);
    } else {
        error = "ship.events.on exige callback ou opções e callback";
        return 0;
    }

    auto subscription = RegisterEvent(state, std::string(name, length), callbackIndex, priority);
    if (!subscription.isOk()) {
        error = ErrorMessage(subscription.code);
        return 0;
    }
    lua_pushinteger(state, static_cast<lua_Integer>(subscription.value->id));
    return 1;
}

int LuaApiBinding::EventsOff(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->RemoveEventFromLua(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::RemoveEventFromLua(lua_State* state, const char*& error) {
    if (!lua_isinteger(state, 1)) {
        error = "ship.events.off exige uma inscrição inteira";
        return 0;
    }
    const lua_Integer raw = lua_tointeger(state, 1);
    if (raw <= 0) {
        error = "inscrição inválida";
        return 0;
    }
    const auto removed = RemoveEvent({static_cast<std::uint64_t>(raw)});
    if (!removed.isOk()) {
        error = ErrorMessage(removed.code);
        return 0;
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

// ---------------------------------------------------------------------------
// ship.timer — frames inteiros (plan-sdk.md §8.1); o host injeta o scheduler
// ---------------------------------------------------------------------------

int LuaApiBinding::TimerAfter(lua_State* state) noexcept {
    return ScheduleTimer(state, false);
}

int LuaApiBinding::TimerEvery(lua_State* state) noexcept {
    return ScheduleTimer(state, true);
}

int LuaApiBinding::ScheduleTimer(lua_State* state, bool repeating) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->ScheduleTimerFromLua(state, repeating, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::ScheduleTimerFromLua(lua_State* state, bool repeating, const char*& error) {
    if (!lua_isinteger(state, 1)) {
        error = repeating ? "ship.timer.every exige um intervalo em frames inteiro"
                          : "ship.timer.after exige um atraso em frames inteiro";
        return 0;
    }
    const lua_Integer frames = lua_tointeger(state, 1);
    if (frames < 1) {
        error = "ship.timer exige um atraso de pelo menos 1 frame";
        return 0;
    }
    if (!lua_isfunction(state, 2)) {
        error = repeating ? "ship.timer.every exige um callback"
                          : "ship.timer.after exige um callback";
        return 0;
    }
    auto scheduled =
        DoScheduleTimer(state, repeating, static_cast<std::uint64_t>(frames), 2);
    if (!scheduled.isOk()) {
        error = ErrorMessage(scheduled.code);
        return 0;
    }
    lua_pushinteger(state, static_cast<lua_Integer>(scheduled.value->id));
    return 1;
}

Result<TimerHandle> LuaApiBinding::DoScheduleTimer(lua_State* state, bool repeating,
                                                   std::uint64_t frames, int callbackIndex) {
    if (mTimers == nullptr) {
        return Result<TimerHandle>::err(ErrorCode::Unsupported,
                                        ErrorMessage(ErrorCode::Unsupported));
    }
    if (mTimerCallbacks.size() >= mMaxTimersPerMod) {
        return Result<TimerHandle>::err(ErrorCode::ResourceLimit,
                                        ErrorMessage(ErrorCode::ResourceLimit));
    }
    callbackIndex = lua_absindex(state, callbackIndex);
    lua_pushvalue(state, callbackIndex);
    const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
    if (reference == LUA_NOREF) {
        return Result<TimerHandle>::err(ErrorCode::HostFailure,
                                        "não foi possível reter o callback do timer");
    }

    auto callback = std::make_shared<LuaCallback>();
    callback->registryReference = reference;
    auto handleHolder = std::make_shared<std::uint64_t>(0);

    LuaRuntime* runtime = &mRuntime;
    const std::string modId = mRuntime.ModId();
    Logger logger = mLogger;
    TimerCallback onFire = [this, runtime, callback, handleHolder, logger, modId,
                            repeating]() {
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
        std::string failure;
        if (status != LUA_OK) {
            size_t length = 0;
            const char* message = lua_tolstring(s, -1, &length);
            failure = message != nullptr ? std::string(message, length)
                                         : "callback de timer falhou sem mensagem";
            lua_pop(s, 2);
            logger.error(modId, failure);
        } else {
            lua_pop(s, 1);
        }
        if (!repeating) {
            // Timers one-shot disparam uma única vez: libera o callback Lua e
            // remove o registro imediatamente, sem esperar cancel/unload.
            callback->active = false;
            if (callback->registryReference != kNoReference) {
                luaL_unref(s, LUA_REGISTRYINDEX, callback->registryReference);
                callback->registryReference = kNoReference;
            }
            mTimerCallbacks.erase(*handleHolder);
        }
        if (status != LUA_OK) {
            // O scheduler contabiliza a falha e desativa após falhas repetidas.
            throw std::runtime_error(failure);
        }
    };

    Result<TimerHandle> scheduled =
        repeating ? mTimers->Every(modId, frames, {mModLoadOrder, mModPriority}, std::move(onFire))
                  : mTimers->After(modId, frames, {mModLoadOrder, mModPriority}, std::move(onFire));
    if (!scheduled.isOk()) {
        luaL_unref(state, LUA_REGISTRYINDEX, reference);
        return scheduled;
    }
    *handleHolder = scheduled.value->id;
    mTimerCallbacks.emplace(scheduled.value->id, std::move(callback));
    return scheduled;
}

int LuaApiBinding::TimerCancel(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->CancelTimer(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::CancelTimer(lua_State* state, const char*& error) {
    if (!lua_isinteger(state, 1)) {
        error = "ship.timer.cancel exige um handle inteiro";
        return 0;
    }
    const lua_Integer raw = lua_tointeger(state, 1);
    if (raw <= 0) {
        error = "handle de timer inválido";
        return 0;
    }
    const std::uint64_t id = static_cast<std::uint64_t>(raw);
    const auto found = mTimerCallbacks.find(id);
    if (found == mTimerCallbacks.end()) {
        error = ErrorMessage(ErrorCode::InvalidHandle);
        return 0;
    }
    if (mTimers != nullptr) {
        try {
            mTimers->Cancel({id});
        } catch (...) {
            // O callback já está inativo; o scheduler pode descartar o registro.
        }
    }
    found->second->active = false;
    if (found->second->registryReference != kNoReference) {
        luaL_unref(state, LUA_REGISTRYINDEX, found->second->registryReference);
        found->second->registryReference = kNoReference;
    }
    mTimerCallbacks.erase(found);
    lua_pushboolean(state, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// ship.storage — namespace automático por mod (plan-sdk.md §8.18)
// ---------------------------------------------------------------------------

int LuaApiBinding::StorageGet(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->GetStorage(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::GetStorage(lua_State* state, const char*& error) {
    if (mStorage == nullptr) {
        error = ErrorMessage(ErrorCode::Unsupported);
        return 0;
    }
    if (lua_type(state, 1) != LUA_TSTRING) {
        error = "ship.storage.get exige uma chave textual";
        return 0;
    }
    size_t length = 0;
    const char* key = lua_tolstring(state, 1, &length);
    if (key == nullptr) {
        error = "ship.storage.get exige uma chave textual";
        return 0;
    }
    auto result = mStorage->Get(mRuntime.ModId(), std::string(key, length));
    if (!result.isOk()) {
        error = ErrorMessage(result.code);
        return 0;
    }
    if (result.value->has_value()) {
        PushStorageValue(state, **result.value);
        return 1;
    }
    if (lua_gettop(state) >= 2) {
        lua_pushvalue(state, 2);
        return 1;
    }
    lua_pushnil(state);
    return 1;
}

int LuaApiBinding::StorageSet(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->SetStorage(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::SetStorage(lua_State* state, const char*& error) {
    if (mStorage == nullptr) {
        error = ErrorMessage(ErrorCode::Unsupported);
        return 0;
    }
    if (lua_type(state, 1) != LUA_TSTRING) {
        error = "ship.storage.set exige uma chave textual";
        return 0;
    }
    size_t length = 0;
    const char* key = lua_tolstring(state, 1, &length);
    if (key == nullptr) {
        error = "ship.storage.set exige uma chave textual";
        return 0;
    }
    KeyValueStorage::Value value;
    switch (lua_type(state, 2)) {
        case LUA_TBOOLEAN:
            value = lua_toboolean(state, 2) != 0;
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(state, 2)) {
                value = static_cast<std::int64_t>(lua_tointeger(state, 2));
            } else {
                value = static_cast<double>(lua_tonumber(state, 2));
            }
            break;
        case LUA_TSTRING: {
            size_t size = 0;
            const char* text = lua_tolstring(state, 2, &size);
            value = text != nullptr ? std::string(text, size) : std::string{};
            break;
        }
        default:
            error = "ship.storage.set aceita somente booleano, número ou texto";
            return 0;
    }
    const auto stored =
        mStorage->Set(mRuntime.ModId(), std::string(key, length), std::move(value));
    if (!stored.isOk()) {
        error = ErrorMessage(stored.code);
        return 0;
    }
    lua_pushboolean(state, 1);
    return 1;
}

int LuaApiBinding::StorageDelete(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->DeleteStorage(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::DeleteStorage(lua_State* state, const char*& error) {
    if (mStorage == nullptr) {
        error = ErrorMessage(ErrorCode::Unsupported);
        return 0;
    }
    if (lua_type(state, 1) != LUA_TSTRING) {
        error = "ship.storage.delete exige uma chave textual";
        return 0;
    }
    size_t length = 0;
    const char* key = lua_tolstring(state, 1, &length);
    if (key == nullptr) {
        error = "ship.storage.delete exige uma chave textual";
        return 0;
    }
    auto removed = mStorage->Delete(mRuntime.ModId(), std::string(key, length));
    if (!removed.isOk()) {
        error = ErrorMessage(removed.code);
        return 0;
    }
    lua_pushboolean(state, *removed.value ? 1 : 0);
    return 1;
}

int LuaApiBinding::StorageClear(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->ClearStorage(state, error);
        }
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::ClearStorage(lua_State* state, const char*& error) {
    if (mStorage == nullptr) {
        error = ErrorMessage(ErrorCode::Unsupported);
        return 0;
    }
    auto cleared = mStorage->Clear(mRuntime.ModId());
    if (!cleared.isOk()) {
        error = ErrorMessage(cleared.code);
        return 0;
    }
    lua_pushinteger(state, static_cast<lua_Integer>(*cleared.value));
    return 1;
}

#if defined(_MSC_VER)
#define SHIPLUA_WORLD_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define SHIPLUA_WORLD_NOINLINE __attribute__((noinline))
#else
#define SHIPLUA_WORLD_NOINLINE
#endif

int LuaApiBinding::WorldTravel(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int resultCount = 0;
    if (binding == nullptr) {
        error = "contexto da API ship indisponível";
    } else {
        resultCount = binding->WorldTravelFromLua(state, error);
    }
    return error == nullptr ? resultCount : Fail(state, error);
}

SHIPLUA_WORLD_NOINLINE int LuaApiBinding::WorldTravelFromLua(lua_State* state,
                                                             const char*& error) {
    try {
        if (lua_type(state, 1) != LUA_TSTRING || lua_type(state, 2) != LUA_TSTRING) {
            error = "ship.world.travel exige world e destination textuais";
            return 0;
        }
        if (mCapabilities == nullptr ||
            !mCapabilities->Has("world.travel", mHostContext.gameId) ||
            !mHostContext.worldTravel) {
            error = ErrorMessage(ErrorCode::Unsupported);
            return 0;
        }

        std::size_t worldLength = 0;
        std::size_t destinationLength = 0;
        const char* world = lua_tolstring(state, 1, &worldLength);
        const char* destination = lua_tolstring(state, 2, &destinationLength);
        if (world == nullptr || destination == nullptr) {
            error = "ship.world.travel exige world e destination textuais";
            return 0;
        }

        WorldDestination request;
        const std::string_view worldName(world, worldLength);
        if (worldName == "oot") {
            request.world = WorldId::Oot;
        } else if (worldName == "mm") {
            request.world = WorldId::Mm;
        } else {
            error = "world deve ser 'oot' ou 'mm'";
            return 0;
        }
        request.id.assign(destination, destinationLength);

        const Result<void> valid = ValidateWorldDestination(request);
        if (!valid.isOk()) {
            error = ErrorMessage(valid.code);
            return 0;
        }
        const Result<void> traveled = mHostContext.worldTravel(request);
        if (!traveled.isOk()) {
            error = ErrorMessage(traveled.code);
            return 0;
        }
        lua_pushboolean(state, 1);
        return 1;
    } catch (...) {
        error = ErrorMessage(ErrorCode::HostFailure);
        return 0;
    }
}

#undef SHIPLUA_WORLD_NOINLINE

bool LuaApiBinding::HasPermission(const std::string& permission) const {
    return std::binary_search(mModPolicy.permissions.begin(),
                              mModPolicy.permissions.end(), permission);
}

std::size_t LuaApiBinding::EffectiveActorLimit() const {
    std::size_t limit = mModPolicy.maxActors;
    if (mCapabilities != nullptr) {
        const auto descriptor = mCapabilities->Info("actor.spawn", mHostContext.gameId);
        if (descriptor.has_value() && descriptor->limits.perMod.has_value()) {
            limit = std::min<std::size_t>(
                limit, static_cast<std::size_t>(*descriptor->limits.perMod));
        }
    }
    return limit;
}

void LuaApiBinding::PruneDeadActors() noexcept {
    if (mActors == nullptr) {
        mActorHandles.clear();
        return;
    }
    for (auto handle = mActorHandles.begin(); handle != mActorHandles.end();) {
        bool alive = false;
        try {
            auto exists = mActors->Exists(mRuntime.ModId(), *handle);
            alive = exists.isOk() && exists.value.has_value() && *exists.value;
        } catch (...) {
            // A provider exception cannot make a stale accounting entry keep
            // consuming a manifest slot forever.
        }
        if (!alive) {
            handle = mActorHandles.erase(handle);
        } else {
            ++handle;
        }
    }
}

int LuaApiBinding::ActorSpawn(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr) {
        return PushApiError(state, ErrorCode::HostFailure,
                            "ship API context is unavailable");
    }
    try {
        return binding->ActorSpawnFromLua(state);
    } catch (...) {
        return PushApiError(state, ErrorCode::HostFailure,
                            "actor provider raised an unexpected exception");
    }
}

int LuaApiBinding::ActorSpawnFromLua(lua_State* state) {
    if (mCapabilities == nullptr ||
        !mCapabilities->Has("actor.spawn", mHostContext.gameId) ||
        mActors == nullptr) {
        return PushApiError(state, ErrorCode::Unsupported,
                            "actor.spawn is not available in this host");
    }
    if (!HasPermission("world.entities.create")) {
        return PushApiError(
            state, ErrorCode::PermissionDenied,
            "manifest permission 'world.entities.create' is required");
    }
    auto request = ReadActorSpawnRequest(state);
    if (!request.isOk()) {
        return PushApiError(state, request.code, request.message);
    }
    PruneDeadActors();
    const std::size_t limit = EffectiveActorLimit();
    if (limit == 0 || mActorHandles.size() >= limit) {
        return PushApiError(state, ErrorCode::ResourceLimit,
                            "mod actor limit of " + std::to_string(limit) + " reached");
    }
    auto spawned = mActors->Spawn(mRuntime.ModId(), *request.value);
    if (!spawned.isOk()) {
        return PushApiError(state, spawned.code, spawned.message);
    }
    mActorHandles.push_back(*spawned.value);
    PushActorHandle(state, *spawned.value);
    lua_pushnil(state);
    return 2;
}

int LuaApiBinding::ActorDestroy(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr) {
        return PushApiError(state, ErrorCode::HostFailure,
                            "ship API context is unavailable");
    }
    try {
        return binding->ActorDestroyFromLua(state);
    } catch (...) {
        return PushApiError(state, ErrorCode::HostFailure,
                            "actor provider raised an unexpected exception");
    }
}

int LuaApiBinding::ActorDestroyFromLua(lua_State* state) {
    if (mCapabilities == nullptr ||
        !mCapabilities->Has("actor.destroy", mHostContext.gameId) ||
        mActors == nullptr) {
        return PushApiError(state, ErrorCode::Unsupported,
                            "actor.destroy is not available in this host");
    }
    if (!HasPermission("world.entities.destroy")) {
        return PushApiError(
            state, ErrorCode::PermissionDenied,
            "manifest permission 'world.entities.destroy' is required");
    }
    auto handle = ReadActorHandle(state, 1);
    if (!handle.isOk()) {
        return PushApiError(state, handle.code, handle.message);
    }
    auto destroyed = mActors->Destroy(mRuntime.ModId(), *handle.value);
    if (!destroyed.isOk()) {
        return PushApiError(state, destroyed.code, destroyed.message);
    }
    mActorHandles.erase(
        std::remove(mActorHandles.begin(), mActorHandles.end(), *handle.value),
        mActorHandles.end());
    lua_pushboolean(state, 1);
    lua_pushnil(state);
    return 2;
}

int LuaApiBinding::ActorExists(lua_State* state) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    if (binding == nullptr) {
        return PushApiError(state, ErrorCode::HostFailure,
                            "ship API context is unavailable");
    }
    try {
        return binding->ActorExistsFromLua(state);
    } catch (...) {
        return PushApiError(state, ErrorCode::HostFailure,
                            "actor provider raised an unexpected exception");
    }
}

int LuaApiBinding::ActorExistsFromLua(lua_State* state) {
    if (mCapabilities == nullptr ||
        !mCapabilities->Has("actor.exists", mHostContext.gameId) ||
        mActors == nullptr) {
        return PushApiError(state, ErrorCode::Unsupported,
                            "actor.exists is not available in this host");
    }
    if (!HasPermission("world.entities.read")) {
        return PushApiError(
            state, ErrorCode::PermissionDenied,
            "manifest permission 'world.entities.read' is required");
    }
    auto handle = ReadActorHandle(state, 1);
    if (!handle.isOk()) {
        return PushApiError(state, handle.code, handle.message);
    }
    auto exists = mActors->Exists(mRuntime.ModId(), *handle.value);
    if (!exists.isOk() && exists.code == ErrorCode::InvalidHandle) {
        mActorHandles.erase(
            std::remove(mActorHandles.begin(), mActorHandles.end(), *handle.value),
            mActorHandles.end());
        lua_pushboolean(state, 0);
        lua_pushnil(state);
        return 2;
    }
    if (!exists.isOk()) {
        return PushApiError(state, exists.code, exists.message);
    }
    lua_pushboolean(state, *exists.value ? 1 : 0);
    lua_pushnil(state);
    return 2;
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

int LuaApiBinding::WriteLog(lua_State* state, LogLevel level, const char*& error) {
    if (lua_type(state, 1) != LUA_TSTRING) {
        error = "ship.log exige uma mensagem textual";
        return 0;
    }
    size_t length = 0;
    const char* message = lua_tolstring(state, 1, &length);
    if (message == nullptr) {
        error = "ship.log exige uma mensagem textual";
        return 0;
    }
    mLogger.Log(level, mRuntime.ModId(), std::string(message, length));
    return 0;
}

int LuaApiBinding::Log(lua_State* state, LogLevel level) noexcept {
    LuaApiBinding* binding = FromUpvalue(state);
    const char* error = nullptr;
    int result = 0;
    try {
        if (binding == nullptr) {
            error = "contexto da API ship indisponível";
        } else {
            result = binding->WriteLog(state, level, error);
        }
    } catch (...) {
        error = "falha ao registrar mensagem do mod";
    }
    return error != nullptr ? Fail(state, error) : result;
}

int LuaApiBinding::LogDebug(lua_State* state) noexcept {
    return Log(state, LogLevel::Debug);
}

int LuaApiBinding::LogInfo(lua_State* state) noexcept {
    return Log(state, LogLevel::Info);
}

int LuaApiBinding::LogWarn(lua_State* state) noexcept {
    return Log(state, LogLevel::Warn);
}

int LuaApiBinding::LogError(lua_State* state) noexcept {
    return Log(state, LogLevel::Error);
}

} // namespace ShipLua
