# API Reference

Chordlock provides multiple interfaces for different use cases.

## Overview

| Interface | Use Case | Performance | Features |
|-----------|----------|-------------|----------|
| [CLI](/api/cli) | Command-line usage, testing | Native | Full feature set |
| [C++ Library](/api/cpp) | Desktop applications | Native | Core engine |
| [WebAssembly](/api/wasm) | Web applications | Near-native | Browser compatibility |
| [MCP Integration](/api/mcp) | AI-powered analysis | Network | Claude integration |

## Core Concepts

### Key Context

Chordlock's key innovation is **key-context aware** chord detection:

```cpp
// Set key context before analysis
detector.setKeyContext(0, false);  // C major
detector.setKeyContext(9, true);   // A minor
```

Key context affects:
- **Functional priority**: Tonic/Dominant/Subdominant get boosts
- **Chord interpretation**: Same notes → different chord names
- **Roman numeral analysis**: Automatic degree calculation

### Detection Modes

Chordlock offers multiple detection approaches:

```cpp
// Basic detection
auto result = detector.detectChord();

// With alternatives
auto result = detector.detectChordWithAlternatives(5);

// With detailed analysis  
auto result = detector.detectChordWithDetailedAnalysis(10);
```

### Roman Numeral Analysis

Automatic functional harmony analysis:

```cpp
// Analyze current notes
auto degree = detector.analyzeCurrentNotesToDegree(tonic, isMinor);

// Analyze specific chord
auto degree = detector.analyzeDegree("Cmaj7", 0, false); // → "I7"

// Generate from degree
auto notes = detector.degreeToNotes("V7", 0, false); // → G7 notes
```

## Common Workflows

### 1. Real-time MIDI Processing

```cpp
class MIDIProcessor {
    Chordlock detector;
    
public:
    void setKey(int tonic, bool isMinor) {
        detector.setKeyContext(tonic, isMinor);
    }
    
    void onNoteOn(uint8_t note, uint8_t velocity) {
        detector.noteOn(note, velocity);
        
        auto result = detector.detectChord();
        if (result.hasValidChord) {
            auto degree = detector.analyzeCurrentNotesToDegree(tonic, isMinor);
            // Process result...
        }
    }
};
```

### 2. Batch Chord Analysis

```cpp
std::vector<std::string> analyzeProgression(
    const std::vector<std::vector<int>>& chords,
    int tonic, bool isMinor) {
    
    Chordlock detector;
    detector.setKeyContext(tonic, isMinor);
    
    std::vector<std::string> results;
    for (const auto& chord : chords) {
        detector.clearAllNotes();
        for (int note : chord) {
            detector.noteOn(note, 80);
        }
        
        auto result = detector.detectChord();
        auto degree = detector.analyzeCurrentNotesToDegree(tonic, isMinor);
        results.push_back(result.chordName + " [" + degree + "]");
    }
    
    return results;
}
```

### 3. Chord Generation

```cpp
// Generate chord progression
std::vector<std::string> degrees = {"I", "vi", "IV", "V"};
int tonic = 0; // C major

for (const auto& degree : degrees) {
    auto notes = detector.degreeToNotes(degree, tonic, false, 4);
    auto chordName = detector.degreeToChordName(degree, tonic, false);
    
    std::cout << degree << " = " << chordName << " = [";
    for (size_t i = 0; i < notes.size(); i++) {
        std::cout << notes[i];
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}
```

## Error Handling

### C++ Exceptions

```cpp
try {
    auto result = detector.detectChord();
    if (!result.hasValidChord) {
        // Handle no chord detected
    }
} catch (const std::exception& e) {
    // Handle errors
}
```

### CLI Error Codes

```bash
# Exit codes
0  # Success
1  # Invalid arguments
2  # File not found
3  # Invalid key specification
4  # No valid notes found
```

### WebAssembly Error Handling

```javascript
try {
    const result = Module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [5]);
    const data = JSON.parse(result);
    
    if (data.error) {
        console.error('Chordlock error:', data.error);
    }
} catch (e) {
    console.error('Module error:', e);
}
```

## Performance Considerations

### Optimization Tips

1. **Reuse instances**: Create detector once, reuse for multiple analyses
2. **Batch operations**: Group note changes when possible
3. **Limit alternatives**: Only request needed number of alternatives
4. **Key context**: Set once, analyze multiple chords in same key

### Benchmarks

| Operation | C++ | WebAssembly | CLI |
|-----------|-----|-------------|-----|
| Single chord detection | ~0.001ms | ~0.002ms | ~1ms |
| With 5 alternatives | ~0.005ms | ~0.010ms | ~2ms |
| Roman numeral analysis | ~0.001ms | ~0.002ms | ~0.5ms |
| Degree generation | ~0.003ms | ~0.005ms | ~1ms |

## API Stability

- **Stable APIs**: Core detection, key context, Roman numeral analysis
- **Experimental**: Advanced polychord analysis, custom scales
- **Deprecated**: Legacy functions from v1.x (use v2.0 equivalents)

Choose your interface and dive into the detailed documentation:

- [CLI Interface →](/api/cli)
- [C++ Library →](/api/cpp)  
- [WebAssembly →](/api/wasm)
- [MCP Integration →](/api/mcp)