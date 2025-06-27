# [Chordlock](https://kurogedelic.github.io/Chordlock/)

**Bidirectional chord detection and generation engine built with modern C++17**

A high-performance chord detection library that supports both native C++ applications and web browsers via WebAssembly. Features both chord detection (MIDI → chord name) and reverse lookup (chord name → MIDI notes) for complete music analysis and generation workflows.

## Features

🎯 **Bidirectional Chord Processing**
- **Forward Detection**: MIDI notes → chord names with confidence scoring
- **Reverse Lookup**: Chord names → MIDI note arrays with alternative voicings
- **339 chord types** supported: Major/Minor/7th → Jazz/Hybrid harmonies
- **Intelligent fallback** for ambiguous chord name inputs (CM7, Cmaj7, C Major 7)
- **Enharmonic equivalents** automatically handled (F# ↔ Gb)

🎵 **Advanced Harmony Features**
- **Slash chord detection** with intelligent bass note analysis
- **Multi-candidate analysis** with confidence scoring
- **Single note recognition** for individual pitches
- **Duplicate elimination** for clean results
- **Alternative voicings** for composition workflows

⚡ **High Performance**
- **Sub-millisecond detection** optimized for real-time performance
- **SIMD-accelerated** bit operations for maximum speed
- **Memory efficient** hash table design
- **Zero external dependencies**

🌐 **Cross-Platform Support**
- **Native C++17** implementation with static/shared libraries
- **WebAssembly** build for browser compatibility
- **Interactive web UI** with virtual piano and MIDI support
- **Command-line interface** for direct integration and reverse lookup

## Quick Start

### Web Demo
```bash
python3 -m http.server 8000 --directory web
# Open http://localhost:8000 in browser
```

**🚀 Quick Test** - Try the live web demo:
- **[Piano Mode](https://kurogedelic.github.io/Chordlock/)**: Interactive chord detection with virtual piano and MIDI input
- **[Chord Input Mode](https://kurogedelic.github.io/Chordlock/chord-input.html)**: Chord name input → visual piano display with note information

**Available Web Applications:**
- **index.html**: Interactive chord detection with virtual piano and MIDI input
- **chord-input.html**: Chord name input → visual piano display with note information

### C++ Usage
```cpp
#include "Chordlock.hpp"

Chordlock detector;

// Forward detection: MIDI notes → chord name
detector.noteOn(60, 80);  // C
detector.noteOn(64, 80);  // E  
detector.noteOn(67, 80);  // G
auto result = detector.detectChord();
std::cout << result.chordName << std::endl;  // "C"

// Reverse lookup: chord name → MIDI notes
auto notes = detector.chordNameToNotes("Cmaj7", 4);
for (int note : notes) {
    std::cout << note << " ";  // 48 52 55 59
}

// Alternative voicings and enharmonic equivalents
auto alternatives = detector.chordNameToNotesWithAlternatives("GM7");
// Returns multiple voicings for G major 7th chord
```

### Build
```bash
./scripts/build_all.sh
# Creates: native libraries, WebAssembly modules, distribution package
```

### Command Line Interface
```bash
# Forward detection: analyze MIDI notes
./chordlock_cli -N 60,64,67,71  # → Cmaj7

# Reverse lookup: chord name to MIDI notes  
./chordlock_cli -c "Gmaj7"      # → [55, 59, 62, 66]
./chordlock_cli -c "CM7"        # → [48, 52, 55, 59] (with fallback)
./chordlock_cli -c "F#m"        # → Alternative suggestions if not found

# File processing
./chordlock_cli -f chord_list.txt
```

## Supported Chords

From basic triads to advanced jazz harmonies:
- **Triads**: Major, Minor, Diminished, Augmented
- **Extensions**: 7th, 9th, 11th, 13th chords
- **Jazz**: Dominant alterations (b5, #5, b9, #9, #11, b13)
- **Advanced**: Half-diminished, Suspended, Add chords, Polytonal

## Use Cases & Input Sources

**Forward Detection (MIDI → Chord Names)**
- **Real-time MIDI analysis** from keyboards, DAWs, live performance
- **Audio transcription** integration (FFT → note detection → chord analysis)
- **Music education** for chord identification learning
- **Harmonic analysis** of existing compositions

**Reverse Lookup (Chord Names → MIDI Notes)**
- **Composition assistance** for chord progression generation
- **Automatic accompaniment** systems and backing tracks
- **Music theory education** showing chord voicings
- **MIDI file generation** from chord charts and lead sheets
- **DAW integration** for quick chord input workflows

**Input Methods**
- **MIDI devices** via Web MIDI API or native MIDI
- **Manual note input** through virtual piano interface
- **Direct note arrays** for programmatic analysis
- **Text-based chord names** with intelligent parsing (CM7, Cmaj7, C Major 7)

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
// Note input for forward detection
void noteOn(int midiNote, int velocity = 80);
void noteOff(int midiNote);
void setChordFromMidiNotes(const std::vector<int>& notes);

// Forward detection: MIDI → chord names
DetectionResult detectChord();
DetectionResult detectChordWithAlternatives(int maxAlternatives = 5);
std::vector<std::string> getAlternativeChords(int maxCount = 3);

// Reverse lookup: chord names → MIDI notes
std::vector<int> chordNameToNotes(const std::string& chordName, int rootOctave = 4);
std::vector<std::vector<int>> chordNameToNotesWithAlternatives(const std::string& chordName, int rootOctave = 4);
std::vector<std::string> findSimilarChordNames(const std::string& input);
std::string chordNameToNotesJSON(const std::string& chordName, int rootOctave = 4);

// Configuration
void setSlashChordDetection(bool enabled);
void setVelocitySensitivity(bool enabled);
void setConfidenceThreshold(float threshold);
void setKeySignature(int keySignature);
```

### WebAssembly
```javascript
// Initialize
chordlock_init();

// Forward detection
chordlock_note_on(60, 80);
chordlock_note_on(64, 80);
chordlock_note_on(67, 80);
const result = chordlock_detect_chord();
console.log(result); // "C"

// Reverse lookup: chord name → MIDI notes
const json = chordlock_chord_name_to_notes_json("Cmaj7", 4);
const data = JSON.parse(json);
console.log(data.notes); // [48, 52, 55, 59]

// Alternative voicings for composition
const alternatives = chordlock_chord_name_to_notes_with_alternatives("GM7", 4);
const altData = JSON.parse(alternatives);
console.log(altData.alternatives); // [[57,61,62,66], ...]

// Find similar chords for ambiguous inputs
const similar = chordlock_find_similar_chord_names("F#m");
const similarData = JSON.parse(similar);
console.log(similarData.similar); // ["Gbm", "F#dim", ...]
```

## Project Structure

```
src/
├── Chordlock.{hpp,cpp}              # Main bidirectional detection class
├── chordlock_wasm.cpp               # WebAssembly bindings  
├── cli_main.cpp                     # Command-line interface with reverse lookup
├── engines/EnhancedHashLookupEngine # Core forward detection algorithm
├── processors/VelocityProcessor     # MIDI processing
└── enhanced_chord_hash_table.hpp    # 339-chord database

web/
├── index.html                       # Interactive chord detection demo
├── chord-input.html                 # Chord name input application
├── styles.css                       # UI styling
└── chordlock.{js,wasm}             # WebAssembly build

scripts/
└── build_all.sh                     # Comprehensive build system
```

## Requirements

- **C++17** compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **Emscripten** for WebAssembly builds
- Modern browser with Web MIDI API support

## License

MIT License

## Author

**Leo Kuroshita** ([@kurogedelic](https://github.com/kurogedelic))

© 2024 Leo Kuroshita. All rights reserved.

## Examples

### Chord Progression Generation
```cpp
// Generate I-vi-IV-V progression in C major
std::vector<std::string> progression = {"C", "Am", "F", "G"};
for (const auto& chord : progression) {
    auto notes = detector.chordNameToNotes(chord, 4);
    // Use notes for MIDI generation, playback, etc.
}
```

### Intelligent Chord Name Parsing
```cpp
// All of these resolve to the same Cmaj7 chord
auto notes1 = detector.chordNameToNotes("CM7", 4);      // [48, 52, 55, 59]
auto notes2 = detector.chordNameToNotes("Cmaj7", 4);    // [48, 52, 55, 59]  
auto notes3 = detector.chordNameToNotes("CMajor7", 4);  // [48, 52, 55, 59]

// Enharmonic equivalents work automatically
auto notesFs = detector.chordNameToNotes("F#m", 4);     // Alternative suggestions
auto notesGb = detector.chordNameToNotes("Gbm", 4);     // [55, 60, 63] - direct match
```

### Real-time Bidirectional Analysis
```cpp
// Live MIDI → Chord → Alternative voicings workflow
detector.noteOn(60, 80); detector.noteOn(64, 80); detector.noteOn(67, 80);
auto detected = detector.detectChord();
std::cout << "Detected: " << detected.chordName << std::endl; // "C"

// Generate alternative voicings for the same chord
auto alternatives = detector.chordNameToNotesWithAlternatives(detected.chordName, 5);
for (const auto& voicing : alternatives) {
    // Play different inversions/voicings
}
```

---

*Built for musicians, developers, and music technology enthusiasts who demand bidirectional accuracy and performance.*
