# Installation

Get Chordlock running on your system.

## Prerequisites

- **C++17** compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **Emscripten** (for WebAssembly builds)
- **Node.js 16+** (for MCP integration)

## CLI Installation

### macOS/Linux

```bash
git clone https://github.com/kurogedelic/Chordlock.git
cd Chordlock

# Build CLI
g++ -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
    -I./src/processors -I./src/utils src/cli_main.cpp src/Chordlock.cpp \
    src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
    -o build/chordlock -framework CoreMIDI -framework CoreFoundation

# Test installation
./build/chordlock -N 60,64,67 -k C
```

### Windows

```bash
git clone https://github.com/kurogedelic/Chordlock.git
cd Chordlock

# Build with MSVC
cl /std:c++17 /O2 /I.\src /I.\src\engines /I.\src\interfaces \
   /I.\src\processors /I.\src\utils src\cli_main.cpp src\Chordlock.cpp \
   src\engines\EnhancedHashLookupEngine.cpp src\processors\VelocityProcessor.cpp \
   /Fe:build\chordlock.exe

# Test installation
.\build\chordlock.exe -N 60,64,67 -k C
```

## WebAssembly Build

```bash
# Install Emscripten first
# https://emscripten.org/docs/getting_started/downloads.html

# Build WebAssembly module
emcc -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
     -I./src/processors -I./src/utils src/chordlock_wasm.cpp src/Chordlock.cpp \
     src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
     -o web_app/chordlock.js -s EXPORTED_RUNTIME_METHODS='["ccall"]' \
     -s MODULARIZE=1 -s EXPORT_NAME="ChordlockModule"
```

## MCP Integration

### Install MCP Server

```bash
npm install -g @kurogedelic/chordlock-mcp
```

### Configure Claude Desktop

Add to your Claude Desktop configuration:

**macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
**Windows**: `%APPDATA%\Claude\claude_desktop_config.json`

```json
{
  "mcpServers": {
    "chordlock": {
      "command": "npx",
      "args": ["@kurogedelic/chordlock-mcp"]
    }
  }
}
```

### Configure Claude Code

Add to your Claude Code MCP settings:

```json
{
  "mcpServers": {
    "chordlock": {
      "command": "node",
      "args": ["/path/to/Chordlock/chordlock-mcp/dist/index.js"]
    }
  }
}
```

## Docker Installation

```dockerfile
FROM emscripten/emsdk:latest

WORKDIR /chordlock
COPY . .

# Build CLI
RUN g++ -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
    -I./src/processors -I./src/utils src/cli_main.cpp src/Chordlock.cpp \
    src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
    -o build/chordlock

# Build WebAssembly  
RUN emcc -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
    -I./src/processors -I./src/utils src/chordlock_wasm.cpp src/Chordlock.cpp \
    src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
    -o web_app/chordlock.js -s EXPORTED_RUNTIME_METHODS='["ccall"]' \
    -s MODULARIZE=1 -s EXPORT_NAME="ChordlockModule"

EXPOSE 3000
CMD ["./build/chordlock", "-h"]
```

## Verification

Test your installation:

```bash
# CLI functionality
./build/chordlock -N 60,64,67 -k C
# Expected: C (confidence: 21.87) [I]

# Key context analysis  
./build/chordlock -N 52,60,64,67,74 -k C
./build/chordlock -N 52,60,64,67,74 -k Em
# Should show different chord interpretations

# Degree generation
./build/chordlock -d "V7" -k C
# Expected: G7 [55, 59, 62, 65]

# WebAssembly (open in browser)
open web_app/index.html
```

## Troubleshooting

### Common Issues

**Compilation errors**:
- Ensure C++17 support: `g++ --version`
- Check include paths are correct
- On macOS, install Xcode command line tools

**WebAssembly build fails**:
- Install/activate Emscripten: `source /path/to/emsdk/emsdk_env.sh`
- Clear cache: `emcc --clear-cache`

**MCP not working**:
- Restart Claude Desktop/Code after config changes
- Check Node.js version: `node --version`
- Verify MCP server: `npx @kurogedelic/chordlock-mcp`

**MIDI not working (macOS)**:
- Grant MIDI permissions in System Preferences
- Check Framework linking: `-framework CoreMIDI`

### Platform-Specific Notes

**macOS**: 
- Core MIDI framework required for MIDI input
- May need to grant microphone permissions

**Linux**:
- Install ALSA development headers: `sudo apt-get install libasound2-dev`
- MIDI support varies by distribution

**Windows**:
- Use Visual Studio 2019+ or MinGW-w64
- MIDI support requires Windows SDK

## Next Steps

- Try the [Quick Start Guide](/quick-start)
- Explore the [API Reference](/api/)
- Open the [Interactive Demo](../web_app/index.html)
- Learn about [Key Context Analysis](/features/key-context)