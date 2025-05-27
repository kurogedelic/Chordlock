#!/bin/bash

# u30b3u30f3u30d1u30a4u30ebu30b9u30afu30eau30d7u30c8
emcc -v -I. Chordlock.cpp \
     -o chordlock.js \
     -s MODULARIZE=1 \
     -s EXPORT_NAME="createChordlockModule" \
     -s EXPORTED_FUNCTIONS="['_noteOn','_noteOff','_detect','_detectChord','_detectWithConfidence','_detectAlternatives','_detectExtended','_getChordRootAndBass','_setVelocity','_setVelocitySensitivity','_setOnChordDetection','_setKey','_reset','_malloc','_free']" \
     -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString','getValue']"
