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
