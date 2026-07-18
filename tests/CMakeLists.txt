add_executable(runtime_tests unit/RuntimeTests.cpp)
target_link_libraries(runtime_tests PRIVATE shiplua)

add_test(NAME runtime_tests COMMAND runtime_tests)

add_executable(handle_registry_tests unit/HandleRegistryTests.cpp)
target_link_libraries(handle_registry_tests PRIVATE shiplua)

add_test(NAME handle_registry_tests COMMAND handle_registry_tests)

add_executable(mod_lifecycle_tests unit/ModLifecycleTests.cpp)
target_link_libraries(mod_lifecycle_tests PRIVATE shiplua)

add_test(NAME mod_lifecycle_tests COMMAND mod_lifecycle_tests)

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

add_executable(world_handoff_tests unit/WorldHandoffTests.cpp)
target_link_libraries(world_handoff_tests PRIVATE shiplua)

add_test(NAME world_handoff_tests COMMAND world_handoff_tests)

add_executable(world_asset_catalog_tests unit/WorldAssetCatalogTests.cpp)
target_link_libraries(world_asset_catalog_tests PRIVATE shiplua)

add_test(NAME world_asset_catalog_tests COMMAND world_asset_catalog_tests)

add_executable(generated_api_bindings_tests unit/GeneratedApiBindingsTests.cpp)
target_link_libraries(generated_api_bindings_tests PRIVATE shiplua)

add_test(NAME generated_api_bindings_tests COMMAND generated_api_bindings_tests)

add_executable(lua_api_binding_tests unit/LuaApiBindingTests.cpp)
target_link_libraries(lua_api_binding_tests PRIVATE shiplua)

add_test(NAME lua_api_binding_tests COMMAND lua_api_binding_tests)

add_executable(key_value_storage_tests unit/KeyValueStorageTests.cpp)
target_link_libraries(key_value_storage_tests PRIVATE shiplua)

add_test(NAME key_value_storage_tests COMMAND key_value_storage_tests)

add_executable(mock_runtime_unit_tests unit/MockRuntimeTests.cpp)
target_link_libraries(mock_runtime_unit_tests PRIVATE shiplua)

add_test(NAME mock_runtime_unit_tests COMMAND mock_runtime_unit_tests)

add_executable(mod_test_runner_tests unit/ModTestRunnerTests.cpp)
target_link_libraries(mod_test_runner_tests PRIVATE shiplua)
target_compile_definitions(mod_test_runner_tests PRIVATE
    SHIPLUA_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME mod_test_runner_tests COMMAND mod_test_runner_tests)

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

add_executable(api_contract_tests contract/ApiContractTests.cpp)
target_link_libraries(api_contract_tests PRIVATE shiplua)
target_compile_definitions(api_contract_tests PRIVATE
    SHIPLUA_GENERATED_DIR="${CMAKE_SOURCE_DIR}/generated")

add_test(NAME api_contract_tests COMMAND api_contract_tests)

add_executable(mock_runtime_tests contract/MockRuntimeTests.cpp)
target_link_libraries(mock_runtime_tests PRIVATE shiplua lua_static)
target_compile_definitions(mock_runtime_tests PRIVATE
    SHIPLUA_GENERATED_DIR="${CMAKE_SOURCE_DIR}/generated")

add_test(NAME mock_runtime_tests COMMAND mock_runtime_tests)

add_test(NAME api_contract_codegen_check
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_api_contracts.py" --check)
add_test(NAME api_contract_codegen_tests
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/schema/test_generate_api_contracts.py")

# ---------------------------------------------------------------------------
# MODSDK-004 — mod test runner (mock runtime) via CLI
# ---------------------------------------------------------------------------
add_test(NAME mod_runner_example
    COMMAND shiplua_mod_test_runner "${CMAKE_SOURCE_DIR}/examples/frame-counter")
add_test(NAME mod_runner_example_mm
    COMMAND shiplua_mod_test_runner "${CMAKE_SOURCE_DIR}/examples/frame-counter" --game mm)
add_test(NAME mod_runner_fixture_passing
    COMMAND shiplua_mod_test_runner "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mod-runner/passing-mod")
add_test(NAME mod_runner_fixture_failing
    COMMAND shiplua_mod_test_runner "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mod-runner/failing-mod")
set_tests_properties(mod_runner_fixture_failing PROPERTIES WILL_FAIL TRUE)
add_test(NAME mod_runner_fixture_no_tests
    COMMAND shiplua_mod_test_runner "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mod-runner/no-tests-mod")
add_test(NAME mod_runner_package
    COMMAND shiplua_mod_test_runner "${SHIPLUA_HELLO_WORLD_PACKAGE}")
add_test(NAME mod_runner_usage COMMAND shiplua_mod_test_runner)
set_tests_properties(mod_runner_usage PROPERTIES WILL_FAIL TRUE)
