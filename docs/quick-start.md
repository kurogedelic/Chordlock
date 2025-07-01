# Quick Start

Get up and running with Chordlock in minutes.

## Installation

### Prerequisites

- **C++17** compiler (GCC 8+, Clang 7+)
- **Emscripten** (for WebAssembly builds)  
- **Node.js 16+** (for MCP integration)

### Build from Source

```bash
git clone https://github.com/kurogedelic/Chordlock.git
cd Chordlock

# Build CLI
g++ -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
    -I./src/processors -I./src/utils src/cli_main.cpp src/Chordlock.cpp \
    src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
    -o build/chordlock -framework CoreMIDI -framework CoreFoundation

# Build WebAssembly
emcc -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
     -I./src/processors -I./src/utils src/chordlock_wasm.cpp src/Chordlock.cpp \
     src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
     -o web_app/chordlock.js -s EXPORTED_RUNTIME_METHODS='["ccall"]'
```

### MCP Installation

```bash
npm install -g @kurogedelic/chordlock-mcp
```

## Basic Usage

### CLI Interface

```bash
# Detect chord from MIDI notes with key context
./build/chordlock -N 60,64,67 -k C
# → C (confidence: 21.87)
# → Roman Numeral: I (in C major)

# Generate chord from Roman numeral
./build/chordlock -d "V7" -k C  
# → G7 [55, 59, 62, 65]

# Convert chord name to MIDI notes
./build/chordlock -c "Cmaj7"
# → [48, 52, 55, 59]
```

### C++ Library

```cpp
#include "Chordlock.hpp"

int main() {
    Chordlock detector;
    
    // Set key context
    detector.setKeyContext(0, false); // C major
    
    // Input notes
    detector.noteOn(60, 80); // C
    detector.noteOn(64, 80); // E
    detector.noteOn(67, 80); // G
    
    // Detect chord
    auto result = detector.detectChord();
    std::cout << "Chord: " << result.chordName << std::endl;
    
    // Analyze Roman numeral
    auto degree = detector.analyzeCurrentNotesToDegree(0, false);
    std::cout << "Degree: " << degree << std::endl;
    
    return 0;
}
```

### WebAssembly

```html
<script src="chordlock.js"></script>
<script>
ChordlockModule().then(function(Module) {
    // Initialize
    Module.ccall('chordlock_init', null, [], []);
    Module.ccall('chordlock_set_key', null, ['number'], [0]); // C major
    
    // Input notes
    Module.ccall('chordlock_note_on', null, ['number', 'number'], [60, 80]);
    Module.ccall('chordlock_note_on', null, ['number', 'number'], [64, 80]);
    Module.ccall('chordlock_note_on', null, ['number', 'number'], [67, 80]);
    
    // Detect chord
    const result = Module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [5]);
    const data = JSON.parse(result);
    console.log('Chord:', data.chord);
});
</script>
```

### MCP Integration

Configure in Claude Desktop/Code:

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

Then use with AI:

> "What MIDI notes make up a Dm7b5 chord?"
> "Generate a jazz ii-V-I progression in Bb major"
> "Analyze this chord progression: C - Am - F - G"

## Key Features Demo

### Key-Context Analysis

```bash
# Same notes, different interpretations
./build/chordlock -N 52,60,64,67,74 -k C   # → C/E [I]
./build/chordlock -N 52,60,64,67,74 -k Em  # → Em7(add♭6) [i7]
```

### Roman Numeral Analysis

```bash
./build/chordlock -N 55,59,62,65 -k C      # → G7 [V7]
./build/chordlock -N 57,60,64 -k C         # → Am [vi]  
./build/chordlock -N 53,57,60,64 -k C      # → F [IV]
```

### Degree Generation

```bash
./build/chordlock -d "ii7" -k C    # → Dm7 [50, 53, 57, 60]
./build/chordlock -d "V9" -k Am    # → E9 [52, 56, 59, 62, 66]
./build/chordlock -d "bVII" -k Gm  # → F [53, 57, 60]
```

## Next Steps

- Explore the [API Reference](/api/) for detailed documentation
- Try the [Interactive Demo](../web_app/index.html)
- Learn about [Key Context Analysis](/features/key-context)
- See [Advanced Examples](/examples/advanced)