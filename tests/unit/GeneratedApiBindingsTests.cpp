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
    static_assert(kFunctions.size() == 18);
    static_assert(kEvents.size() == 11);
    static_assert(kCapabilities.size() == 17);

    Check(kApiVersion == "0.3.0", "API version should derive from the schema");
    Check(kFunctions.front().name == "ship.game.id", "first function should preserve schema order");
    Check(kFunctions[6].arguments.size() == 3 && !kFunctions[6].arguments[2].required,
          "optional callback metadata should be generated");
    Check(kEvents[1].name == "game.frame" && kEvents[1].payload[0].name == "frame",
          "event payload metadata should be generated");
    Check(kCapabilities[6].name == "mm.room.events" && !kCapabilities[6].supportsOot &&
              kCapabilities[6].supportsMm,
          "host-specific capability support should be generated");
    Check(kFunctions[9].name == "ship.world.travel" &&
              kFunctions[9].capability == "world.travel",
          "world travel should retain its common capability contract");
    Check(kFunctions[10].name == "ship.mm.player.jump" && kFunctions[10].availability == "mm" &&
              kFunctions[10].capability == "mm.player.jump",
          "MM jump should retain its host and capability contract");
    Check(kFunctions[11].name == "ship.mm.spawn_dog" && kFunctions[11].availability == "mm" &&
              kFunctions[11].capability == "mm.spawn_dog",
          "MM spawn_dog should retain its host and capability contract");
    Check(kFunctions[12].name == "ship.oot.player.jump" && kFunctions[12].availability == "oot" &&
              kFunctions[12].capability == "oot.player.jump",
          "OOT jump should carry its host and capability contract");
    Check(kFunctions[13].name == "ship.oot.spawn_dog" && kFunctions[13].availability == "oot" &&
              kFunctions[13].capability == "oot.spawn_dog",
          "OOT spawn_dog should carry its host and capability contract");
    Check(kCapabilities[12].name == "oot.player.jump" && kCapabilities[12].supportsOot &&
              !kCapabilities[12].supportsMm,
          "OOT jump capability should be host-specific");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All generated API binding checks passed\n";
    return 0;
}
