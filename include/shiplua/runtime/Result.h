#pragma once

#include <optional>
#include <string>
#include <utility>

namespace ShipLua {

enum class ErrorCode {
    Ok,
    Unsupported,
    InvalidArgument,
    InvalidHandle,
    InvalidState,
    PermissionDenied,
    ResourceLimit,
    HostFailure,
    ScriptFailure
};

template <typename T>
struct Result {
    ErrorCode code = ErrorCode::Ok;
    std::string message;
    std::optional<T> value;

    static Result ok(T v) {
        Result r;
        r.code = ErrorCode::Ok;
        r.value = std::move(v);
        return r;
    }

    static Result err(ErrorCode c, std::string msg) {
        Result r;
        r.code = c;
        r.message = std::move(msg);
        return r;
    }

    bool isOk() const { return code == ErrorCode::Ok; }
};

template <>
struct Result<void> {
    ErrorCode code = ErrorCode::Ok;
    std::string message;

    static Result ok() {
        Result r;
        r.code = ErrorCode::Ok;
        return r;
    }

    static Result err(ErrorCode c, std::string msg) {
        Result r;
        r.code = c;
        r.message = std::move(msg);
        return r;
    }

    bool isOk() const { return code == ErrorCode::Ok; }
};

} // namespace ShipLua
