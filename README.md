# 🎵 Chordlock v2.0 - Advanced Key-Context Chord Detection

**Revolutionary MIDI chord detection engine with functional harmony analysis**

Chordlock is an intelligent MIDI chord detection library that provides **key-context aware** chord analysis with support for complex harmonies, slash chords, and functional harmony theory. Unlike traditional chord detection systems, Chordlock considers the musical key to provide contextually appropriate chord interpretations.

## 🎯 Key Features

### 🔥 **World-First Key-Context Analysis**
- **Functional Harmony Priority**: Tonic/Dominant/Subdominant functions get priority boosts
- **Key-Dependent Interpretation**: Same notes = different chords based on musical key
- **Example**: `E-C-E-G-D` → `C(add9)` in C major, `Em7(add♭6)` in E minor

### 🎼 **Advanced Chord Detection**
- **Complex Chords**: 7th, 9th, 11th, 13th extensions
- **Slash Chords**: Intelligent bass note analysis (`C/E`, `F/G`)
- **Altered Chords**: `G7#5b9`, `Dm7b5`, `C7alt`
- **Polychords**: Advanced harmonic structures

### 🎹 **Multiple Interfaces**
- **CLI**: Command-line batch processing and testing
- **WebAssembly**: Browser integration for web applications
- **Web App**: Interactive piano interface with real-time analysis

### ⚡ **High Performance**
- **Sub-millisecond detection**: Optimized C++ engine
- **Real-time processing**: Perfect for live performance
- **Bitmasking optimization**: Efficient pattern matching

## 🚀 Quick Start

### CLI Usage
```bash
# Build the CLI version
./scripts/build_cli.sh

# Detect chord with key context
./build/chordlock -N 60,64,67 -k 0    # C major triad in C major
./build/chordlock -N 60,64,67 -k 16   # C major triad in E minor

# Batch process MIDI file
./build/chordlock -f input.mid -k 0
```

### Web Integration
```html
<script src="chordlock.js"></script>
<script>
ChordlockModule().then(function(Module) {
    Module.ccall('chordlock_init', null, [], []);
    Module.ccall('chordlock_set_key', null, ['number'], [0]); // C major
    
    // Detect chord
    Module.ccall('chordlock_note_on', null, ['number', 'number'], [60, 80]); // C
    Module.ccall('chordlock_note_on', null, ['number', 'number'], [64, 80]); // E
    Module.ccall('chordlock_note_on', null, ['number', 'number'], [67, 80]); // G
    
    const result = Module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [5]);
    console.log(JSON.parse(result));
});
</script>
```

### 🤖 AI Integration (MCP)

Chordlock now supports **Model Context Protocol** for music analysis:

```bash
# Install MCP package
npm install -g @kurogedelic/chordlock-mcp

# Configure Claude Desktop/Code
{
  "mcpServers": {
    "chordlock": {
      "command": "npx",
      "args": ["@kurogedelic/chordlock-mcp"]
    }
  }
}
```

**AI Use Cases:**
- *"What MIDI notes make up a Dm7b5 chord?"*
- *"Generate a jazz ii-V-I progression in Bb major"*
- *"Analyze this chord progression: C - Am - F - G"*

## API Reference

### C++ Core
```cpp
#include "Chordlock.hpp"

Chordlock detector;

// Chord detection
detector.noteOn(60, 80);  // C
detector.noteOn(64, 80);  // E
detector.noteOn(67, 80);  // G
auto result = detector.detectChord();
// result.chordName = "C"

// Chord generation
auto notes = detector.chordNameToNotes("Cmaj7", 4);
// notes = [48, 52, 55, 59]
```

### WebAssembly
```javascript
// Initialize
chordlock_init();

// Detect chord
chordlock_note_on(60, 80);
chordlock_note_on(64, 80);
chordlock_note_on(67, 80);
const chord = chordlock_detect_chord(); // "C"

// Generate notes
const json = chordlock_chord_name_to_notes_json("Cmaj7", 4);
const data = JSON.parse(json);
// data.notes = [48, 52, 55, 59]
```

## Supported Chords

**339+ chord types** including:
- **Basic**: Major, Minor, Diminished, Augmented
- **Extended**: 7th, 9th, 11th, 13th
- **Jazz**: Altered dominants, half-diminished, sus chords
- **Advanced**: Add chords, slash chords, polytonal harmony

## Architecture

```
Core Engine (C++17)     →  WebAssembly Build  →  Web Applications
     ↓                           ↓                      ↓
CLI Application         →  MCP Server        →  AI Integration
```

**Performance**: Native ~0.001ms, WebAssembly ~0.002ms per detection

## Requirements

- **C++17** compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **Emscripten** for WebAssembly builds
- **Node.js 16+** for MCP integration

---

*Built for musicians, developers, and enthusiasts who demand accurate and fast music analysis.*

## License

MIT License

## Author

**Leo Kuroshita** ([@kurogedelic](https://github.com/kurogedelic))

© 2024-2025 Leo Kuroshita. All rights reserved.