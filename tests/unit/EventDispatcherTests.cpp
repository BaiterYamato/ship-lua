#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "shiplua/events/EventDispatcher.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::EventCallback Append(std::vector<std::string>& calls, std::string name) {
    return [&calls, name = std::move(name)](ShipLua::EventPayload&) {
        calls.push_back(name);
        return ShipLua::EventFlow::Continue;
    };
}

void TestDefinitionsAndDeterministicOrder() {
    ShipLua::EventDispatcher dispatcher;
    Check(dispatcher.DefineEvent("game.frame", ShipLua::EventKind::Observe).isOk(),
          "event definition should succeed");
    Check(!dispatcher.DefineEvent("game.frame", ShipLua::EventKind::Observe).isOk(),
          "duplicate event definition should fail");
    Check(!dispatcher.DefineEvent("", ShipLua::EventKind::Observe).isOk(),
          "empty event name should fail");

    std::vector<std::string> calls;
    dispatcher.Subscribe("game.frame", "zeta", {1, 20, 0}, Append(calls, "load-first"));
    dispatcher.Subscribe("game.frame", "beta", {2, 10, 0}, Append(calls, "priority-first"));
    dispatcher.Subscribe("game.frame", "alpha", {2, 10, 5}, Append(calls, "callback-alpha"));
    dispatcher.Subscribe("game.frame", "beta", {2, 10, 5}, Append(calls, "callback-beta-1"));
    dispatcher.Subscribe("game.frame", "beta", {2, 10, 5}, Append(calls, "callback-beta-2"));

    ShipLua::EventPayload payload;
    const auto result = dispatcher.Dispatch("game.frame", payload);
    Check(result.isOk(), "defined event should dispatch");
    Check(calls == std::vector<std::string>({"load-first", "priority-first", "callback-alpha",
                                             "callback-beta-1", "callback-beta-2"}),
          "order should use load, mod priority, callback priority, mod id and registration id");
    Check(!dispatcher.Dispatch("unknown", payload).isOk(), "unknown event should fail");
}

void TestDeferredMutationAndUnsubscribe() {
    ShipLua::EventDispatcher dispatcher;
    dispatcher.DefineEvent("game.frame", ShipLua::EventKind::Observe);
    std::vector<std::string> calls;
    ShipLua::Subscription removed;
    bool added = false;

    dispatcher.Subscribe("game.frame", "controller", {0, 0, 0},
                         [&](ShipLua::EventPayload&) {
                             calls.push_back("controller");
                             dispatcher.Unsubscribe(removed);
                             if (!added) {
                                 dispatcher.Subscribe("game.frame", "late", {0, 0, 2},
                                                      Append(calls, "late"));
                                 added = true;
                             }
                             return ShipLua::EventFlow::Continue;
                         });
    removed = *dispatcher.Subscribe("game.frame", "removed", {0, 0, 1},
                                    Append(calls, "removed")).value;

    ShipLua::EventPayload payload;
    const auto first = dispatcher.Dispatch("game.frame", payload);
    Check(first.isOk() && calls == std::vector<std::string>{"controller"},
          "unsubscribe should skip a later callback and subscribe should be deferred");
    Check(dispatcher.SubscriptionCount() == 2, "only controller and deferred callback should remain");
    calls.clear();
    const auto second = dispatcher.Dispatch("game.frame", payload);
    Check(second.isOk() && calls == std::vector<std::string>({"controller", "late"}),
          "deferred callback should run only on the next dispatch");
}

void TestKindsAndPayloadIsolation() {
    ShipLua::EventDispatcher dispatcher;
    dispatcher.DefineEvent("observe", ShipLua::EventKind::Observe);
    dispatcher.DefineEvent("filter", ShipLua::EventKind::Filter);
    dispatcher.DefineEvent("transform", ShipLua::EventKind::Transform);
    dispatcher.DefineEvent("consume", ShipLua::EventKind::Consume);

    int afterFilter = 0;
    int afterConsume = 0;
    dispatcher.Subscribe("observe", "observer", {}, [](ShipLua::EventPayload& payload) {
        payload["changed"] = true;
        return ShipLua::EventFlow::Continue;
    });
    dispatcher.Subscribe("filter", "filter", {0, 0, 0}, [](ShipLua::EventPayload&) {
        return ShipLua::EventFlow::Block;
    });
    dispatcher.Subscribe("filter", "after", {0, 0, 1}, [&](ShipLua::EventPayload&) {
        ++afterFilter;
        return ShipLua::EventFlow::Continue;
    });
    dispatcher.Subscribe("transform", "transformer", {}, [](ShipLua::EventPayload& payload) {
        payload["score"] = std::int64_t{42};
        return ShipLua::EventFlow::Continue;
    });
    dispatcher.Subscribe("consume", "consumer", {0, 0, -1}, [](ShipLua::EventPayload&) {
        return ShipLua::EventFlow::Consume;
    });
    dispatcher.Subscribe("consume", "after", {0, 0, 1}, [&](ShipLua::EventPayload&) {
        ++afterConsume;
        return ShipLua::EventFlow::Continue;
    });

    ShipLua::EventPayload payload;
    dispatcher.Dispatch("observe", payload);
    Check(!payload.contains("changed"), "observe mutations should not escape the callback snapshot");
    const auto filtered = dispatcher.Dispatch("filter", payload);
    Check(filtered.isOk() && filtered.value->blocked && afterFilter == 0,
          "filter block should stop propagation");
    dispatcher.Dispatch("transform", payload);
    Check(std::get<std::int64_t>(payload.at("score").value) == 42,
          "transform should persist payload mutations");
    const auto consumed = dispatcher.Dispatch("consume", payload);
    Check(consumed.isOk() && consumed.value->consumed && afterConsume == 0,
          "consume should stop propagation");
}

void TestFailureIsolationAndFlowValidation() {
    ShipLua::EventDispatcher dispatcher;
    dispatcher.DefineEvent("game.frame", ShipLua::EventKind::Observe);
    int healthyCalls = 0;
    dispatcher.Subscribe("game.frame", "broken", {}, [](ShipLua::EventPayload&) -> ShipLua::EventFlow {
        throw std::runtime_error("boom");
    });
    dispatcher.Subscribe("game.frame", "invalid-flow", {0, 0, 1}, [](ShipLua::EventPayload&) {
        return ShipLua::EventFlow::Block;
    });
    dispatcher.Subscribe("game.frame", "healthy", {0, 0, 2}, [&](ShipLua::EventPayload&) {
        ++healthyCalls;
        return ShipLua::EventFlow::Continue;
    });

    ShipLua::EventPayload payload;
    const auto result = dispatcher.Dispatch("game.frame", payload);
    Check(result.isOk() && healthyCalls == 1, "callback failure should not stop healthy callbacks");
    Check(result.isOk() && result.value->callbacksInvoked == 3 && result.value->failures.size() == 2,
          "exceptions and incompatible flows should be reported with the outcome");
    bool foundBroken = false;
    if (result.isOk()) {
        const auto broken = std::find_if(result.value->failures.begin(), result.value->failures.end(),
                                         [](const ShipLua::CallbackFailure& failure) {
                                             return failure.modId == "broken";
                                         });
        foundBroken = broken != result.value->failures.end() && broken->message == "boom";
    }
    Check(foundBroken,
          "failure should identify the owning mod and error");
}

void TestOwnerThreadRestriction() {
    ShipLua::EventDispatcher dispatcher;
    dispatcher.DefineEvent("game.frame", ShipLua::EventKind::Observe);
    ShipLua::ErrorCode code = ShipLua::ErrorCode::Ok;
    std::thread worker([&] {
        ShipLua::EventPayload payload;
        code = dispatcher.Dispatch("game.frame", payload).code;
    });
    worker.join();
    Check(code == ShipLua::ErrorCode::InvalidState,
          "dispatch outside the owner thread should be rejected");
}

} // namespace

int main() {
    TestDefinitionsAndDeterministicOrder();
    TestDeferredMutationAndUnsubscribe();
    TestKindsAndPayloadIsolation();
    TestFailureIsolationAndFlowValidation();
    TestOwnerThreadRestriction();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All event dispatcher checks passed\n";
    return 0;
}
