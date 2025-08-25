#!/bin/bash

echo "🎵 Building ChordLock WebAssembly Module..."
echo "=========================================="

# Check if emscripten is installed
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten (emcc) is not installed."
    echo ""
    echo "Please install Emscripten first:"
    echo "1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "2. cd emsdk"
    echo "3. ./emsdk install latest"
    echo "4. ./emsdk activate latest"
    echo "5. source ./emsdk_env.sh"
    exit 1
fi

# Create build directory
mkdir -p build_wasm
cd build_wasm

# Configure with CMake for Emscripten
echo ""
echo "📦 Configuring with CMake..."
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

# Build
echo ""
echo "🔨 Building WASM module..."
emmake make -j8

# Check if build was successful
if [ -f "chordlock_wasm.js" ] && [ -f "chordlock_wasm.wasm" ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "📁 Output files:"
    ls -lh chordlock_wasm.*
    echo ""
    echo "🚀 To run the demo:"
    echo "   1. cd build_wasm"
    echo "   2. python3 -m http.server 8000"
    echo "   3. Open http://localhost:8000 in your browser"
    echo ""
    echo "📦 Files needed for deployment:"
    echo "   - index.html"
    echo "   - style.css"
    echo "   - app.js"
    echo "   - chordlock_wasm.js"
    echo "   - chordlock_wasm.wasm"
else
    echo ""
    echo "❌ Build failed. Please check the error messages above."
    exit 1
fi