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

add_executable(mod_root_loader_tests unit/ModRootLoaderTests.cpp)
target_link_libraries(mod_root_loader_tests PRIVATE shiplua miniz)

add_test(NAME mod_root_loader_tests COMMAND mod_root_loader_tests)

add_executable(event_dispatcher_tests unit/EventDispatcherTests.cpp)
target_link_libraries(event_dispatcher_tests PRIVATE shiplua)

add_test(NAME event_dispatcher_tests COMMAND event_dispatcher_tests)

add_executable(frame_timer_scheduler_tests unit/FrameTimerSchedulerTests.cpp)
target_link_libraries(frame_timer_scheduler_tests PRIVATE shiplua)

add_test(NAME frame_timer_scheduler_tests COMMAND frame_timer_scheduler_tests)

add_executable(world_session_tests unit/WorldSessionTests.cpp)
target_link_libraries(world_session_tests PRIVATE shiplua)

add_test(NAME world_session_tests COMMAND world_session_tests)

add_executable(portable_item_catalog_tests unit/PortableItemCatalogTests.cpp)
target_link_libraries(portable_item_catalog_tests PRIVATE shiplua)

add_test(NAME portable_item_catalog_tests COMMAND portable_item_catalog_tests)

add_executable(atomic_file_tests unit/AtomicFileTests.cpp)
target_link_libraries(atomic_file_tests PRIVATE shiplua)

add_test(NAME atomic_file_tests COMMAND atomic_file_tests)

add_executable(generated_api_bindings_tests unit/GeneratedApiBindingsTests.cpp)
target_link_libraries(generated_api_bindings_tests PRIVATE shiplua)

add_test(NAME generated_api_bindings_tests COMMAND generated_api_bindings_tests)

add_executable(lua_api_binding_tests unit/LuaApiBindingTests.cpp)
target_link_libraries(lua_api_binding_tests PRIVATE shiplua)

add_test(NAME lua_api_binding_tests COMMAND lua_api_binding_tests)

add_executable(hello_world_conformance_tests conformance/HelloWorldConformanceTests.cpp)
target_link_libraries(hello_world_conformance_tests PRIVATE shiplua)
target_compile_definitions(hello_world_conformance_tests PRIVATE
    SHIPLUA_HELLO_WORLD_PACKAGE="${SHIPLUA_HELLO_WORLD_PACKAGE}")
add_dependencies(hello_world_conformance_tests package_examples)

add_test(NAME hello_world_conformance_tests COMMAND hello_world_conformance_tests)
add_test(NAME hello_world_package_validation
    COMMAND shiplua_manifest_validator "${SHIPLUA_HELLO_WORLD_PACKAGE}")
add_test(NAME hello_world_packaging_tests
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/conformance/test_package_hello_world.py")

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

add_test(NAME api_schema_validation
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/validate_api_schemas.py")
add_test(NAME api_schema_validation_tests
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/schema/test_validate_api_schemas.py")
add_test(NAME api_cpp_codegen_check
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_cpp_api.py" --check)
add_test(NAME api_cpp_codegen_tests
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/schema/test_generate_cpp_api.py")
add_test(NAME api_docs_codegen_check
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_api_docs.py" --check)
add_test(NAME api_docs_codegen_tests
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/schema/test_generate_api_docs.py")
