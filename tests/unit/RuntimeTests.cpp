// Minimal header-free test harness for the ShipLua runtime core.
// No external test framework; main() returns nonzero if any check failed.

#include <cstdio>
#include <string>

#include "shiplua/runtime/LuaRuntime.h"
#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/Result.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (cond) {                                                         \
            std::printf("PASS: %s\n", msg);                                  \
        } else {                                                             \
            std::printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

using ShipLua::ErrorCode;
using ShipLua::Logger;
using ShipLua::LogLevel;
using ShipLua::LuaRuntime;

// A quiet logger so expected script errors don't spam stderr; still counts
// how many Error-level messages were emitted.
static Logger MakeQuietLogger(int* errorCount = nullptr) {
    return Logger([errorCount](LogLevel level, const std::string&, const std::string&) {
        if (level == LogLevel::Error && errorCount != nullptr) {
            ++(*errorCount);
        }
    });
}

static void TestIsolation() {
    LuaRuntime a("modA", MakeQuietLogger());
    LuaRuntime b("modB", MakeQuietLogger());

    CHECK(a.DoString("x = 5").isOk(), "isolation: set x in runtime A");
    CHECK(b.DoString("assert(x == nil)").isOk(), "isolation: x is nil in runtime B");
    CHECK(a.DoString("assert(x == 5)").isOk(), "isolation: x still 5 in runtime A");
    CHECK(a.ModId() == "modA", "isolation: ModId preserved");
}

static void TestDangerousLibsAbsent() {
    LuaRuntime rt("sandbox", MakeQuietLogger());
    auto r = rt.DoString(
        "assert(io == nil)\n"
        "assert(debug == nil)\n"
        "assert(package == nil)\n"
        "assert(os.execute == nil)\n"
        "assert(os.exit == nil)\n"
        "assert(os.getenv == nil)\n"
        "assert(os.remove == nil)\n"
        "assert(os.rename == nil)\n"
        "assert(os.tmpname == nil)\n"
        "assert(os.setlocale == nil)\n"
        "assert(dofile == nil)\n"
        "assert(loadfile == nil)\n");
    CHECK(r.isOk(), "sandbox: dangerous libs and functions absent");
    if (!r.isOk()) {
        std::printf("  error: %s\n", r.message.c_str());
    }
}

static void TestSafeLibsPresent() {
    LuaRuntime rt("safe", MakeQuietLogger());
    auto r = rt.DoString(
        "assert(type(os.time) == 'function')\n"
        "assert(type(os.clock) == 'function')\n"
        "assert(type(os.date) == 'function')\n"
        "assert(type(os.difftime) == 'function')\n"
        "assert(string.format('%d', 3) == '3')\n"
        "assert(math.floor(2.7) == 2)\n"
        "assert(type(table.insert) == 'function')\n"
        "assert(type(coroutine.create) == 'function')\n"
        "assert(type(utf8.char) == 'function')\n"
        "assert(type(load) == 'function')\n"
        "assert(type(pcall) == 'function')\n");
    CHECK(r.isOk(), "sandbox: safe libs present and working");
    if (!r.isOk()) {
        std::printf("  error: %s\n", r.message.c_str());
    }
}

static void TestProtectedError() {
    int errorCount = 0;
    LuaRuntime rt("errmod", MakeQuietLogger(&errorCount));

    auto bad = rt.DoString("error('boom')");
    CHECK(!bad.isOk(), "protected error: DoString reports failure");
    CHECK(bad.code == ErrorCode::ScriptFailure, "protected error: code is ScriptFailure");
    CHECK(bad.message.find("boom") != std::string::npos,
          "protected error: message contains 'boom'");
    CHECK(bad.message.find("stack traceback") != std::string::npos,
          "protected error: message contains a traceback");
    CHECK(errorCount == 1, "protected error: error was logged with modId");

    auto ok = rt.DoString("return 1");
    CHECK(ok.isOk(), "protected error: runtime still usable after error");

    // Compile error path.
    auto compileErr = rt.DoString("this is not lua !!!");
    CHECK(!compileErr.isOk() && compileErr.code == ErrorCode::ScriptFailure,
          "protected error: compile error reported as ScriptFailure");
}

static void TestLifecycle() {
    {
        LuaRuntime rt("shortlived", MakeQuietLogger());
        CHECK(rt.DoString("y = 'hello'").isOk(), "lifecycle: nested-scope runtime works");
    } // destructor closes the state here

    LuaRuntime rt2("afterwards", MakeQuietLogger());
    CHECK(rt2.DoString("assert(y == nil)").isOk(), "lifecycle: fresh runtime after destruction");

    // Move semantics: moved-from runtime must not double-close.
    LuaRuntime moved = std::move(rt2);
    CHECK(moved.DoString("return 1").isOk(), "lifecycle: moved-to runtime usable");
    CHECK(rt2.State() == nullptr, "lifecycle: moved-from runtime has null state");
}

static void TestMemoryLimit() {
    LuaRuntime rt("hungry", MakeQuietLogger(), 200000);
    auto r = rt.DoString(
        "local t = {}\n"
        "for i = 1, 1000000 do\n"
        "  t[#t + 1] = string.rep('x', 128) .. i\n"
        "end\n");
    CHECK(!r.isOk(), "memory limit: allocation-heavy script fails");
    CHECK(r.code == ErrorCode::ScriptFailure, "memory limit: code is ScriptFailure");

    // Runtime should still work for small allocations after OOM + GC.
    auto small = rt.DoString("collectgarbage(); return 1");
    CHECK(small.isOk(), "memory limit: runtime usable after OOM");
}

int main() {
    TestIsolation();
    TestDangerousLibsAbsent();
    TestSafeLibsPresent();
    TestProtectedError();
    TestLifecycle();
    TestMemoryLimit();

    if (g_failures == 0) {
        std::printf("\nAll tests passed.\n");
        return 0;
    }
    std::printf("\n%d check(s) FAILED.\n", g_failures);
    return 1;
}
