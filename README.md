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
- **⚡ High Performance**: Sub-millisecond detection with C++ optimization
- **🔧 Multiple Interfaces**: CLI, C++, WebAssembly, MCP integration
- **🎵 Advanced Chords**: 339+ chord types including jazz harmony

## 🏗️ Architecture

```
src/          # Core C++ engine
build/        # CLI application  
web_app/      # Interactive demos
chordlock-mcp/ # AI integration
docs/         # Documentation site
```

## 🤖 AI Integration

Works with Claude via Model Context Protocol:

```bash
npm install -g @kurogedelic/chordlock-mcp
```

Ask Claude: *"Generate a jazz ii-V-I progression in Bb major"*

## 📄 License

MIT License - see [LICENSE](LICENSE) for details.

**© 2024-2025 Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic))**