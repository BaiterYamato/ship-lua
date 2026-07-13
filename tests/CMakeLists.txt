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

add_executable(package_extractor_tests unit/PackageExtractorTests.cpp)
target_link_libraries(package_extractor_tests PRIVATE shiplua miniz)

add_test(NAME package_extractor_tests COMMAND package_extractor_tests)

add_executable(semver_tests unit/SemVersionTests.cpp)
target_link_libraries(semver_tests PRIVATE shiplua)

add_test(NAME semver_tests COMMAND semver_tests)

add_executable(dependency_resolver_tests unit/DependencyResolverTests.cpp)
target_link_libraries(dependency_resolver_tests PRIVATE shiplua)

add_test(NAME dependency_resolver_tests COMMAND dependency_resolver_tests)

add_executable(event_dispatcher_tests unit/EventDispatcherTests.cpp)
target_link_libraries(event_dispatcher_tests PRIVATE shiplua)

add_test(NAME event_dispatcher_tests COMMAND event_dispatcher_tests)

add_executable(frame_timer_scheduler_tests unit/FrameTimerSchedulerTests.cpp)
target_link_libraries(frame_timer_scheduler_tests PRIVATE shiplua)

add_test(NAME frame_timer_scheduler_tests COMMAND frame_timer_scheduler_tests)

add_test(NAME validator_valid_file
    COMMAND shiplua_manifest_validator
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/manifest/valid-minimal.toml")
add_test(NAME validator_valid_directory
    COMMAND shiplua_manifest_validator
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/validator/valid-mod")
add_test(NAME validator_invalid_manifest
    COMMAND shiplua_manifest_validator
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/manifest/invalid-version.toml")
set_tests_properties(validator_invalid_manifest PROPERTIES WILL_FAIL TRUE)
add_test(NAME validator_usage COMMAND shiplua_manifest_validator)
set_tests_properties(validator_usage PROPERTIES WILL_FAIL TRUE)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME api_schema_validation
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/validate_api_schemas.py")
add_test(NAME api_schema_validation_tests
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/schema/test_validate_api_schemas.py")
