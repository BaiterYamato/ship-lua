#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

struct EventValue {
    using Array = std::vector<EventValue>;
    using Object = std::map<std::string, EventValue>;
    using Storage = std::variant<std::monostate, bool, std::int64_t, double, std::string, Array, Object>;

    Storage value;

    EventValue() = default;
    EventValue(bool input) : value(input) {}
    EventValue(std::int64_t input) : value(input) {}
    EventValue(double input) : value(input) {}
    EventValue(std::string input) : value(std::move(input)) {}
    EventValue(const char* input) : value(std::string(input)) {}
    EventValue(Array input) : value(std::move(input)) {}
    EventValue(Object input) : value(std::move(input)) {}
};

using EventPayload = EventValue::Object;

enum class EventKind {
    Observe,
    Filter,
    Transform,
    Consume,
};

enum class EventFlow {
    Continue,
    Block,
    Consume,
};

struct Subscription {
    std::uint64_t id = 0;
};

struct SubscriptionOptions {
    std::size_t modLoadOrder = 0;
    int modPriority = 50;
    int callbackPriority = 0;
};

using EventCallback = std::function<EventFlow(EventPayload&)>;

struct CallbackFailure {
    Subscription subscription;
    std::string modId;
    std::string message;
};

struct DispatchOutcome {
    bool blocked = false;
    bool consumed = false;
    std::size_t callbacksInvoked = 0;
    std::vector<CallbackFailure> failures;
};

class EventDispatcher {
  public:
    EventDispatcher();

    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;

    Result<void> DefineEvent(const std::string& name, EventKind kind);
    Result<Subscription> Subscribe(const std::string& eventName, const std::string& modId,
                                   SubscriptionOptions options, EventCallback callback);
    Result<void> Unsubscribe(Subscription subscription);
    Result<DispatchOutcome> Dispatch(const std::string& eventName, EventPayload& payload);

    std::size_t SubscriptionCount() const;

  private:
    struct OrderKey {
        std::string eventName;
        std::size_t modLoadOrder = 0;
        int modPriority = 50;
        int callbackPriority = 0;
        std::string modId;
        std::uint64_t registrationId = 0;

        bool operator<(const OrderKey& other) const;
    };

    struct Record {
        OrderKey key;
        EventCallback callback;
        bool active = true;
    };

    bool IsOwnerThread() const;
    void FinishDispatch();
    void InsertRecord(Record record);

    std::thread::id mOwnerThread;
    std::map<std::string, EventKind> mEvents;
    std::map<OrderKey, Record> mSubscriptions;
    std::map<std::uint64_t, OrderKey> mById;
    std::vector<Record> mPending;
    std::uint64_t mNextRegistrationId = 1;
    std::size_t mDispatchDepth = 0;
};

} // namespace ShipLua
