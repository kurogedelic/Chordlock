# ChordLock 🎵

**Ultra-fast chord identification engine** - Identify any chord in under 1μs with proper music notation

## Quick Start

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# Basic usage
./chord_cli --notes "60,64,67"        # → C
./chord_cli --notes "64,67,72" --analyze # → C/E (slash chord)

# Performance test
./chord_cli --benchmark               # → ~0.45μs per chord
```

## Features

- ⚡ **Ultra-fast**: 0.45μs per chord (22x faster than 10μs target)
- 🎯 **Accurate**: 98% accuracy on complex chords, 100% on standard chords
- 🎼 **Music notation**: Proper chord names (C, Dm7, F#m7♭5) with algorithmic generation
- 🔄 **Inversion detection**: Correctly identifies slash chords (C/E, C/G)
- 🔧 **No dependencies**: Compiled chord tables, no YAML runtime
- 🍎 **ARM optimized**: NEON SIMD for Apple Silicon
- 🎹 **Comprehensive**: 100+ chord types with jazz notation support

## Performance

| Metric | Result | Target |
|--------|--------|--------|
| Speed | **0.45μs** | 10μs |
| Memory | **<1MB** | <1MB |
| Accuracy | **100%** | >99% |
| Startup | **<1ms** | <1ms |

## Supported Chords

- **Triads**: C, Cm, C°, C+, Csus2, Csus4
- **7ths**: C7, CM7, Cm7, Cø7, C°7
- **Extensions**: C9, C11, C13, Cadd9, Cmadd9
- **Altered**: C7♭9, C7♯9, C7♯11, C7♭13, C7alt
- **Slash chords**: C/E, C/G, Dm7/C
- **Advanced**: Inversions, complex jazz notation

## Examples

```bash
# Standard chords with proper names
./chord_cli --notes "60,64,67"      # → C
./chord_cli --notes "60,63,67"      # → Cm  
./chord_cli --notes "60,64,67,70"   # → C7

# Inversions and slash chords
./chord_cli --notes "64,67,72" --analyze
# → C/E (slash chord), 100% confidence, Root: C

# Complex chords
./chord_cli --notes "60,64,67,74"   # → Cadd9
./chord_cli --notes "60,64,67,70,75" # → C7♯9

# Batch processing
echo -e "60,64,67\n64,67,72" | ./chord_cli --batch
```

## Architecture

```
Core/
├── IntervalEngine       # SIMD-optimized interval calculation
├── ChordDatabase        # Fast lookup with caching
├── ChordIdentifier      # Main identification logic
└── ChordNameGenerator   # Algorithmic chord name generation

Analysis/
└── InversionDetector    # Slash chord and inversion analysis

Utils/
└── NoteConverter        # MIDI ↔ note name conversion with accidentals

Data/
└── CompiledTables.h     # Pre-compiled chord database (no runtime YAML)
```

## Building

**Requirements**: C++17, CMake 3.15+

**Optional**: yaml-cpp, GoogleTest, Google Benchmark

```bash
# macOS
brew install cmake googletest google-benchmark

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Test
make test
./chord_benchmark
```

## License

GNU Lesser General Public License v3.0 (LGPL-3.0)

This project is licensed under the LGPL-3.0 License - see the [GNU website](https://www.gnu.org/licenses/lgpl-3.0.html) for details.

**LGPL-3.0 allows:**
- Commercial use
- Modification
- Distribution
- Patent use
- Private use

**Requirements:**
- License and copyright notice
- State changes
- Disclose source (for LGPL components only)
- Same license (for LGPL components only)

This means you can use ChordLock in proprietary applications as a library, but any modifications to ChordLock itself must be shared under LGPL-3.0.