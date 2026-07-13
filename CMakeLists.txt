cmake_minimum_required(VERSION 3.20)
project(ShipLua LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Lua 5.4 (official source mirror — raw sources, no build system of its own)
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lua)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.1.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniz)

if(NOT EXISTS "${lua_SOURCE_DIR}/lua.h")
    message(FATAL_ERROR
        "Lua sources were not fetched (network problem?). "
        "FetchContent could not populate https://github.com/lua/lua.git v5.4.7.")
endif()

file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
# Exclude the standalone interpreter/compiler entry points, the single-file
# amalgamation, and the internal test module shipped in the mirror.
list(FILTER LUA_SOURCES EXCLUDE REGEX "/(lua|luac|onelua|ltests)\\.c$")

add_library(lua_static STATIC ${LUA_SOURCES})
set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE C)
target_include_directories(lua_static PUBLIC "${lua_SOURCE_DIR}")
# No LUA_USE_POSIX / platform macros: generic ANSI build works on Windows/MinGW.

# ---------------------------------------------------------------------------
# ShipLua core
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SHIPLUA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(shiplua STATIC ${SHIPLUA_SOURCES})
target_include_directories(shiplua PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/include")
target_link_libraries(shiplua PRIVATE lua_static tomlplusplus::tomlplusplus miniz)

add_executable(shiplua_manifest_validator tools/manifest_validator.cpp)
target_link_libraries(shiplua_manifest_validator PRIVATE shiplua)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
