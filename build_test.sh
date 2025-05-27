#!/bin/bash

# Build test program
echo "Building Chordlock test program..."

clang++ -std=c++17 -O2 -Wall -o test_chordlock test_cli.cpp Chordlock.cpp

if [ $? -eq 0 ]; then
    echo "Build successful! Run ./test_chordlock to test"
else
    echo "Build failed!"
    exit 1
fi