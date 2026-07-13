#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "shiplua/timer/FrameTimerScheduler.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void TestAfterAndDeterministicOrder() {
    ShipLua::FrameTimerScheduler scheduler;
    std::vector<std::string> calls;
    scheduler.After("zeta", 2, {1, 20}, [&] { calls.push_back("load-first"); });
    scheduler.After("beta", 2, {2, 10}, [&] { calls.push_back("priority-first"); });
    scheduler.After("alpha", 2, {2, 20}, [&] { calls.push_back("alpha"); });
    scheduler.After("beta", 2, {2, 20}, [&] { calls.push_back("beta-1"); });
    scheduler.After("beta", 2, {2, 20}, [&] { calls.push_back("beta-2"); });

    const auto first = scheduler.Tick();
    Check(first.isOk() && first.value->frame == 1 && calls.empty(),
          "after(2) should not run on the first frame");
    const auto second = scheduler.Tick();
    Check(second.isOk() && second.value->callbacksInvoked == 5,
          "all timers due on frame two should run");
    Check(calls == std::vector<std::string>({"load-first", "priority-first", "alpha",
                                             "beta-1", "beta-2"}),
          "timers should use deterministic load, priority, mod and registration order");
    Check(scheduler.TimerCount() == 0, "one-shot timers should be removed after execution");
}

void TestEveryAndCancellation() {
    ShipLua::FrameTimerScheduler scheduler;
    std::vector<std::uint64_t> frames;
    ShipLua::TimerHandle repeating;
    repeating = *scheduler.Every("clock", 2, {}, [&] {
                    frames.push_back(scheduler.CurrentFrame());
                    if (frames.size() == 3) {
                        scheduler.Cancel(repeating);
                    }
                }).value;

    for (int index = 0; index < 8; ++index) {
        scheduler.Tick();
    }
    Check(frames == std::vector<std::uint64_t>({2, 4, 6}),
          "every(2) should remain aligned and self-cancel safely");
    Check(scheduler.TimerCount() == 0, "self-cancelled timer should be removed after the tick");
    Check(!scheduler.Cancel(repeating).isOk(), "cancelled timer handle should become invalid");
}

void TestMutationsDuringTick() {
    ShipLua::FrameTimerScheduler scheduler;
    std::vector<std::string> calls;
    ShipLua::TimerHandle removed;
    scheduler.After("controller", 1, {0, 0}, [&] {
        calls.push_back("controller");
        scheduler.Cancel(removed);
        scheduler.After("late", 1, {}, [&] { calls.push_back("late"); });
    });
    removed = *scheduler.After("removed", 1, {0, 0}, [&] { calls.push_back("removed"); }).value;

    scheduler.Tick();
    Check(calls == std::vector<std::string>{"controller"},
          "cancel should skip a due timer and a new timer should be deferred");
    scheduler.Tick();
    Check(calls == std::vector<std::string>({"controller", "late"}),
          "timer created during tick should run on a future frame");
}

void TestFailureIsolationAndDisableThreshold() {
    ShipLua::FrameTimerScheduler scheduler(3);
    int brokenCalls = 0;
    int healthyCalls = 0;
    scheduler.Every("broken", 1, {0, 0}, [&] {
        ++brokenCalls;
        throw std::runtime_error("boom");
    });
    scheduler.Every("healthy", 1, {0, 0}, [&] { ++healthyCalls; });

    std::vector<ShipLua::TimerFailure> reported;
    for (int frame = 0; frame < 4; ++frame) {
        const auto tick = scheduler.Tick();
        if (tick.isOk()) {
            reported.insert(reported.end(), tick.value->failures.begin(), tick.value->failures.end());
        }
    }
    Check(brokenCalls == 3, "recurring timer should disable after three consecutive failures");
    Check(healthyCalls == 4, "broken timer should not stop a healthy timer");
    Check(reported.size() == 3 && reported.back().disabled &&
              reported.back().modId == "broken" && reported.back().message == "boom",
          "failure should identify the mod and report when the timer is disabled");
}

void TestCancelAllValidationAndOwnerThread() {
    ShipLua::FrameTimerScheduler scheduler;
    Check(!scheduler.After("mod", 0, {}, [] {}).isOk(), "zero-frame timer should be rejected");
    Check(!scheduler.After("", 1, {}, [] {}).isOk(), "empty mod id should be rejected");
    scheduler.After("one", 1, {}, [] {});
    scheduler.Every("one", 2, {}, [] {});
    scheduler.After("two", 1, {}, [] {});
    const auto cancelled = scheduler.CancelAll("one");
    Check(cancelled.isOk() && *cancelled.value == 2 && scheduler.TimerCount() == 1,
          "cancel all should remove only timers owned by the selected mod");

    ShipLua::ErrorCode code = ShipLua::ErrorCode::Ok;
    std::thread worker([&] { code = scheduler.Tick().code; });
    worker.join();
    Check(code == ShipLua::ErrorCode::InvalidState,
          "tick outside the owner thread should be rejected");
}

void TestTickCannotReenter() {
    ShipLua::FrameTimerScheduler scheduler;
    ShipLua::ErrorCode nestedCode = ShipLua::ErrorCode::Ok;
    scheduler.After("mod", 1, {}, [&] { nestedCode = scheduler.Tick().code; });
    const auto outer = scheduler.Tick();
    Check(outer.isOk() && nestedCode == ShipLua::ErrorCode::InvalidState,
          "nested tick should fail without invalidating the outer tick");
}

} // namespace

int main() {
    TestAfterAndDeterministicOrder();
    TestEveryAndCancellation();
    TestMutationsDuringTick();
    TestFailureIsolationAndDisableThreshold();
    TestCancelAllValidationAndOwnerThread();
    TestTickCannotReenter();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All frame timer scheduler checks passed\n";
    return 0;
}
