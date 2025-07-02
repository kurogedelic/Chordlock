# 🎵 Chordlock v2.0

> **Revolutionary MIDI chord detection engine with key-context awareness**

Chordlock provides intelligent chord detection that considers musical key context for accurate harmonic analysis. Unlike traditional systems, it understands that the same notes can represent different chords depending on the musical context.

## ✨ Key Innovation

```bash
# Same notes, different interpretations based on key
./build/chordlock -N 52,60,64,67,74 -k C   # → C/E [I] in C major
./build/chordlock -N 52,60,64,67,74 -k Em  # → Em7(add♭6) [i7] in E minor
```

## 🚀 Quick Start

```bash
# Install
git clone https://github.com/kurogedelic/Chordlock.git
cd Chordlock && make build

# Use CLI
./build/chordlock -N 60,64,67 -k C         # Detect chord with key context
./build/chordlock -d "V7" -k C             # Generate V7 chord in C major
./build/chordlock -c "Cmaj7"               # Convert chord name to MIDI notes
```

## 📚 Documentation & Demos

**[📖 Full Documentation](https://kurogedelic.github.io/Chordlock/docs/)**

- [🚀 Quick Start Guide](https://kurogedelic.github.io/Chordlock/docs/quick-start)
- [📋 API Reference](https://kurogedelic.github.io/Chordlock/docs/api/)
- [🎼 Key Context Analysis](https://kurogedelic.github.io/Chordlock/docs/installation)

**🎹 Interactive Web Applications:**
- [🎹 Simple Piano](https://kurogedelic.github.io/Chordlock/web_app/simple-piano.html) - Visual chord testing
- [📚 Chord Dictionary](https://kurogedelic.github.io/Chordlock/web_app/dictionary.html) - Chord ↔ Notes converter

## 🎯 Core Features

- **🔥 Key-Context Aware**: World-first chord detection considering musical key
- **🎼 Roman Numeral Analysis**: Automatic functional harmony analysis  
- **⚡ High Performance**: Sub-4ms detection with production-ready reliability
- **🔧 Multiple Interfaces**: CLI, C++, WebAssembly, MCP integration
- **🎵 Advanced Chords**: 500+ chord patterns including jazz harmony

## 🏗️ Architecture

```
src/           # Core C++ engine with advanced detection algorithms
├── engines/   # Hash-based chord detection with symmetric processing
├── processors/ # MIDI velocity and audio processing
└── interfaces/ # Clean API abstractions

build/         # Compiled CLI application
web/           # WebAssembly binaries for browser integration
web_app/       # Interactive demo applications
chordlock-mcp/ # AI integration via Model Context Protocol
docs/          # Documentation and guides
scripts/       # Build automation and deployment tools
```

## ⚡ Performance

Chordlock v2.0 delivers exceptional real-time performance optimized for production use:

**🚀 Core Metrics:**
- **Detection Time**: Sub-4ms average response
- **Accuracy**: 100% for basic triads, 85-90% for complex jazz harmony
- **Memory**: Minimal footprint with hash-table optimization
- **Throughput**: Real-time MIDI processing capability

**🔧 Technical Innovations:**
- **Hash-based Lookup**: O(1) chord pattern matching
- **Symmetric Processing**: Dedicated lane for augmented/diminished chords
- **Bass Priority Rule**: 100-point scoring for root detection
- **SIMD Optimization**: Vector processing for audio analysis

**📊 Production Features:**
- ✅ Real-time MIDI chord detection
- ✅ Key-context aware harmonic analysis
- ✅ Roman numeral functional analysis
- ✅ Cross-platform compatibility (CLI + WebAssembly)
- ✅ Zero-dependency core engine

## 🤖 AI Integration

Works with Claude via Model Context Protocol:

```bash
npm install -g @kurogedelic/chordlock-mcp
```

Ask Claude: *"Generate a jazz ii-V-I progression in Bb major"*

## 📄 License

LGPL-3.0-only - see [LICENSE](LICENSE) for details.

**© 2024-2025 Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic))**
