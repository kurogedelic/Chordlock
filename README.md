# Chordlock v3

**Real-time chord detection engine built with modern C++17**

A high-performance chord detection library that supports both native C++ applications and web browsers via WebAssembly. Designed for real-time MIDI input and audio analysis with advanced harmony recognition.

## Features

🎯 **Comprehensive Chord Recognition**
- **339 chord types** supported: Major/Minor/7th → Jazz/Hybrid harmonies
- **Slash chord detection** with intelligent bass note analysis
- **Multi-candidate analysis** with confidence scoring
- **Single note recognition** for individual pitches

⚡ **High Performance**
- **Sub-millisecond detection** optimized for real-time performance
- **SIMD-accelerated** bit operations for maximum speed
- **Memory efficient** hash table design
- **Zero external dependencies**

🌐 **Cross-Platform Support**
- **Native C++17** implementation with static/shared libraries
- **WebAssembly** build for browser compatibility
- **Interactive web UI** with virtual piano and MIDI support
- **Command-line interface** for direct integration

## Quick Start

### Web Demo
```bash
python3 -m http.server 8000 --directory web
# Open http://localhost:8000 in browser
```

### C++ Usage
```cpp
#include "Chordlock.hpp"

Chordlock detector;
detector.noteOn(60, 80);  // C
detector.noteOn(64, 80);  // E  
detector.noteOn(67, 80);  // G

auto result = detector.detectChord();
std::cout << result.chordName << std::endl;  // "C"
```

### Build
```bash
./scripts/build_all.sh
# Creates: native libraries, WebAssembly modules, distribution package
```

## Supported Chords

From basic triads to advanced jazz harmonies:
- **Triads**: Major, Minor, Diminished, Augmented
- **Extensions**: 7th, 9th, 11th, 13th chords
- **Jazz**: Dominant alterations (b5, #5, b9, #9, #11, b13)
- **Advanced**: Half-diminished, Suspended, Add chords, Polytonal

## Input Sources

- **MIDI devices** via Web MIDI API or native MIDI
- **Manual note input** through virtual piano interface
- **Direct note arrays** for programmatic analysis
- **Audio analysis** integration ready (FFT → note detection → chord analysis)

## Architecture

**C++ Core**
```
├── Enhanced Hash Lookup Engine    # 339-chord database with rotational matching
├── Velocity Processor            # Dynamic note weighting and timing
├── SIMD Utilities               # Hardware-optimized bit operations  
└── WebAssembly Bindings         # Browser compatibility layer
```

**Performance**: Native ~0.001ms, WebAssembly ~0.002ms per detection

## API Reference

### Core Methods
```cpp
// Note input
void noteOn(int midiNote, int velocity = 80);
void noteOff(int midiNote);

// Detection  
ChordResult detectChord();
std::string detectChordDetailed(int maxResults = 5);

// Configuration
void setSlashChordDetection(bool enabled);
void setConfidenceThreshold(float threshold);
```

### WebAssembly
```javascript
chordlock_init();
chordlock_note_on(60, 80);
const result = chordlock_detect_chord();
```

## Project Structure

```
src/
├── Chordlock.{hpp,cpp}              # Main detection class
├── chordlock_wasm.cpp               # WebAssembly bindings  
├── engines/EnhancedHashLookupEngine # Core algorithm
├── processors/VelocityProcessor     # MIDI processing
└── enhanced_chord_hash_table.hpp    # 339-chord database

web/
├── index.html                       # Interactive demo
├── styles.css                       # UI styling
└── chordlock.{js,wasm}             # WebAssembly build
```

## Requirements

- **C++17** compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **Emscripten** for WebAssembly builds
- Modern browser with Web MIDI API support

## License

MIT License

## Author

[@kurogedelic](https://github.com/kurogedelic)

---

*Built for musicians, developers, and music technology enthusiasts who demand accuracy and performance.*