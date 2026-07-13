#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <utility>

namespace ShipLua {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

using LogSink = std::function<void(LogLevel, const std::string& modId, const std::string& message)>;

inline const char* LogLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

class Logger {
  public:
    Logger() = default;

    explicit Logger(LogSink sink) : mSink(std::move(sink)) {}

    void SetSink(LogSink sink) { mSink = std::move(sink); }

    void Log(LogLevel level, const std::string& modId, const std::string& message) const {
        if (mSink) {
            mSink(level, modId, message);
        } else {
            std::fprintf(stderr, "[%s][%s] %s\n", LogLevelName(level), modId.c_str(), message.c_str());
        }
    }

    void debug(const std::string& modId, const std::string& message) const {
        Log(LogLevel::Debug, modId, message);
    }
    void info(const std::string& modId, const std::string& message) const {
        Log(LogLevel::Info, modId, message);
    }
    void warn(const std::string& modId, const std::string& message) const {
        Log(LogLevel::Warn, modId, message);
    }
    void error(const std::string& modId, const std::string& message) const {
        Log(LogLevel::Error, modId, message);
    }

  private:
    LogSink mSink;
};

} // namespace ShipLua
