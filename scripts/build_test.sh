#!/bin/bash

# Build test program
echo "Building Chordlock test program..."

clang++ -std=c++17 -O2 -Wall -o build/test_chordlock src/test_cli.cpp src/Chordlock.cpp

if [ $? -eq 0 ]; then
    echo "Build successful! Run ./build/test_chordlock to test"
else
    echo "Build failed!"
    exit 1
fi