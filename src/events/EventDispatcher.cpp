#include "shiplua/events/EventDispatcher.h"

#include <exception>
#include <limits>
#include <tuple>
#include <utility>

namespace ShipLua {

bool EventDispatcher::OrderKey::operator<(const OrderKey& other) const {
    return std::tie(eventName, modLoadOrder, modPriority, callbackPriority, modId, registrationId) <
           std::tie(other.eventName, other.modLoadOrder, other.modPriority, other.callbackPriority,
                    other.modId, other.registrationId);
}

EventDispatcher::EventDispatcher() : mOwnerThread(std::this_thread::get_id()) {}

bool EventDispatcher::IsOwnerThread() const {
    return std::this_thread::get_id() == mOwnerThread;
}

Result<void> EventDispatcher::DefineEvent(const std::string& name, EventKind kind) {
    if (!IsOwnerThread()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "event dispatcher accessed outside its owner thread");
    }
    if (mDispatchDepth != 0) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "events cannot be defined during dispatch");
    }
    if (name.empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument, "event name cannot be empty");
    }
    if (!mEvents.emplace(name, kind).second) {
        return Result<void>::err(ErrorCode::InvalidState, "event is already defined: '" + name + "'");
    }
    return Result<void>::ok();
}

void EventDispatcher::InsertRecord(Record record) {
    const OrderKey key = record.key;
    mSubscriptions.emplace(key, std::move(record));
    mById.emplace(key.registrationId, key);
}

Result<Subscription> EventDispatcher::Subscribe(const std::string& eventName,
                                                 const std::string& modId,
                                                 SubscriptionOptions options,
                                                 EventCallback callback) {
    if (!IsOwnerThread()) {
        return Result<Subscription>::err(ErrorCode::InvalidState,
                                         "event dispatcher accessed outside its owner thread");
    }
    if (!mEvents.contains(eventName)) {
        return Result<Subscription>::err(ErrorCode::Unsupported,
                                         "event is not defined: '" + eventName + "'");
    }
    if (modId.empty() || !callback) {
        return Result<Subscription>::err(ErrorCode::InvalidArgument,
                                         "subscription requires a mod id and callback");
    }

    const Subscription subscription{mNextRegistrationId++};
    Record record{{eventName, options.modLoadOrder, options.modPriority,
                   options.callbackPriority, modId, subscription.id},
                  std::move(callback), true};
    if (mDispatchDepth == 0) {
        InsertRecord(std::move(record));
    } else {
        mById.emplace(subscription.id, record.key);
        mPending.push_back(std::move(record));
    }
    return Result<Subscription>::ok(subscription);
}

Result<void> EventDispatcher::Unsubscribe(Subscription subscription) {
    if (!IsOwnerThread()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "event dispatcher accessed outside its owner thread");
    }
    const auto byId = mById.find(subscription.id);
    if (subscription.id == 0 || byId == mById.end()) {
        return Result<void>::err(ErrorCode::InvalidHandle, "subscription is not active");
    }

    auto active = mSubscriptions.find(byId->second);
    if (active != mSubscriptions.end()) {
        if (mDispatchDepth == 0) {
            mSubscriptions.erase(active);
            mById.erase(byId);
        } else {
            active->second.active = false;
        }
        return Result<void>::ok();
    }
    for (Record& pending : mPending) {
        if (pending.key.registrationId == subscription.id && pending.active) {
            pending.active = false;
            return Result<void>::ok();
        }
    }
    return Result<void>::err(ErrorCode::InvalidHandle, "subscription is not active");
}

void EventDispatcher::FinishDispatch() {
    if (--mDispatchDepth != 0) {
        return;
    }
    for (auto it = mSubscriptions.begin(); it != mSubscriptions.end();) {
        if (!it->second.active) {
            mById.erase(it->first.registrationId);
            it = mSubscriptions.erase(it);
        } else {
            ++it;
        }
    }
    for (Record& pending : mPending) {
        if (pending.active) {
            mSubscriptions.emplace(pending.key, std::move(pending));
            // mById was inserted when the pending subscription was created.
        } else {
            mById.erase(pending.key.registrationId);
        }
    }
    mPending.clear();
}

Result<DispatchOutcome> EventDispatcher::Dispatch(const std::string& eventName,
                                                   EventPayload& payload) {
    if (!IsOwnerThread()) {
        return Result<DispatchOutcome>::err(
            ErrorCode::InvalidState, "event dispatcher accessed outside its owner thread");
    }
    const auto definition = mEvents.find(eventName);
    if (definition == mEvents.end()) {
        return Result<DispatchOutcome>::err(ErrorCode::Unsupported,
                                            "event is not defined: '" + eventName + "'");
    }

    DispatchOutcome outcome;
    ++mDispatchDepth;
    const OrderKey first{eventName, 0, std::numeric_limits<int>::min(),
                         std::numeric_limits<int>::min(), {}, 0};
    auto it = mSubscriptions.lower_bound(first);
    while (it != mSubscriptions.end() && it->first.eventName == eventName) {
        Record& record = it->second;
        ++it;
        if (!record.active) {
            continue;
        }
        ++outcome.callbacksInvoked;
        EventFlow flow = EventFlow::Continue;
        try {
            if (definition->second == EventKind::Transform) {
                flow = record.callback(payload);
            } else {
                EventPayload readOnlySnapshot = payload;
                flow = record.callback(readOnlySnapshot);
            }
        } catch (const std::exception& error) {
            outcome.failures.push_back(
                {{record.key.registrationId}, record.key.modId, error.what()});
            continue;
        } catch (...) {
            outcome.failures.push_back(
                {{record.key.registrationId}, record.key.modId, "unknown callback failure"});
            continue;
        }

        const EventKind kind = definition->second;
        if (kind == EventKind::Filter && flow == EventFlow::Block) {
            outcome.blocked = true;
            break;
        }
        if (kind == EventKind::Consume && flow == EventFlow::Consume) {
            outcome.consumed = true;
            break;
        }
        if ((kind == EventKind::Observe && flow != EventFlow::Continue) ||
            (kind == EventKind::Transform && flow != EventFlow::Continue) ||
            (kind == EventKind::Filter && flow == EventFlow::Consume) ||
            (kind == EventKind::Consume && flow == EventFlow::Block)) {
            outcome.failures.push_back({{record.key.registrationId}, record.key.modId,
                                        "callback returned a flow incompatible with event kind"});
        }
    }
    FinishDispatch();
    return Result<DispatchOutcome>::ok(std::move(outcome));
}

std::size_t EventDispatcher::SubscriptionCount() const {
    return mById.size();
}

} // namespace ShipLua
