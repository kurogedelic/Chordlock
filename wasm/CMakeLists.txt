cmake_minimum_required(VERSION 3.15)
project(ChordLockWASM)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emscripten specific settings
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    
    # Core source files (without YAML dependency)
    set(CORE_SOURCES
        ../Core/ChordIdentifier.cpp
        ../Core/ChordDatabase.cpp
        ../Core/IntervalEngine.cpp
        ../Core/ChordNameGenerator.cpp
        ../Core/ErrorHandling.cpp
        ../Core/AdvancedChordRecognition.cpp
        ../Analysis/InversionDetector.cpp
        ../Utils/NoteConverter.cpp
        ../Utils/OutputFormatter.cpp
        ../Utils/MemoryTracker.cpp
    )
    
    # WASM binding
    add_executable(chordlock_wasm
        wasm_binding.cpp
        ${CORE_SOURCES}
    )
    
    # Compiler flags for optimization and WASM features
    target_compile_options(chordlock_wasm PRIVATE
        -O3
        -fno-exceptions
        -fno-rtti
        -DWASM_BUILD
        -DUSE_COMPILED_TABLES
    )
    
    # Linker flags for WASM
    set_target_properties(chordlock_wasm PROPERTIES
        LINK_FLAGS "-O3 \
            --bind \
            -s WASM=1 \
            -s MODULARIZE=1 \
            -s EXPORT_NAME='ChordLockModule' \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s MAXIMUM_MEMORY=67108864 \
            -s INITIAL_MEMORY=4194304 \
            -s NO_EXIT_RUNTIME=1 \
            -s ENVIRONMENT='web' \
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
            -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
            --no-entry"
    )
    
    # Include directories
    target_include_directories(chordlock_wasm PRIVATE
        ${CMAKE_SOURCE_DIR}/..
    )
endif()