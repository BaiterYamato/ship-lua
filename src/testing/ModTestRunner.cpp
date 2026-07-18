#include "shiplua/testing/ModTestRunner.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

#include "shiplua/runtime/LuaRuntime.h"

#if __has_include(<lua.hpp>)
#include <lua.hpp>
#else
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#endif

namespace ShipLua {

std::size_t ModTestReport::PassedCount() const {
    std::size_t passed = 0;
    for (const ModTestCaseResult& test : tests) {
        if (test.ok) {
            ++passed;
        }
    }
    return passed;
}

std::size_t ModTestReport::FailedCount() const {
    return tests.size() - PassedCount();
}

ModTestRunner::ModTestRunner(MockHostOptions options) : mOptions(std::move(options)) {}

namespace {

// ---------------------------------------------------------------------------
// DSL de testes (Lua): describe / it / assert estendido / __shiplua_test
// ---------------------------------------------------------------------------

const char* const kTestDsl = R"lua(
do
    local builtin_assert = _G.assert
    local results = {}
    local suite_stack = {}

    local function full_name(name)
        local parts = {}
        for _, suite in ipairs(suite_stack) do
            parts[#parts + 1] = suite
        end
        parts[#parts + 1] = name
        return table.concat(parts, " / ")
    end

    local function record(name, ok, err)
        results[#results + 1] = { name = name, ok = ok and true or false, err = err }
    end

    function describe(name, fn)
        builtin_assert(type(name) == "string", "describe exige um nome textual")
        builtin_assert(type(fn) == "function", "describe exige uma função")
        suite_stack[#suite_stack + 1] = name
        local ok, err = pcall(fn)
        suite_stack[#suite_stack] = nil
        if not ok then
            record(full_name("(bloco describe)"), false, tostring(err))
        end
    end

    function it(name, fn)
        builtin_assert(type(name) == "string", "it exige um nome textual")
        builtin_assert(type(fn) == "function", "it exige uma função")
        local ok, err = pcall(fn)
        record(full_name(name), ok, ok and nil or tostring(err))
    end

    local function fmt(value)
        if type(value) == "string" then
            return string.format("%q", value)
        end
        return tostring(value)
    end

    local function fail(message)
        error(message, 3)
    end

    local test_assert = {}

    function test_assert.equals(expected, actual, message)
        if expected ~= actual then
            fail((message and (message .. ": ") or "") ..
                 "esperado " .. fmt(expected) .. ", obtido " .. fmt(actual))
        end
        return true
    end

    local function deep_equals(left, right, depth)
        if left == right then
            return true
        end
        if type(left) ~= "table" or type(right) ~= "table" then
            return false
        end
        depth = (depth or 0) + 1
        if depth > 16 then
            return false
        end
        for key, value in pairs(left) do
            if not deep_equals(value, right[key], depth) then
                return false
            end
        end
        for key in pairs(right) do
            if left[key] == nil then
                return false
            end
        end
        return true
    end

    function test_assert.same(expected, actual, message)
        if not deep_equals(expected, actual) then
            fail((message and (message .. ": ") or "") ..
                 "valores diferentes (esperado " .. fmt(expected) ..
                 ", obtido " .. fmt(actual) .. ")")
        end
        return true
    end

    function test_assert.is_true(value, message)
        if value ~= true then
            fail((message and (message .. ": ") or "") .. "esperado true, obtido " .. fmt(value))
        end
        return true
    end

    function test_assert.is_false(value, message)
        if value ~= false then
            fail((message and (message .. ": ") or "") .. "esperado false, obtido " .. fmt(value))
        end
        return true
    end

    function test_assert.is_nil(value, message)
        if value ~= nil then
            fail((message and (message .. ": ") or "") .. "esperado nil, obtido " .. fmt(value))
        end
        return true
    end

    function test_assert.not_nil(value, message)
        if value == nil then
            fail((message and (message .. ": ") or "") .. "esperado valor não nulo")
        end
        return true
    end

    function test_assert.contains(haystack, needle, message)
        if type(haystack) ~= "string" or string.find(haystack, needle, 1, true) == nil then
            fail((message and (message .. ": ") or "") ..
                 fmt(haystack) .. " não contém " .. fmt(needle))
        end
        return true
    end

    function test_assert.fail(message)
        fail(message or "falha explícita")
    end

    setmetatable(test_assert, {
        __call = function(_, condition, message, ...)
            return builtin_assert(condition, message, ...)
        end,
    })

    _G.assert = test_assert
    _G.__shiplua_test = { results = results }
end
)lua";

// ---------------------------------------------------------------------------
// Módulo Lua `mock` (somente modo de teste)
// ---------------------------------------------------------------------------

int MockFail(lua_State* state, const char* message) {
    lua_pushstring(state, message);
    return lua_error(state);
}

int MockFail(lua_State* state, const std::string& message) {
    return MockFail(state, message.c_str());
}

MockRuntime* MockFrom(lua_State* state) {
    return static_cast<MockRuntime*>(lua_touserdata(state, lua_upvalueindex(1)));
}

void SetMockFunction(lua_State* state, int tableIndex, const char* name, lua_CFunction function,
                     MockRuntime* mock) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_pushlightuserdata(state, mock);
    lua_pushcclosure(state, function, 1);
    lua_setfield(state, tableIndex, name);
}

Result<EventValue> PullEventValue(lua_State* state, int index, int depth) {
    if (depth > 16) {
        return Result<EventValue>::err(ErrorCode::InvalidArgument,
                                       "payload excede a profundidade máxima de 16 níveis");
    }
    index = lua_absindex(state, index);
    switch (lua_type(state, index)) {
        case LUA_TBOOLEAN:
            return Result<EventValue>::ok(EventValue(lua_toboolean(state, index) != 0));
        case LUA_TNUMBER:
            if (lua_isinteger(state, index)) {
                return Result<EventValue>::ok(
                    EventValue(static_cast<std::int64_t>(lua_tointeger(state, index))));
            }
            return Result<EventValue>::ok(
                EventValue(static_cast<double>(lua_tonumber(state, index))));
        case LUA_TSTRING: {
            size_t length = 0;
            const char* text = lua_tolstring(state, index, &length);
            return Result<EventValue>::ok(
                EventValue(text != nullptr ? std::string(text, length) : std::string{}));
        }
        case LUA_TTABLE: {
            bool hasStringKeys = false;
            bool hasIntegerKeys = false;
            std::size_t count = 0;
            lua_pushnil(state);
            while (lua_next(state, index) != 0) {
                ++count;
                if (lua_type(state, -2) == LUA_TSTRING) {
                    hasStringKeys = true;
                } else if (lua_isinteger(state, -2)) {
                    hasIntegerKeys = true;
                } else {
                    lua_pop(state, 2);
                    return Result<EventValue>::err(
                        ErrorCode::InvalidArgument,
                        "chaves de payload devem ser texto ou inteiros");
                }
                lua_pop(state, 1);
            }
            if (hasStringKeys && hasIntegerKeys) {
                return Result<EventValue>::err(
                    ErrorCode::InvalidArgument,
                    "payload não pode misturar chaves textuais e inteiras");
            }
            if (hasIntegerKeys) {
                EventValue::Array array;
                array.reserve(count);
                for (std::size_t i = 1; i <= count; ++i) {
                    lua_geti(state, index, static_cast<lua_Integer>(i));
                    if (lua_isnil(state, -1)) {
                        lua_pop(state, 1);
                        return Result<EventValue>::err(
                            ErrorCode::InvalidArgument,
                            "array de payload não pode ter buracos");
                    }
                    auto value = PullEventValue(state, -1, depth + 1);
                    lua_pop(state, 1);
                    if (!value.isOk()) {
                        return value;
                    }
                    array.push_back(std::move(*value.value));
                }
                return Result<EventValue>::ok(EventValue(std::move(array)));
            }
            EventValue::Object object;
            lua_pushnil(state);
            while (lua_next(state, index) != 0) {
                size_t length = 0;
                const char* key = lua_tolstring(state, -2, &length);
                auto value = PullEventValue(state, -1, depth + 1);
                if (!value.isOk()) {
                    lua_pop(state, 2);
                    return value;
                }
                object[std::string(key, length)] = std::move(*value.value);
                lua_pop(state, 1);
            }
            return Result<EventValue>::ok(EventValue(std::move(object)));
        }
        default:
            return Result<EventValue>::err(
                ErrorCode::InvalidArgument,
                "payload aceita somente booleano, número, texto e tabelas");
    }
}

int MockEventsEmit(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr || lua_type(state, 1) != LUA_TSTRING) {
        return MockFail(state, "mock.events.emit exige um nome de evento textual");
    }
    size_t length = 0;
    const char* name = lua_tolstring(state, 1, &length);
    EventPayload payload;
    if (lua_gettop(state) >= 2 && !lua_isnil(state, 2)) {
        if (!lua_istable(state, 2)) {
            return MockFail(state, "payload de mock.events.emit deve ser uma tabela");
        }
        auto pulled = PullEventValue(state, 2, 0);
        if (!pulled.isOk()) {
            return MockFail(state, pulled.message);
        }
        if (!std::holds_alternative<EventValue::Object>(pulled.value->value)) {
            return MockFail(state, "payload de mock.events.emit deve ser um objeto");
        }
        payload = std::get<EventValue::Object>(std::move(pulled.value->value));
    }
    try {
        auto outcome = mock->EmitEvent(std::string(name, length), std::move(payload));
        if (!outcome.isOk()) {
            return MockFail(state, outcome.message);
        }
        lua_pushinteger(state, static_cast<lua_Integer>(outcome.value->callbacksInvoked));
        return 1;
    } catch (...) {
        return MockFail(state, "falha do host ao emitir evento no mock");
    }
}

int MockTimerAdvance(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    std::uint64_t frames = 1;
    if (lua_gettop(state) >= 1 && !lua_isnil(state, 1)) {
        if (!lua_isinteger(state, 1)) {
            return MockFail(state, "mock.timer.advance exige frames inteiras");
        }
        const lua_Integer requested = lua_tointeger(state, 1);
        if (requested < 1 || requested > 1000000) {
            return MockFail(state, "mock.timer.advance aceita entre 1 e 1000000 frames");
        }
        frames = static_cast<std::uint64_t>(requested);
    }
    if (mock == nullptr) {
        return MockFail(state, "contexto do mock indisponível");
    }
    try {
        auto advanced = mock->AdvanceFrames(frames);
        if (!advanced.isOk()) {
            return MockFail(state, advanced.message);
        }
        lua_pushinteger(state, static_cast<lua_Integer>(*advanced.value));
        return 1;
    } catch (...) {
        return MockFail(state, "falha do host ao avançar frames no mock");
    }
}

int MockTimerFrame(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr) {
        return MockFail(state, "contexto do mock indisponível");
    }
    lua_pushinteger(state, static_cast<lua_Integer>(mock->CurrentFrame()));
    return 1;
}

int MockInputPress(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr || lua_type(state, 1) != LUA_TSTRING) {
        return MockFail(state, "mock.input.press exige uma tecla textual");
    }
    size_t length = 0;
    const char* key = lua_tolstring(state, 1, &length);
    if (key == nullptr || length == 0) {
        return MockFail(state, "mock.input.press exige uma tecla não vazia");
    }
    try {
        const std::size_t fired = mock->PressKey(std::string(key, length));
        lua_pushinteger(state, static_cast<lua_Integer>(fired));
        return 1;
    } catch (...) {
        return MockFail(state, "falha do host ao simular input no mock");
    }
}

int MockInputTrigger(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr || lua_type(state, 1) != LUA_TSTRING) {
        return MockFail(state, "mock.input.trigger exige um id textual");
    }
    size_t length = 0;
    const char* id = lua_tolstring(state, 1, &length);
    if (id == nullptr || length == 0) {
        return MockFail(state, "mock.input.trigger exige um id não vazio");
    }
    try {
        lua_pushboolean(state, mock->TriggerHotkey(std::string(id, length)) ? 1 : 0);
        return 1;
    } catch (...) {
        return MockFail(state, "falha do host ao disparar hotkey no mock");
    }
}

int MockLogEntries(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr) {
        return MockFail(state, "contexto do mock indisponível");
    }
    const std::vector<MockLogEntry>& logs = mock->Logs();
    lua_createtable(state, static_cast<int>(logs.size()), 0);
    lua_Integer index = 1;
    for (const MockLogEntry& entry : logs) {
        lua_createtable(state, 0, 3);
        lua_pushstring(state, LogLevelName(entry.level));
        lua_setfield(state, -2, "level");
        lua_pushlstring(state, entry.modId.data(), entry.modId.size());
        lua_setfield(state, -2, "mod");
        lua_pushlstring(state, entry.message.data(), entry.message.size());
        lua_setfield(state, -2, "message");
        lua_seti(state, -2, index++);
    }
    return 1;
}

int MockLogClear(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr) {
        return MockFail(state, "contexto do mock indisponível");
    }
    mock->ClearLogs();
    lua_pushboolean(state, 1);
    return 1;
}

int MockReset(lua_State* state) noexcept {
    MockRuntime* mock = MockFrom(state);
    if (mock == nullptr) {
        return MockFail(state, "contexto do mock indisponível");
    }
    mock->ResetSimulation();
    lua_pushboolean(state, 1);
    return 1;
}

void InstallMockModule(lua_State* state, MockRuntime* mock) {
    lua_newtable(state);
    const int mockIndex = lua_gettop(state);

    lua_newtable(state);
    SetMockFunction(state, -1, "emit", MockEventsEmit, mock);
    lua_setfield(state, mockIndex, "events");

    lua_newtable(state);
    SetMockFunction(state, -1, "advance", MockTimerAdvance, mock);
    SetMockFunction(state, -1, "frame", MockTimerFrame, mock);
    lua_setfield(state, mockIndex, "timer");

    lua_newtable(state);
    SetMockFunction(state, -1, "press", MockInputPress, mock);
    SetMockFunction(state, -1, "trigger", MockInputTrigger, mock);
    lua_setfield(state, mockIndex, "input");

    lua_newtable(state);
    SetMockFunction(state, -1, "entries", MockLogEntries, mock);
    SetMockFunction(state, -1, "clear", MockLogClear, mock);
    lua_setfield(state, mockIndex, "log");

    SetMockFunction(state, mockIndex, "reset", MockReset, mock);

    lua_setglobal(state, "mock");
}

// ---------------------------------------------------------------------------
// Coleta dos resultados registrados pela DSL
// ---------------------------------------------------------------------------

void HarvestResults(lua_State* state, const std::string& fileLabel, ModTestReport& report) {
    lua_getglobal(state, "__shiplua_test");
    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        report.errors.push_back(fileLabel + ": DSL de teste não inicializada no runtime do mod");
        return;
    }
    lua_getfield(state, -1, "results");
    if (!lua_istable(state, -1)) {
        lua_pop(state, 2);
        report.errors.push_back(fileLabel + ": tabela de resultados da DSL ausente");
        return;
    }
    const lua_Integer count = static_cast<lua_Integer>(luaL_len(state, -1));
    for (lua_Integer i = 1; i <= count; ++i) {
        lua_geti(state, -1, i);
        ModTestCaseResult test;
        test.file = fileLabel;
        lua_getfield(state, -1, "name");
        if (lua_type(state, -1) == LUA_TSTRING) {
            test.test = lua_tostring(state, -1);
        }
        lua_getfield(state, -2, "ok");
        test.ok = lua_toboolean(state, -1) != 0;
        lua_getfield(state, -3, "err");
        if (!test.ok && lua_type(state, -1) == LUA_TSTRING) {
            test.message = lua_tostring(state, -1);
        }
        lua_pop(state, 4); // err, ok, name, entrada
        report.tests.push_back(std::move(test));
    }
    lua_pop(state, 2); // results, __shiplua_test
}

Result<std::string> ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<std::string>::err(ErrorCode::InvalidArgument,
                                        "cannot open file '" + path.string() + "'");
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad()) {
        return Result<std::string>::err(ErrorCode::InvalidArgument,
                                        "failed to read file '" + path.string() + "'");
    }
    return Result<std::string>::ok(contents.str());
}

} // namespace

std::vector<std::filesystem::path> ModTestRunner::DiscoverTestFiles(
    const std::filesystem::path& modDir) {
    std::vector<std::filesystem::path> files;
    const std::filesystem::path testsDir = modDir / "tests";
    std::error_code error;
    if (!std::filesystem::is_directory(testsDir, error) || error) {
        return files;
    }
    for (const auto& entry : std::filesystem::directory_iterator(testsDir, error)) {
        if (error) {
            break;
        }
        const auto status = std::filesystem::symlink_status(entry.path(), error);
        if (error || !std::filesystem::is_regular_file(status) ||
            std::filesystem::is_symlink(status)) {
            continue;
        }
        if (entry.path().extension() == ".lua") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end(), [](const std::filesystem::path& left,
                                             const std::filesystem::path& right) {
        return left.filename().generic_string() < right.filename().generic_string();
    });
    return files;
}

Result<ModTestReport> ModTestRunner::Run(const std::filesystem::path& modDir) {
    ModTestReport report;
    std::error_code error;
    const auto status = std::filesystem::symlink_status(modDir, error);
    if (error || !std::filesystem::is_directory(status) || std::filesystem::is_symlink(status)) {
        return Result<ModTestReport>::err(
            ErrorCode::InvalidArgument,
            "mod '" + modDir.string() + "' não é um diretório regular acessível");
    }

    const std::vector<std::filesystem::path> files = DiscoverTestFiles(modDir);
    if (files.empty()) {
        report.noTestsFound = true;
        return Result<ModTestReport>::ok(std::move(report));
    }
    for (const std::filesystem::path& file : files) {
        RunFile(modDir, file, report);
    }
    report.files = files.size();
    return Result<ModTestReport>::ok(std::move(report));
}

void ModTestRunner::RunFile(const std::filesystem::path& modDir,
                            const std::filesystem::path& testFile, ModTestReport& report) {
    const std::string fileLabel = "tests/" + testFile.filename().generic_string();

    auto created = MockRuntime::Create(mOptions);
    if (!created.isOk()) {
        report.errors.push_back(fileLabel + ": não foi possível criar o mock: " + created.message);
        return;
    }
    MockRuntime& mock = *created.value;

    auto loaded = mock.LoadModFromDirectory(modDir.string());
    if (!loaded.isOk()) {
        report.errors.push_back(fileLabel + ": mod não carregou no mock: " + loaded.message);
        return;
    }
    report.modId = mock.ModId();

    LuaRuntime* runtime = mock.Host().GetRuntime(mock.ModId());
    lua_State* state = mock.ModState();
    if (runtime == nullptr || state == nullptr) {
        report.errors.push_back(fileLabel + ": runtime do mod indisponível no mock");
        return;
    }

    InstallMockModule(state, &mock);
    const auto dsl = runtime->DoString(kTestDsl, "shiplua/test-dsl.lua");
    if (!dsl.isOk()) {
        report.errors.push_back(fileLabel + ": falha ao instalar a DSL de teste: " + dsl.message);
        return;
    }

    // Lifecycle: game.ready ao carregar; game.shutdown ao final do arquivo.
    (void)mock.EmitGameReady();

    auto source = ReadTextFile(testFile);
    if (!source.isOk()) {
        report.errors.push_back(fileLabel + ": não foi possível ler o arquivo: " + source.message);
        (void)mock.EmitGameShutdown();
        return;
    }
    const std::string chunkName = mock.ModId() + "/" + fileLabel;
    const auto executed = runtime->DoString(*source.value, chunkName);
    if (!executed.isOk()) {
        // Erro de topo (sintaxe ou erro fora de it): os its já executados
        // continuam válidos e são coletados abaixo.
        report.tests.push_back(
            ModTestCaseResult{fileLabel, "(arquivo de teste)", false, executed.message});
    }

    HarvestResults(state, fileLabel, report);
    (void)mock.EmitGameShutdown();
}

} // namespace ShipLua
