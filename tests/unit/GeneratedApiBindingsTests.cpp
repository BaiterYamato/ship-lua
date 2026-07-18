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
    static_assert(kFunctions.size() == 17);
    static_assert(kEvents.size() == 11);
    static_assert(kCapabilities.size() == 16);

    Check(kApiVersion == "0.3.0", "API version should derive from the schema");
    Check(kFunctions.front().name == "ship.game.id", "first function should preserve schema order");
    Check(kFunctions.front().version == "0.1.0" && kFunctions.front().stability == "stable",
          "function version and stability should derive from the schema");
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
    Check(kFunctions[9].version == "0.2.0" && kFunctions[9].stability == "experimental",
          "MM jump should retain its version and stability contract");
    Check(kFunctions[10].name == "ship.mm.spawn_dog" && kFunctions[10].availability == "mm" &&
              kFunctions[10].capability == "mm.spawn_dog",
          "MM spawn_dog should retain its host and capability contract");
    Check(kFunctions[10].version == "0.3.0" && kFunctions[10].stability == "experimental",
          "MM spawn_dog should retain its version and stability contract");
    Check(kFunctions[11].name == "ship.oot.player.jump" && kFunctions[11].availability == "oot" &&
              kFunctions[11].capability == "oot.player.jump",
          "OOT jump should carry its host and capability contract");
    Check(kFunctions[11].version == "0.3.0" && kFunctions[11].stability == "experimental",
          "OOT jump should retain its version and stability contract");
    Check(kFunctions[12].name == "ship.oot.spawn_dog" && kFunctions[12].availability == "oot" &&
              kFunctions[12].capability == "oot.spawn_dog",
          "OOT spawn_dog should carry its host and capability contract");
    Check(kFunctions[12].version == "0.3.0" && kFunctions[12].stability == "experimental",
          "OOT spawn_dog should retain its version and stability contract");
    Check(kCapabilities[11].name == "oot.player.jump" && kCapabilities[11].supportsOot &&
              !kCapabilities[11].supportsMm,
          "OOT jump capability should be host-specific");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All generated API binding checks passed\n";
    return 0;
}
