// Executa generated/tests/mock_contract.lua em um lua_State bruto (stdlib
// completa), validando o mock runtime (generated/lua/shiplua_mock.lua) e os
// validadores (generated/lua/shiplua_validate.lua) derivados da IDL.

#include <iostream>
#include <string>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#ifndef SHIPLUA_GENERATED_DIR
#define SHIPLUA_GENERATED_DIR "generated"
#endif

namespace {

std::string NormalizeSlashes(std::string path) {
    for (char& character : path) {
        if (character == '\\') {
            character = '/';
        }
    }
    return path;
}

} // namespace

int main() {
    lua_State* state = luaL_newstate();
    if (state == nullptr) {
        std::cerr << "FAIL: could not create lua_State\n";
        return 1;
    }
    luaL_openlibs(state);

    // Torna require("shiplua_validate")/require("shiplua_mock") resolvíveis.
    const std::string luaDir = NormalizeSlashes(std::string(SHIPLUA_GENERATED_DIR) + "/lua");
    lua_getglobal(state, "package");
    lua_getfield(state, -1, "path");
    const std::string current = lua_tostring(state, -1) != nullptr ? lua_tostring(state, -1) : "";
    lua_pop(state, 1);
    lua_pushstring(state, (luaDir + "/?.lua;" + current).c_str());
    lua_setfield(state, -2, "path");
    lua_pop(state, 1);

    const std::string script =
        NormalizeSlashes(std::string(SHIPLUA_GENERATED_DIR) + "/tests/mock_contract.lua");
    int status = luaL_loadfile(state, script.c_str());
    if (status == LUA_OK) {
        status = lua_pcall(state, 0, 0, 0);
    }
    if (status != LUA_OK) {
        const char* message = lua_tostring(state, -1);
        std::cerr << "FAIL: mock contract script failed: "
                  << (message != nullptr ? message : "(no message)") << '\n';
        lua_close(state);
        return 1;
    }
    lua_close(state);
    std::cout << "All mock runtime contract checks passed\n";
    return 0;
}
