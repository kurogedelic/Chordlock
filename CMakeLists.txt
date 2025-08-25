cmake_minimum_required(VERSION 3.15)
project(ChordLock VERSION 1.0.0 LANGUAGES CXX)

# License information
set(PROJECT_LICENSE "LGPL-3.0")
set(PROJECT_DESCRIPTION "Ultra-fast chord identification engine")
set(PROJECT_HOMEPAGE_URL "https://github.com/yourusername/ChordLock")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    
    # Enable SIMD optimizations
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -msse4.1")
    endif()
    
    # Warning flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /arch:AVX2")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /DDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find dependencies (yaml-cpp is now optional for compiled mode)
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(YAMLCPP yaml-cpp)
    endif()
endif()

# Force use of compiled tables for now (no external YAML dependency)
message(STATUS "Using compiled chord tables for maximum performance")
add_definitions(-DUSE_COMPILED_TABLES)
set(USE_COMPILED_TABLES ON)

# Enable advanced optimization features
option(USE_ROBIN_HOOD "Enable Robin Hood hashing for 23-66% speedup" ON)
option(USE_PERFECT_HASH "Enable Perfect Minimal Hash Function for 37ns lookups" ON)
option(USE_CACHE_OBLIVIOUS "Enable Cache-Oblivious B-tree for optimal memory hierarchy" ON)

if(USE_ROBIN_HOOD)
    add_definitions(-DUSE_ROBIN_HOOD)
    message(STATUS "Robin Hood hashing enabled")
endif()

if(USE_PERFECT_HASH)
    add_definitions(-DUSE_PERFECT_HASH)
    message(STATUS "Perfect Minimal Hash Function enabled")
endif()

if(USE_CACHE_OBLIVIOUS)
    add_definitions(-DUSE_CACHE_OBLIVIOUS)
    message(STATUS "Cache-Oblivious B-tree enabled")
endif()

# Optional dependencies
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmark tests" ON)
option(BUILD_CLI "Build command line interface" ON)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

if(BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${YAMLCPP_INCLUDE_DIRS})

# Core library source files
set(CHORDLOCK_CORE_SOURCES
    Core/IntervalEngine.cpp
    Core/ChordDatabase.cpp
    Core/ChordDatabaseOptimized.cpp
    Core/ChordIdentifier.cpp
    Core/ChordNameGenerator.cpp
    Core/ProgressionAnalyzer.cpp
    Core/ErrorHandling.cpp
    Analysis/InversionDetector.cpp
    Utils/NoteConverter.cpp
    Utils/OutputFormatter.cpp
    Utils/MemoryTracker.cpp
)

# Core library headers
set(CHORDLOCK_CORE_HEADERS
    Core/PerformanceStrategy.h
    Core/IntervalEngine.h
    Core/ChordDatabase.h
    Core/ChordIdentifier.h
    Core/ChordNameGenerator.h
    Core/ProgressionAnalyzer.h
    Core/ErrorHandling.h
    Analysis/InversionDetector.h
    Utils/NoteConverter.h
    Utils/OutputFormatter.h
    Utils/MemoryTracker.h
)

# Create main library
add_library(chordlock_core STATIC ${CHORDLOCK_CORE_SOURCES} ${CHORDLOCK_CORE_HEADERS})

# No external dependencies when using compiled tables

# Set library properties
set_target_properties(chordlock_core PROPERTIES
    OUTPUT_NAME chordlock
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Install library
install(TARGETS chordlock_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES ${CHORDLOCK_CORE_HEADERS} DESTINATION include/chordlock)

# Build CLI application
if(BUILD_CLI)
    add_executable(chord_cli CLI/main.cpp)
    target_link_libraries(chord_cli chordlock_core)
    
    install(TARGETS chord_cli DESTINATION bin)
endif()

# Build tests
if(BUILD_TESTS)
    add_subdirectory(Tests/unit_tests)
    add_subdirectory(Tests/integration_tests)
    
    # Add performance consistency test
    add_executable(performance_test Tests/performance_consistency_test.cpp)
    target_link_libraries(performance_test chordlock_core)
    target_compile_features(performance_test PRIVATE cxx_std_17)
    
    # Add memory usage test
    add_executable(memory_usage_test memory_usage_test.cpp)
    target_link_libraries(memory_usage_test chordlock_core)
    target_compile_features(memory_usage_test PRIVATE cxx_std_17)
endif()

# Build benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(Tests/benchmark_tests)
endif()

# Copy data files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/interval_dict.yaml
               ${CMAKE_CURRENT_BINARY_DIR}/interval_dict.yaml COPYONLY)

# Create pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/chordlock.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/chordlock.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/chordlock.pc"
        DESTINATION lib/pkgconfig)

# Print configuration summary
message(STATUS "")
message(STATUS "ChordLock Configuration Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "  Build CLI: ${BUILD_CLI}")
message(STATUS "")