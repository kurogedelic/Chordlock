#!/bin/bash

# Build script for Chordlock CLI

echo "Building Chordlock CLI..."

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -Wall"
LDFLAGS=""

# Platform specific flags
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Building for macOS..."
    LDFLAGS="-framework CoreMIDI -framework CoreFoundation"
else
    echo "Warning: MIDI support not implemented for this platform"
fi

# Build command
$CXX $CXXFLAGS -o chordlock_cli cli_main.cpp Chordlock.cpp $LDFLAGS

if [ $? -eq 0 ]; then
    echo "Build successful! Run ./chordlock_cli to start"
    echo ""
    echo "Usage:"
    echo "  ./chordlock_cli           - Run with default settings"
    echo "  ./chordlock_cli -n        - Disable slash chord detection"
    echo "  ./chordlock_cli -v        - Enable velocity sensitivity"
    echo "  ./chordlock_cli -h        - Show help"
else
    echo "Build failed!"
    exit 1
fi