# Integration Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Find required packages
find_package(GTest REQUIRED)

# Integration test executable
add_executable(integration_tests
    test_complete_pipeline.cpp
)

# Link libraries
target_link_libraries(integration_tests 
    chordlock_core 
    GTest::gtest 
    GTest::gtest_main
)

# Set C++17 standard
target_compile_features(integration_tests PRIVATE cxx_std_17)

# Add test to CTest
add_test(NAME IntegrationTests COMMAND integration_tests)

# Set test properties
set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 300  # 5 minutes timeout
    LABELS "integration"
)

# Add memory tracking test
add_executable(memory_integration_test
    test_memory_integration.cpp
)

target_link_libraries(memory_integration_test 
    chordlock_core 
    GTest::gtest 
    GTest::gtest_main
)

target_compile_features(memory_integration_test PRIVATE cxx_std_17)
add_test(NAME MemoryIntegrationTest COMMAND memory_integration_test)

# Add performance integration test
add_executable(performance_integration_test
    test_performance_integration.cpp
)

target_link_libraries(performance_integration_test 
    chordlock_core 
    GTest::gtest 
    GTest::gtest_main
)

target_compile_features(performance_integration_test PRIVATE cxx_std_17)
add_test(NAME PerformanceIntegrationTest COMMAND performance_integration_test)

# Test data
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_data/integration_test_chords.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/integration_test_chords.yaml
    COPYONLY
)

# Enable verbose test output
if(VERBOSE_TESTS)
    set_tests_properties(IntegrationTests PROPERTIES ENVIRONMENT "GTEST_COLOR=1")
    set_tests_properties(MemoryIntegrationTest PROPERTIES ENVIRONMENT "GTEST_COLOR=1")
    set_tests_properties(PerformanceIntegrationTest PROPERTIES ENVIRONMENT "GTEST_COLOR=1")
endif()