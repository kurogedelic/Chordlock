cmake_minimum_required(VERSION 3.15)

# Find Google Benchmark
find_package(benchmark REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Benchmark source files
set(BENCHMARK_SOURCES
    chord_identification_benchmark.cpp
)

# Create benchmark executable
add_executable(chord_benchmark ${BENCHMARK_SOURCES})

# Link libraries
target_link_libraries(chord_benchmark
    chordlock_core
    benchmark::benchmark
    benchmark::benchmark_main
    pthread
)

# No external YAML dependency when using compiled tables

# Compiler flags
target_compile_features(chord_benchmark PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(chord_benchmark PRIVATE 
        -O3 -DNDEBUG -march=native
    )
endif()

# Copy test data
configure_file(${CMAKE_SOURCE_DIR}/interval_dict.yaml
               ${CMAKE_CURRENT_BINARY_DIR}/interval_dict.yaml COPYONLY)