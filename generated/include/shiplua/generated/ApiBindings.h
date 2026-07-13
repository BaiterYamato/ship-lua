// Gerado por tools/generate_cpp_api.py. Não edite manualmente.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace ShipLua::Generated {

inline constexpr std::string_view kApiVersion = "0.2.0";
inline constexpr std::uint32_t kSchemaVersion = 1;

enum class GameId {
    Oot,
    Mm,
};

using Subscription = std::uint64_t;

struct EventOptions {
    std::optional<std::int64_t> priority;
};

struct ActorHandle {
    std::int64_t slot;
    std::int64_t generation;
    GameId game;
};

struct ActorSnapshot {
    ActorHandle handle;
    std::int64_t actor_id;
    std::int64_t category;
};

struct HotkeyOptions {
    std::optional<std::string> default_;
    std::optional<std::string> label;
};

enum class ApiError {
    Unsupported,
    InvalidArgument,
    InvalidHandle,
    InvalidState,
    PermissionDenied,
    ResourceLimit,
    HostFailure,
    ScriptFailure,
};

enum class EventKind { Observe, Filter, Transform, Consume };

struct FieldBinding {
    std::string_view name;
    std::string_view type;
    bool required;
};

enum class FunctionId {
    ShipGameId,
    ShipGameHostVersion,
    ShipRuntimeVersion,
    ShipApiVersion,
    ShipCapabilitiesHas,
    ShipCapabilitiesList,
    ShipEventsOn,
    ShipEventsOff,
    ShipHotkeysRegister,
    ShipMmPlayerJump,
    ShipLogDebug,
    ShipLogInfo,
    ShipLogWarn,
    ShipLogError,
};

struct FunctionBinding {
    FunctionId id;
    std::string_view name;
    std::string_view returnType;
    std::string_view availability;
    std::string_view capability;
    std::span<const FieldBinding> arguments;
    std::span<const std::string_view> errors;
};

inline constexpr std::array<FieldBinding, 0> kShipGameIdArguments{{
}};
inline constexpr std::array<std::string_view, 0> kShipGameIdErrors{{
}};
inline constexpr std::array<FieldBinding, 0> kShipGameHostVersionArguments{{
}};
inline constexpr std::array<std::string_view, 0> kShipGameHostVersionErrors{{
}};
inline constexpr std::array<FieldBinding, 0> kShipRuntimeVersionArguments{{
}};
inline constexpr std::array<std::string_view, 0> kShipRuntimeVersionErrors{{
}};
inline constexpr std::array<FieldBinding, 0> kShipApiVersionArguments{{
}};
inline constexpr std::array<std::string_view, 0> kShipApiVersionErrors{{
}};
inline constexpr std::array<FieldBinding, 1> kShipCapabilitiesHasArguments{{
    {"name", "string", true},
}};
inline constexpr std::array<std::string_view, 1> kShipCapabilitiesHasErrors{{
    "invalid_argument",
}};
inline constexpr std::array<FieldBinding, 0> kShipCapabilitiesListArguments{{
}};
inline constexpr std::array<std::string_view, 0> kShipCapabilitiesListErrors{{
}};
inline constexpr std::array<FieldBinding, 3> kShipEventsOnArguments{{
    {"event", "string", true},
    {"options_or_callback", "any", true},
    {"callback", "callback", false},
}};
inline constexpr std::array<std::string_view, 2> kShipEventsOnErrors{{
    "invalid_argument",
    "unsupported",
}};
inline constexpr std::array<FieldBinding, 1> kShipEventsOffArguments{{
    {"subscription", "subscription", true},
}};
inline constexpr std::array<std::string_view, 1> kShipEventsOffErrors{{
    "invalid_handle",
}};
inline constexpr std::array<FieldBinding, 3> kShipHotkeysRegisterArguments{{
    {"id", "string", true},
    {"options", "hotkey_options", false},
    {"callback", "callback", true},
}};
inline constexpr std::array<std::string_view, 2> kShipHotkeysRegisterErrors{{
    "invalid_argument",
    "unsupported",
}};
inline constexpr std::array<FieldBinding, 0> kShipMmPlayerJumpArguments{{
}};
inline constexpr std::array<std::string_view, 0> kShipMmPlayerJumpErrors{{
}};
inline constexpr std::array<FieldBinding, 1> kShipLogDebugArguments{{
    {"message", "string", true},
}};
inline constexpr std::array<std::string_view, 1> kShipLogDebugErrors{{
    "invalid_argument",
}};
inline constexpr std::array<FieldBinding, 1> kShipLogInfoArguments{{
    {"message", "string", true},
}};
inline constexpr std::array<std::string_view, 1> kShipLogInfoErrors{{
    "invalid_argument",
}};
inline constexpr std::array<FieldBinding, 1> kShipLogWarnArguments{{
    {"message", "string", true},
}};
inline constexpr std::array<std::string_view, 1> kShipLogWarnErrors{{
    "invalid_argument",
}};
inline constexpr std::array<FieldBinding, 1> kShipLogErrorArguments{{
    {"message", "string", true},
}};
inline constexpr std::array<std::string_view, 1> kShipLogErrorErrors{{
    "invalid_argument",
}};

inline constexpr std::array<FunctionBinding, 14> kFunctions{{
    {FunctionId::ShipGameId, "ship.game.id", "game_id", "common", {}, kShipGameIdArguments, kShipGameIdErrors},
    {FunctionId::ShipGameHostVersion, "ship.game.host_version", "string", "common", {}, kShipGameHostVersionArguments, kShipGameHostVersionErrors},
    {FunctionId::ShipRuntimeVersion, "ship.runtime.version", "string", "common", {}, kShipRuntimeVersionArguments, kShipRuntimeVersionErrors},
    {FunctionId::ShipApiVersion, "ship.api.version", "string", "common", {}, kShipApiVersionArguments, kShipApiVersionErrors},
    {FunctionId::ShipCapabilitiesHas, "ship.capabilities.has", "boolean", "common", {}, kShipCapabilitiesHasArguments, kShipCapabilitiesHasErrors},
    {FunctionId::ShipCapabilitiesList, "ship.capabilities.list", "array<string>", "common", {}, kShipCapabilitiesListArguments, kShipCapabilitiesListErrors},
    {FunctionId::ShipEventsOn, "ship.events.on", "subscription", "common", {}, kShipEventsOnArguments, kShipEventsOnErrors},
    {FunctionId::ShipEventsOff, "ship.events.off", "boolean", "common", {}, kShipEventsOffArguments, kShipEventsOffErrors},
    {FunctionId::ShipHotkeysRegister, "ship.hotkeys.register", "boolean", "common", {}, kShipHotkeysRegisterArguments, kShipHotkeysRegisterErrors},
    {FunctionId::ShipMmPlayerJump, "ship.mm.player.jump", "boolean", "mm", "mm.player.jump", kShipMmPlayerJumpArguments, kShipMmPlayerJumpErrors},
    {FunctionId::ShipLogDebug, "ship.log.debug", "nil", "common", {}, kShipLogDebugArguments, kShipLogDebugErrors},
    {FunctionId::ShipLogInfo, "ship.log.info", "nil", "common", {}, kShipLogInfoArguments, kShipLogInfoErrors},
    {FunctionId::ShipLogWarn, "ship.log.warn", "nil", "common", {}, kShipLogWarnArguments, kShipLogWarnErrors},
    {FunctionId::ShipLogError, "ship.log.error", "nil", "common", {}, kShipLogErrorArguments, kShipLogErrorErrors},
}};

struct EventBinding {
    std::string_view name;
    EventKind kind;
    std::string_view phase;
    bool cancellable;
    bool supportsOot;
    bool supportsMm;
    std::string_view capability;
    std::span<const FieldBinding> payload;
};

inline constexpr std::array<FieldBinding, 4> kGameReadyPayload{{
    {"game_id", "game_id", true},
    {"host_version", "string", true},
    {"runtime_version", "string", true},
    {"api_version", "string", true},
}};
inline constexpr std::array<FieldBinding, 1> kGameFramePayload{{
    {"frame", "integer", true},
}};
inline constexpr std::array<FieldBinding, 0> kGameShutdownPayload{{
}};
inline constexpr std::array<FieldBinding, 1> kSceneEnterPayload{{
    {"scene_id", "integer", true},
}};
inline constexpr std::array<FieldBinding, 1> kActorInitPayload{{
    {"actor", "actor_snapshot", true},
}};
inline constexpr std::array<FieldBinding, 1> kActorUpdatePayload{{
    {"actor", "actor_snapshot", true},
}};
inline constexpr std::array<FieldBinding, 1> kActorDestroyPayload{{
    {"handle", "actor_handle", true},
}};
inline constexpr std::array<FieldBinding, 1> kSaveLoadedPayload{{
    {"slot", "integer", true},
}};
inline constexpr std::array<FieldBinding, 1> kTextOpenPayload{{
    {"text_id", "integer", true},
}};
inline constexpr std::array<FieldBinding, 2> kAudioSequenceStartedPayload{{
    {"player_index", "integer", true},
    {"sequence_id", "integer", true},
}};
inline constexpr std::array<FieldBinding, 2> kInputHotkeyPayload{{
    {"action", "string", true},
    {"key", "string", true},
}};

inline constexpr std::array<EventBinding, 11> kEvents{{
    {"game.ready", EventKind::Observe, "mvp", false, true, true, {}, kGameReadyPayload},
    {"game.frame", EventKind::Observe, "mvp", false, true, true, {}, kGameFramePayload},
    {"game.shutdown", EventKind::Observe, "mvp", false, true, true, {}, kGameShutdownPayload},
    {"scene.enter", EventKind::Observe, "host_bridge", false, true, true, "scene.events", kSceneEnterPayload},
    {"actor.init", EventKind::Observe, "host_bridge", false, true, true, "actor.events", kActorInitPayload},
    {"actor.update", EventKind::Observe, "host_bridge", false, true, true, "actor.events", kActorUpdatePayload},
    {"actor.destroy", EventKind::Observe, "host_bridge", false, true, true, "actor.events", kActorDestroyPayload},
    {"save.loaded", EventKind::Observe, "host_bridge", false, true, true, "save.events", kSaveLoadedPayload},
    {"text.open", EventKind::Observe, "host_bridge", false, true, true, "text.events", kTextOpenPayload},
    {"audio.sequence_started", EventKind::Observe, "host_bridge", false, true, true, "audio.sequence.events", kAudioSequenceStartedPayload},
    {"input.hotkey", EventKind::Observe, "host_bridge", false, true, true, {}, kInputHotkeyPayload},
}};

struct CapabilityBinding {
    std::string_view name;
    std::string_view status;
    bool supportsOot;
    bool supportsMm;
};

inline constexpr std::array<CapabilityBinding, 13> kCapabilities{{
    {"scene.events", "contract", true, true},
    {"actor.events", "contract", true, true},
    {"save.events", "contract", true, true},
    {"text.events", "contract", true, true},
    {"audio.sequence.events", "contract", true, true},
    {"mm.room.events", "planned", false, true},
    {"mm.cycle", "planned", false, true},
    {"mm.owl_save", "planned", false, true},
    {"mm.clock", "planned", false, true},
    {"mm.player.jump", "contract", false, true},
    {"oot.ocarina", "planned", true, false},
    {"oot.dungeon_keys", "planned", true, false},
    {"oot.equipment", "planned", true, false},
}};

} // namespace ShipLua::Generated
