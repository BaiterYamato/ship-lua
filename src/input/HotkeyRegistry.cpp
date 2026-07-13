#include "shiplua/input/HotkeyRegistry.h"

namespace ShipLua {

NullHotkeyRegistry::NullHotkeyRegistry(Logger logger) : mLogger(std::move(logger)) {}

bool NullHotkeyRegistry::Register(const HotkeyBinding& binding, std::function<void()> onFire) {
    (void)onFire;
    mLogger.warn(binding.modId, "host sem suporte a hotkeys: registro de '" + binding.id + "' ignorado");
    return false;
}

void NullHotkeyRegistry::Fire(const std::string& modId, const std::string& id) {
    (void)modId;
    (void)id;
}

std::vector<HotkeyBinding> NullHotkeyRegistry::Registered() const {
    return {};
}

} // namespace ShipLua
