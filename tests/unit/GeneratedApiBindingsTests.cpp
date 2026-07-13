#include <iostream>
#include <string_view>
#include <type_traits>

#include "shiplua/generated/ApiBindings.h"

namespace {

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    using namespace ShipLua::Generated;
    static_assert(std::is_same_v<Subscription, std::uint64_t>);
    static_assert(std::is_same_v<decltype(ActorHandle::game), GameId>);
    static_assert(kFunctions.size() == 14);
    static_assert(kEvents.size() == 11);
    static_assert(kCapabilities.size() == 13);

    Check(kApiVersion == "0.2.0", "API version should derive from the schema");
    Check(kFunctions.front().name == "ship.game.id", "first function should preserve schema order");
    Check(kFunctions[6].arguments.size() == 3 && !kFunctions[6].arguments[2].required,
          "optional callback metadata should be generated");
    Check(kEvents[1].name == "game.frame" && kEvents[1].payload[0].name == "frame",
          "event payload metadata should be generated");
    Check(kCapabilities[5].name == "mm.room.events" && !kCapabilities[5].supportsOot &&
              kCapabilities[5].supportsMm,
          "host-specific capability support should be generated");
    Check(kFunctions[9].name == "ship.mm.player.jump" && kFunctions[9].availability == "mm" &&
              kFunctions[9].capability == "mm.player.jump",
          "MM jump should retain its host and capability contract");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All generated API binding checks passed\n";
    return 0;
}
