add_executable(runtime_tests unit/RuntimeTests.cpp)
target_link_libraries(runtime_tests PRIVATE shiplua)

add_test(NAME runtime_tests COMMAND runtime_tests)
