#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

struct TimerHandle {
    std::uint64_t id = 0;
};

struct TimerOptions {
    std::size_t modLoadOrder = 0;
    int modPriority = 50;
};

using TimerCallback = std::function<void()>;

struct TimerFailure {
    TimerHandle timer;
    std::string modId;
    std::string message;
    bool disabled = false;
};

struct TimerTickOutcome {
    std::uint64_t frame = 0;
    std::size_t callbacksInvoked = 0;
    std::vector<TimerFailure> failures;
};

class FrameTimerScheduler {
  public:
    explicit FrameTimerScheduler(std::size_t maxConsecutiveFailures = 3);

    FrameTimerScheduler(const FrameTimerScheduler&) = delete;
    FrameTimerScheduler& operator=(const FrameTimerScheduler&) = delete;

    Result<TimerHandle> After(const std::string& modId, std::uint64_t frames,
                              TimerOptions options, TimerCallback callback);
    Result<TimerHandle> Every(const std::string& modId, std::uint64_t frames,
                              TimerOptions options, TimerCallback callback);
    Result<void> Cancel(TimerHandle timer);
    Result<std::size_t> CancelAll(const std::string& modId);
    Result<TimerTickOutcome> Tick();

    std::uint64_t CurrentFrame() const;
    std::size_t TimerCount() const;

  private:
    struct Record {
        TimerHandle handle;
        std::string modId;
        TimerOptions options;
        std::uint64_t dueFrame = 0;
        std::uint64_t interval = 0;
        TimerCallback callback;
        std::size_t consecutiveFailures = 0;
        bool active = true;
    };

    Result<TimerHandle> Schedule(const std::string& modId, std::uint64_t frames,
                                 std::uint64_t interval, TimerOptions options,
                                 TimerCallback callback);
    bool IsOwnerThread() const;
    void RemoveInactive();

    std::thread::id mOwnerThread;
    std::map<std::uint64_t, Record> mTimers;
    std::uint64_t mCurrentFrame = 0;
    std::uint64_t mNextTimerId = 1;
    std::size_t mMaxConsecutiveFailures = 3;
    bool mTicking = false;
};

} // namespace ShipLua
