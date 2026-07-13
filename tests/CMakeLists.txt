add_executable(runtime_tests unit/RuntimeTests.cpp)
target_link_libraries(runtime_tests PRIVATE shiplua)

add_test(NAME runtime_tests COMMAND runtime_tests)

add_executable(manifest_host_tests unit/ManifestHostTests.cpp)
target_link_libraries(manifest_host_tests PRIVATE shiplua)

add_test(NAME manifest_host_tests COMMAND manifest_host_tests)

add_executable(manifest_discovery_tests unit/ManifestDiscoveryTests.cpp)
target_link_libraries(manifest_discovery_tests PRIVATE shiplua)
target_compile_definitions(manifest_discovery_tests PRIVATE
    SHIPLUA_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME manifest_discovery_tests COMMAND manifest_discovery_tests)
