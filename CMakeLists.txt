cmake_minimum_required(VERSION 3.15)
project(ChordLock VERSION 2.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Use compiled tables by default
add_definitions(-DUSE_COMPILED_TABLES)

# Include directories
include_directories(.)

# Core sources
set(CHORDLOCK_CORE_SOURCES
    Core/IntervalEngine.cpp
    Core/ChordDatabase.cpp
    Core/ChordIdentifier.cpp
    Core/ChordNameGenerator.cpp
    Analysis/InversionDetector.cpp
    Utils/NoteConverter.cpp
)

# Create static library
add_library(chordlock_static STATIC ${CHORDLOCK_CORE_SOURCES})
target_include_directories(chordlock_static PUBLIC .)

# Simple CLI example
add_executable(chordlock_cli Examples/cli_example.cpp)
target_link_libraries(chordlock_cli chordlock_static)

# Install
install(TARGETS chordlock_static chordlock_cli
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY Core/ Utils/ Analysis/ Data/
    DESTINATION include/ChordLock
    FILES_MATCHING PATTERN "*.h"
)