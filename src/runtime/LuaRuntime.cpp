#include "shiplua/runtime/LuaRuntime.h"

#include <cstdlib>
#include <cstring>

// The official lua/lua source mirror does not ship lua.hpp (it was dropped
// from the Lua 5.4 distribution), so fall back to the extern "C" wrapper
// when it is unavailable.
#if __has_include(<lua.hpp>)
#include <lua.hpp>
#else
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

namespace ShipLua {

// ---------------------------------------------------------------------------
// Allocator
// ---------------------------------------------------------------------------

struct LuaRuntime::AllocatorState {
    std::size_t limitBytes = 0; // 0 == unlimited
    std::size_t usedBytes = 0;

    static void* Alloc(void* ud, void* ptr, size_t osize, size_t nsize);
};

void* LuaRuntime::AllocatorState::Alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    auto* state = static_cast<AllocatorState*>(ud);
    // Per the Lua manual: when ptr is NULL, osize encodes the type of the
    // object being allocated, not a previous size.
    const std::size_t oldSize = (ptr != nullptr) ? osize : 0;

    if (nsize == 0) {
        std::free(ptr);
        state->usedBytes -= oldSize;
        return nullptr;
    }

    // Enforce the memory cap only on growth; shrinking must never fail.
    if (state->limitBytes != 0 && nsize > oldSize &&
        (state->usedBytes - oldSize + nsize) > state->limitBytes) {
        return nullptr;
    }

    void* newPtr = std::realloc(ptr, nsize);
    if (newPtr == nullptr) {
        return nullptr;
    }
    state->usedBytes = state->usedBytes - oldSize + nsize;
    return newPtr;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

// Message handler for lua_pcall: appends a traceback to the error message
// (mirrors the handler used by the standalone lua interpreter).
int TracebackHandler(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (msg == nullptr) {
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) {
            return 1;
        }
        msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

// Opens the sandboxed subset of the standard libraries. Runs inside a
// protected call so a Lua error here cannot reach the panic handler.
int OpenSandboxedLibsProtected(lua_State* L) {
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
    lua_pop(L, 1);
    // Deliberately NOT opened: io, debug, package.

    // Remove filesystem-touching base functions.
    lua_pushnil(L);
    lua_setglobal(L, "dofile");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");

    // Sanitize the os table: keep time/clock/date/difftime only.
    lua_getglobal(L, LUA_OSLIBNAME);
    if (lua_istable(L, -1)) {
        static const char* const kRemoved[] = {
            "execute", "exit", "getenv", "remove", "rename", "tmpname", "setlocale",
        };
        for (const char* name : kRemoved) {
            lua_pushnil(L);
            lua_setfield(L, -2, name);
        }
    }
    lua_pop(L, 1);
    return 0;
}

// Extracts the value at the top of the stack as a string for error reporting.
std::string TopToString(lua_State* L) {
    size_t len = 0;
    const char* msg = lua_tolstring(L, -1, &len);
    if (msg != nullptr) {
        return std::string(msg, len);
    }
    return std::string("(error object is a ") + luaL_typename(L, -1) + " value)";
}

} // namespace

// ---------------------------------------------------------------------------
// LuaRuntime
// ---------------------------------------------------------------------------

LuaRuntime::LuaRuntime(std::string modId, Logger logger, std::size_t memoryLimitBytes)
    : mModId(std::move(modId)),
      mLogger(std::move(logger)),
      mAllocState(std::make_unique<AllocatorState>()) {
    mAllocState->limitBytes = memoryLimitBytes;

    mState = lua_newstate(&AllocatorState::Alloc, mAllocState.get());
    if (mState == nullptr) {
        mLogger.error(mModId, "failed to create lua_State (allocation failed or limit too small)");
        return;
    }
    OpenSandboxedLibraries();
}

LuaRuntime::~LuaRuntime() {
    Close();
}

LuaRuntime::LuaRuntime(LuaRuntime&& other) noexcept
    : mModId(std::move(other.mModId)),
      mLogger(std::move(other.mLogger)),
      mAllocState(std::move(other.mAllocState)),
      mState(other.mState) {
    other.mState = nullptr;
}

LuaRuntime& LuaRuntime::operator=(LuaRuntime&& other) noexcept {
    if (this != &other) {
        Close();
        mModId = std::move(other.mModId);
        mLogger = std::move(other.mLogger);
        mAllocState = std::move(other.mAllocState);
        mState = other.mState;
        other.mState = nullptr;
    }
    return *this;
}

const std::string& LuaRuntime::ModId() const {
    return mModId;
}

lua_State* LuaRuntime::State() const {
    return mState;
}

void LuaRuntime::OpenSandboxedLibraries() {
    lua_pushcfunction(mState, OpenSandboxedLibsProtected);
    if (lua_pcall(mState, 0, 0, 0) != LUA_OK) {
        std::string err = TopToString(mState);
        lua_pop(mState, 1);
        mLogger.error(mModId, "failed to open sandboxed libraries: " + err);
    }
}

void LuaRuntime::Close() noexcept {
    if (mState != nullptr) {
        lua_close(mState);
        mState = nullptr;
    }
}

Result<void> LuaRuntime::DoString(const std::string& code, const std::string& chunkName) {
    if (mState == nullptr) {
        return Result<void>::err(ErrorCode::InvalidState, "runtime has no lua_State");
    }

    try {
        lua_pushcfunction(mState, TracebackHandler);
        const int handlerIndex = lua_gettop(mState);

        int status = luaL_loadbuffer(mState, code.data(), code.size(), chunkName.c_str());
        if (status != LUA_OK) {
            std::string err = TopToString(mState);
            lua_pop(mState, 2); // error message + handler
            mLogger.error(mModId, err);
            return Result<void>::err(ErrorCode::ScriptFailure, err);
        }

        status = lua_pcall(mState, 0, 0, handlerIndex);
        if (status != LUA_OK) {
            std::string err = TopToString(mState);
            lua_pop(mState, 2); // error object + handler
            mLogger.error(mModId, err);
            return Result<void>::err(ErrorCode::ScriptFailure, err);
        }

        lua_pop(mState, 1); // handler
        return Result<void>::ok();
    } catch (const std::exception& e) {
        return Result<void>::err(ErrorCode::ScriptFailure,
                                 std::string("unexpected C++ exception: ") + e.what());
    } catch (...) {
        return Result<void>::err(ErrorCode::ScriptFailure, "unexpected unknown C++ exception");
    }
}

} // namespace ShipLua
