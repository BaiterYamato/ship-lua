cmake_minimum_required(VERSION 3.20)
project(ShipLua LANGUAGES C CXX)

# Keep repository-relative paths stable when ShipLua is consumed through
# add_subdirectory() by a host project whose CMAKE_SOURCE_DIR points elsewhere.
set(SHIPLUA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

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

# miniz injects /Zi on MSVC, which conflicts with the project-wide /Z7 and
# makes sccache abort when it cannot find miniz.pdb. Pin miniz to a proper
# per-target program-database PDB so the Windows CI build stays cacheable.
if (MSVC)
    set_target_properties(miniz PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
endif()

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
list(FILTER SHIPLUA_SOURCES EXCLUDE REGEX "/launcher/")

add_library(shiplua STATIC ${SHIPLUA_SOURCES})
target_include_directories(shiplua PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/include")
target_link_libraries(shiplua PRIVATE lua_static tomlplusplus::tomlplusplus miniz)
# Expose Lua headers to privileged consumers (host adapters) so they can install
# host-specific bindings (e.g. ship.mm.*) directly on a runtime's lua_State.
# Mods never see this: it is a build-time include path for the C++ adapter only.
target_include_directories(shiplua PUBLIC "${lua_SOURCE_DIR}")

add_executable(shiplua_manifest_validator tools/manifest_validator.cpp)
target_link_libraries(shiplua_manifest_validator PRIVATE shiplua)

# ---------------------------------------------------------------------------
# Link-Span supervisor
# ---------------------------------------------------------------------------
add_library(linkspan_launcher_core STATIC
    src/launcher/AssetDiscovery.cpp)
target_include_directories(linkspan_launcher_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(linkspan_launcher_core PUBLIC miniz)

if(WIN32)
    add_executable(linkspan_launcher WIN32 src/launcher/main.cpp)
else()
    add_executable(linkspan_launcher src/launcher/main.cpp)
endif()
target_link_libraries(linkspan_launcher PRIVATE linkspan_launcher_core)
if(WIN32)
    target_link_libraries(linkspan_launcher PRIVATE comctl32)
endif()
if(MINGW)
    target_link_options(linkspan_launcher PRIVATE -municode)
endif()
set_target_properties(linkspan_launcher PROPERTIES
    OUTPUT_NAME "link-span"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/x64/$<CONFIG>")

add_executable(linkspan_asset_discovery_tests
    tests/launcher/AssetDiscoveryTests.cpp)
target_link_libraries(linkspan_asset_discovery_tests PRIVATE linkspan_launcher_core)
add_test(NAME linkspan_asset_discovery_tests COMMAND linkspan_asset_discovery_tests)

# ---------------------------------------------------------------------------
# Example packages
# ---------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(SHIPLUA_EXAMPLE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/examples")
set(SHIPLUA_HELLO_WORLD_PACKAGE "${SHIPLUA_EXAMPLE_OUTPUT_DIR}/hello-world.shipmod")
add_custom_command(
    OUTPUT "${SHIPLUA_HELLO_WORLD_PACKAGE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHIPLUA_EXAMPLE_OUTPUT_DIR}"
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/PackageHelloWorld.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/hello-world"
            "${SHIPLUA_HELLO_WORLD_PACKAGE}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/hello-world/manifest.toml"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/hello-world/main.lua"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/PackageHelloWorld.py"
    COMMENT "Empacotando hello-world.shipmod"
    VERBATIM)
add_custom_target(package_examples DEPENDS "${SHIPLUA_HELLO_WORLD_PACKAGE}")

set(SHIPLUA_LINKSPAN_DEMO_PACKAGE
    "${SHIPLUA_EXAMPLE_OUTPUT_DIR}/link-home-to-clock-tower.shipmod")
add_custom_command(
    OUTPUT "${SHIPLUA_LINKSPAN_DEMO_PACKAGE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHIPLUA_EXAMPLE_OUTPUT_DIR}"
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/PackageHelloWorld.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/link-home-to-clock-tower"
            "${SHIPLUA_LINKSPAN_DEMO_PACKAGE}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/link-home-to-clock-tower/manifest.toml"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/link-home-to-clock-tower/main.lua"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/PackageHelloWorld.py"
    COMMENT "Empacotando link-home-to-clock-tower.shipmod"
    VERBATIM)
add_custom_target(package_linkspan_demo DEPENDS "${SHIPLUA_LINKSPAN_DEMO_PACKAGE}")

# O mod test runner executa pacotes .shipmod em alguns testes do ctest; o
# pacote de exemplo precisa existir antes dele rodar.
add_executable(shiplua_mod_test_runner tools/mod_test_runner.cpp)
target_link_libraries(shiplua_mod_test_runner PRIVATE shiplua)
add_dependencies(shiplua_mod_test_runner package_examples)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
