#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/Result.h"

// Forward declaration so consumers of this header do not need Lua headers.
struct lua_State;

namespace ShipLua {

// An isolated, sandboxed Lua 5.4 runtime bound to a single mod.
// Non-copyable, movable. Destroying the runtime closes its lua_State.
class LuaRuntime {
  public:
    explicit LuaRuntime(std::string modId, Logger logger = {}, std::size_t memoryLimitBytes = 0);
    ~LuaRuntime();

    LuaRuntime(const LuaRuntime&) = delete;
    LuaRuntime& operator=(const LuaRuntime&) = delete;

    LuaRuntime(LuaRuntime&& other) noexcept;
    LuaRuntime& operator=(LuaRuntime&& other) noexcept;

    const std::string& ModId() const;

    // Internal accessor for host-integration layers. May be nullptr if
    // construction failed or the runtime was moved-from.
    lua_State* State() const;

    // Compiles and runs `code` in a protected call with a traceback message
    // handler. Never throws and never lets a Lua error escape; after an
    // error the runtime remains usable.
    Result<void> DoString(const std::string& code, const std::string& chunkName = "chunk");

  private:
    struct AllocatorState;

    void OpenSandboxedLibraries();
    void Close() noexcept;

    std::string mModId;
    Logger mLogger;
    std::unique_ptr<AllocatorState> mAllocState;
    lua_State* mState = nullptr;
};

} // namespace ShipLua
