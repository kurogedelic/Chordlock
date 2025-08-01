# 🎵 ChordLock

Ultra-fast chord identification engine with real-time MIDI support.

## ✨ Features

- **⚡ Ultra Fast**: ~0.45μs chord identification (22x faster than target)
- **🎹 MIDI Support**: Real-time input from MIDI devices
- **🧠 Advanced Theory**: 339+ chord types including complex jazz harmony
- **🎼 Inversion Detection**: Automatic bass note and slash chord recognition
- **🔄 Multiple Formats**: CLI, WebAssembly, C++ library

## 🌐 Web Demo

**Try it live**: https://kurogedelic.github.io/ChordLock/web/

Interactive chord identification in your browser - no installation required!

## 🚀 Quick Start

### Prerequisites
- C++17 compatible compiler
- CMake 3.15+

### Build & Test
```bash
git clone https://github.com/kurogedelic/ChordLock.git
cd ChordLock
mkdir build && cd build
cmake ..
make
./chordlock_cli 60,64,67  # Test C Major
```

### Web Demo (Local)
```bash
cd web
# Serve with any web server, e.g.:
python3 -m http.server 8000
# Open http://localhost:8000
```

## 📁 Project Structure

```
ChordLock/
├── Core/                   # Main engine
│   ├── ChordIdentifier.*   # Primary identification logic
│   ├── ChordDatabase.*     # Chord pattern database
│   ├── ChordNameGenerator.*# Chord name conversion
│   ├── IntervalEngine.*    # Interval processing
│   └── PerformanceStrategy.h # Optimization layer
├── Utils/                  # Utilities
│   └── NoteConverter.*     # MIDI ↔ Note name conversion
├── Analysis/               # Advanced features
│   └── InversionDetector.* # Bass detection & inversions
├── Data/                   # Chord patterns
│   └── CompiledTables.h    # Pre-compiled chord database
├── Examples/               # Usage examples
│   └── cli_example.cpp     # Command-line interface
├── web/                    # Web demo
│   ├── index.html          # Interactive demo page
│   ├── chordlock.js        # JavaScript implementation
│   └── README.md           # Web demo documentation
└── .github/workflows/      # GitHub Actions
    └── pages.yml           # Auto-deploy to GitHub Pages
```

## 🎯 Performance

- **Speed**: 0.45μs average (target: <10μs)
- **Accuracy**: 96% on comprehensive test suite
- **Memory**: <1MB footprint
- **Chord Types**: 339+ patterns supported

## 📜 License

LGPL-3.0 - See LICENSE.txt for details.

## 🔗 Links

- **GitHub**: https://github.com/kurogedelic/ChordLock
- **Author**: Leo Kuroshita (@kurogedelic)