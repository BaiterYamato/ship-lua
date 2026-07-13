#pragma once

#include <functional>
#include <string>
#include <vector>

#include "shiplua/runtime/Logger.h"

namespace ShipLua {

// Metadata for a hotkey registered by a mod. The host converts `defaultKey`
// (a physical key name like "K", "F1") to its native scancode — the core never
// references game keyboard enums, so it stays cross-game.
struct HotkeyBinding {
    std::string id;          // unique within a mod, e.g. "jump"
    std::string modId;       // filled in by the core from LuaRuntime::ModId()
    std::string defaultKey;  // physical key name, e.g. "K"
    std::string label;       // human-readable label for the settings UI
};

// Injectable SPI: the core calls, the host implements. Nullable — a host that
// does not support hotkeys leaves the shared_ptr empty and ship.hotkeys.register
// becomes a graceful no-op (warn + false), so mods never fail to load.
class HotkeyRegistry {
  public:
    virtual ~HotkeyRegistry() = default;

    // Registers (or re-registers, updating the callback) a binding. Returns
    // false if the host rejected it (e.g. invalid key name). Idempotent for a
    // repeated (modId, id) pair.
    virtual bool Register(const HotkeyBinding& binding, std::function<void()> onFire) = 0;

    // Fires the registered callback. No-op if absent.
    virtual void Fire(const std::string& modId, const std::string& id) = 0;

    // Enumerates registered bindings for the settings UI. May be empty.
    virtual std::vector<HotkeyBinding> Registered() const = 0;
};

// Default no-op implementation. Hosts that do not support hotkeys inject this
// (or nullptr) so mods still load.
class NullHotkeyRegistry : public HotkeyRegistry {
  public:
    explicit NullHotkeyRegistry(Logger logger = {});
    bool Register(const HotkeyBinding& binding, std::function<void()> onFire) override;
    void Fire(const std::string& modId, const std::string& id) override;
    std::vector<HotkeyBinding> Registered() const override;

  private:
    Logger mLogger;
};

} // namespace ShipLua
