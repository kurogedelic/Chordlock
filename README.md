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
- [🎵 Audio Chord Detection](https://kurogedelic.github.io/Chordlock/web_app/) - Real-time audio analysis
- [🎹 Simple Piano](https://kurogedelic.github.io/Chordlock/web_app/simple-piano.html) - Visual chord testing
- [📚 Chord Dictionary](https://kurogedelic.github.io/Chordlock/web_app/dictionary.html) - Chord ↔ Notes converter

## 🎯 Core Features

- **🔥 Key-Context Aware**: World-first chord detection considering musical key
- **🎼 Roman Numeral Analysis**: Automatic functional harmony analysis  
- **⚡ High Performance**: 3.52ms average response time with 97%+ reliability
- **🔧 Multiple Interfaces**: CLI, C++, WebAssembly, MCP integration
- **🎵 Advanced Chords**: 339+ chord types including jazz harmony

## 🏗️ Architecture

```
src/          # Core C++ engine
build/        # CLI application  
web_app/      # Interactive demos
chordlock-mcp/ # AI integration
docs/         # Documentation site
benchmark/    # Performance testing suite
```

## ⚡ Performance

Chordlock v2.0 delivers exceptional performance with comprehensive benchmarking:

**🚀 Key Metrics:**
- **Average response time**: 3.52ms 
- **Fastest operation**: 3.11ms (chord name conversion)
- **Reliability**: 97%+ success rate across all tests
- **Consistency**: ±0.27ms standard deviation

**📊 Benchmark Coverage:**
- ✅ 34 comprehensive test scenarios
- ✅ All CLI modes and argument combinations  
- ✅ Basic triads → complex jazz chords
- ✅ Key context analysis (major/minor keys)
- ✅ Roman numeral degree analysis
- ✅ Edge cases and stress testing

**📈 Performance Breakdown:**
| Operation Type | Time Range | Quality |
|----------------|------------|---------|
| Basic chord detection | 2-4ms | Excellent |
| Complex jazz chords | 3-5ms | Very Good |
| Key context analysis | 3-4ms | Excellent |
| Degree analysis | 3-6ms | Very Good |

**🔧 Run Benchmarks:**
```bash
# Quick performance test
python3 benchmark/quick_test.py

# Full benchmark suite  
python3 benchmark/benchmark.py

# Compare results
python3 benchmark/compare_results.py before.json after.json
```

*See [benchmark/README.md](benchmark/README.md) for detailed performance analysis.*

## 🤖 AI Integration

Works with Claude via Model Context Protocol:

```bash
npm install -g @kurogedelic/chordlock-mcp
```

Ask Claude: *"Generate a jazz ii-V-I progression in Bb major"*

## 📄 License

MIT License - see [LICENSE](LICENSE) for details.

**© 2024-2025 Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic))**