#include "shiplua/timer/FrameTimerScheduler.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <tuple>
#include <utility>

namespace ShipLua {

FrameTimerScheduler::FrameTimerScheduler(std::size_t maxConsecutiveFailures)
    : mOwnerThread(std::this_thread::get_id()),
      mMaxConsecutiveFailures(std::max<std::size_t>(1, maxConsecutiveFailures)) {}

bool FrameTimerScheduler::IsOwnerThread() const {
    return std::this_thread::get_id() == mOwnerThread;
}

Result<TimerHandle> FrameTimerScheduler::Schedule(const std::string& modId,
                                                  std::uint64_t frames,
                                                  std::uint64_t interval,
                                                  TimerOptions options,
                                                  TimerCallback callback) {
    if (!IsOwnerThread()) {
        return Result<TimerHandle>::err(ErrorCode::InvalidState,
                                        "timer scheduler accessed outside its owner thread");
    }
    if (modId.empty() || !callback) {
        return Result<TimerHandle>::err(ErrorCode::InvalidArgument,
                                        "timer requires a mod id and callback");
    }
    if (frames == 0) {
        return Result<TimerHandle>::err(ErrorCode::InvalidArgument,
                                        "timer frame delay must be greater than zero");
    }
    if (frames > std::numeric_limits<std::uint64_t>::max() - mCurrentFrame) {
        return Result<TimerHandle>::err(ErrorCode::ResourceLimit,
                                        "timer due frame exceeds the frame counter");
    }
    if (mNextTimerId == std::numeric_limits<std::uint64_t>::max()) {
        return Result<TimerHandle>::err(ErrorCode::ResourceLimit,
                                        "timer handle counter is exhausted");
    }

    const TimerHandle handle{mNextTimerId++};
    mTimers.emplace(handle.id, Record{handle, modId, options, mCurrentFrame + frames,
                                      interval, std::move(callback), 0, true});
    return Result<TimerHandle>::ok(handle);
}

Result<TimerHandle> FrameTimerScheduler::After(const std::string& modId,
                                               std::uint64_t frames,
                                               TimerOptions options,
                                               TimerCallback callback) {
    return Schedule(modId, frames, 0, options, std::move(callback));
}

Result<TimerHandle> FrameTimerScheduler::Every(const std::string& modId,
                                               std::uint64_t frames,
                                               TimerOptions options,
                                               TimerCallback callback) {
    return Schedule(modId, frames, frames, options, std::move(callback));
}

Result<void> FrameTimerScheduler::Cancel(TimerHandle timer) {
    if (!IsOwnerThread()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "timer scheduler accessed outside its owner thread");
    }
    const auto record = mTimers.find(timer.id);
    if (timer.id == 0 || record == mTimers.end() || !record->second.active) {
        return Result<void>::err(ErrorCode::InvalidHandle, "timer is not active");
    }
    if (mTicking) {
        record->second.active = false;
    } else {
        mTimers.erase(record);
    }
    return Result<void>::ok();
}

Result<std::size_t> FrameTimerScheduler::CancelAll(const std::string& modId) {
    if (!IsOwnerThread()) {
        return Result<std::size_t>::err(
            ErrorCode::InvalidState, "timer scheduler accessed outside its owner thread");
    }
    if (modId.empty()) {
        return Result<std::size_t>::err(ErrorCode::InvalidArgument, "mod id cannot be empty");
    }

    std::size_t cancelled = 0;
    for (auto it = mTimers.begin(); it != mTimers.end();) {
        if (it->second.active && it->second.modId == modId) {
            ++cancelled;
            if (mTicking) {
                it->second.active = false;
                ++it;
            } else {
                it = mTimers.erase(it);
            }
        } else {
            ++it;
        }
    }
    return Result<std::size_t>::ok(cancelled);
}

void FrameTimerScheduler::RemoveInactive() {
    for (auto it = mTimers.begin(); it != mTimers.end();) {
        if (!it->second.active) {
            it = mTimers.erase(it);
        } else {
            ++it;
        }
    }
}

Result<TimerTickOutcome> FrameTimerScheduler::Tick() {
    if (!IsOwnerThread()) {
        return Result<TimerTickOutcome>::err(
            ErrorCode::InvalidState, "timer scheduler accessed outside its owner thread");
    }
    if (mTicking) {
        return Result<TimerTickOutcome>::err(ErrorCode::InvalidState,
                                             "timer scheduler tick cannot be re-entered");
    }
    if (mCurrentFrame == std::numeric_limits<std::uint64_t>::max()) {
        return Result<TimerTickOutcome>::err(ErrorCode::ResourceLimit,
                                             "timer frame counter is exhausted");
    }

    ++mCurrentFrame;
    mTicking = true;
    std::vector<std::uint64_t> due;
    for (const auto& [id, record] : mTimers) {
        if (record.active && record.dueFrame <= mCurrentFrame) {
            due.push_back(id);
        }
    }
    std::sort(due.begin(), due.end(), [&](std::uint64_t leftId, std::uint64_t rightId) {
        const Record& left = mTimers.at(leftId);
        const Record& right = mTimers.at(rightId);
        return std::tie(left.dueFrame, left.options.modLoadOrder, left.options.modPriority,
                        left.modId, left.handle.id) <
               std::tie(right.dueFrame, right.options.modLoadOrder, right.options.modPriority,
                        right.modId, right.handle.id);
    });

    TimerTickOutcome outcome;
    outcome.frame = mCurrentFrame;
    for (const std::uint64_t id : due) {
        auto found = mTimers.find(id);
        if (found == mTimers.end() || !found->second.active) {
            continue;
        }
        Record& record = found->second;
        ++outcome.callbacksInvoked;
        bool failed = false;
        std::string message;
        try {
            record.callback();
        } catch (const std::exception& error) {
            failed = true;
            message = error.what();
        } catch (...) {
            failed = true;
            message = "unknown timer callback failure";
        }

        if (failed) {
            ++record.consecutiveFailures;
            if (record.interval == 0 ||
                record.consecutiveFailures >= mMaxConsecutiveFailures) {
                record.active = false;
            }
            outcome.failures.push_back(
                {record.handle, record.modId, std::move(message), !record.active});
        } else {
            record.consecutiveFailures = 0;
            if (record.interval == 0) {
                record.active = false;
            }
        }

        if (record.active && record.interval != 0) {
            if (record.dueFrame >
                std::numeric_limits<std::uint64_t>::max() - record.interval) {
                record.active = false;
                outcome.failures.push_back({record.handle, record.modId,
                                            "timer recurrence exceeds the frame counter", true});
            } else {
                record.dueFrame += record.interval;
            }
        }
    }
    mTicking = false;
    RemoveInactive();
    return Result<TimerTickOutcome>::ok(std::move(outcome));
}

std::uint64_t FrameTimerScheduler::CurrentFrame() const {
    return mCurrentFrame;
}

std::size_t FrameTimerScheduler::TimerCount() const {
    return mTimers.size();
}

} // namespace ShipLua
