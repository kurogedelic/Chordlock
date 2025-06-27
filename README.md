# [Chordlock](https://kurogedelic.github.io/Chordlock/)

**High-performance chord detection and generation engine for real-time music analysis**

A modern C++17 chord detection library with WebAssembly support and AI integration through Model Context Protocol (MCP).

## Features

🎯 **Bidirectional Processing**
- **Chord Detection**: MIDI notes → chord names with confidence scoring
- **Chord Generation**: Chord names → MIDI notes with multiple voicings
- **339 chord types** supported: triads to advanced jazz harmony

🎵 **Advanced Analysis**
- Real-time detection with sub-millisecond performance
- Slash chord detection and inversion analysis
- Intelligent chord name normalization (CM7 ↔ Cmaj7)
- Alternative voicing suggestions

⚡ **Cross-Platform**
- **C++ Library**: Native performance for desktop applications
- **WebAssembly**: Browser compatibility with interactive demos
- **MCP Integration**: Music analysis for Claude Code/Desktop

## Quick Start

### 🚀 Try the Web Demo
- **[Interactive Piano](https://kurogedelic.github.io/Chordlock/)** - Real-time chord detection
- **[Chord Input Tool](https://kurogedelic.github.io/Chordlock/chord-input.html)** - Name to notes conversion

### 🔧 Build Locally
```bash
git clone https://github.com/kurogedelic/Chordlock.git
cd Chordlock
./scripts/build_all.sh
```

### 🎹 Command Line Usage
```bash
# Detect chord from MIDI notes
./chordlock_cli -N 60,64,67,71  # → Cmaj7

# Convert chord name to MIDI notes
./chordlock_cli -c "Dm7"        # → [50, 53, 57, 60]
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