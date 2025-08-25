cmake_minimum_required(VERSION 3.15)

# Find Google Test
find_package(GTest REQUIRED)
include(GoogleTest)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${GTEST_INCLUDE_DIRS})

# Test source files
set(TEST_SOURCES
    test_interval_engine.cpp
    test_chord_database.cpp
    test_chord_identifier.cpp
    test_inversion_detector.cpp
    test_note_converter.cpp
    test_performance.cpp
)

# Create test executable
add_executable(chordlock_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(chordlock_tests
    chordlock_core
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# No external YAML dependency when using compiled tables

# Compiler flags for tests
target_compile_features(chordlock_tests PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(chordlock_tests PRIVATE 
        -Wall -Wextra -Wpedantic
        -g -O0  # Debug build for tests
    )
endif()

# Discover tests
gtest_discover_tests(chordlock_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Copy test data files
configure_file(${CMAKE_SOURCE_DIR}/interval_dict.yaml
               ${CMAKE_CURRENT_BINARY_DIR}/interval_dict.yaml COPYONLY)

# Custom test data for testing
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_data/test_chords.yaml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_data/test_chords.yaml
                   ${CMAKE_CURRENT_BINARY_DIR}/test_chords.yaml COPYONLY)
endif()

# Copy comprehensive test set
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_data/chord_test_set.yaml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_data/chord_test_set.yaml
                   ${CMAKE_CURRENT_BINARY_DIR}/chord_test_set.yaml COPYONLY)
endif()

# Define test data directory path for tests
add_compile_definitions(TEST_DATA_DIR="${CMAKE_CURRENT_BINARY_DIR}")

# Create test data directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_data)