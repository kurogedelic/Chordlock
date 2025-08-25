# ChordLock WebAssembly Demo

Full-featured web demo of the ChordLock chord recognition engine running entirely in the browser using WebAssembly.

## 🎵 Features

### Core Recognition
- **Ultra-fast chord identification** - 0.749μs per chord
- **500+ chord patterns** - Comprehensive chord database
- **Real-time analysis** - Instant feedback as you play

### Advanced Features
- **Jazz chord recognition** - 9th, 11th, 13th, altered extensions
- **Quartal harmony detection** - Fourth-based voicings
- **Polychord detection** - Simultaneous chord recognition
- **Slash chord notation** - Bass note identification
- **Rootless voicings** - Jazz piano voicings
- **Inversions** - All inversion types

### Interactive Tools
- **Virtual Piano** - Click to build chords
- **MIDI Input** - Enter MIDI note numbers
- **Transpose** - Shift chords up/down
- **Chord Builder** - Select root and type
- **Suggestions** - Next chord recommendations
- **Performance Benchmark** - Test speed in browser

## 🚀 Quick Start

### Prerequisites
Install Emscripten for WebAssembly compilation:

```bash
# Clone Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest SDK
./emsdk install latest
./emsdk activate latest

# Set up environment
source ./emsdk_env.sh
```

### Build

```bash
# From the wasm directory
chmod +x build_wasm.sh
./build_wasm.sh
```

### Run Locally

```bash
# Start local server
cd build_wasm
python3 -m http.server 8000

# Open in browser
# http://localhost:8000
```

## 📁 Project Structure

```
wasm/
├── index.html           # Main HTML interface
├── style.css           # Modern dark theme styling
├── app.js              # JavaScript application logic
├── wasm_binding.cpp    # C++ to JavaScript bindings
├── CMakeLists.txt      # Emscripten build configuration
└── build_wasm.sh       # Build script
```

## 🎹 Usage

### Virtual Piano
- Click on keys to select notes
- Selected notes appear as badges
- Chord analysis updates in real-time

### Recognition Modes
- **Standard** - Basic chord recognition
- **Jazz** - Extended jazz harmony analysis
- **Advanced** - Polychords and quartal detection

### Examples
Quick example buttons for common chords:
- Basic: C, Cm, C7, Cmaj7
- Extended: C9, C11, C13
- Altered: C7b5, C7#9
- Special: Quartal, Polychord, Rootless

## 🔧 Technical Details

### WebAssembly Module Size
- **JavaScript wrapper**: ~50KB
- **WASM binary**: ~200KB
- **Total download**: ~250KB (gzipped: ~80KB)

### Performance
- **Recognition speed**: < 1ms in browser
- **Initialization**: < 100ms
- **Memory usage**: < 4MB

### Browser Support
- Chrome 57+
- Firefox 52+
- Safari 11+
- Edge 16+

## 🎨 Customization

### Styling
Edit `style.css` to customize the appearance:
- Color scheme uses CSS variables
- Fully responsive design
- Dark theme by default

### Adding Chord Types
Edit `wasm_binding.cpp` to add new chord types:
```cpp
// Add to buildChord() function
case 'custom': notes = [root, ...]; break;
```

## 📊 API Reference

### JavaScript Interface

```javascript
// Initialize
const chordlock = new ChordLockModule.ChordLock();

// Identify chord
const result = chordlock.identifyChord([60, 64, 67]);

// Jazz analysis
const jazz = chordlock.identifyJazzChord([60, 64, 67, 70, 62]);

// Detect polychord
const poly = chordlock.detectPolychord([60, 64, 67, 74, 78, 81]);

// Quartal harmony
const quartal = chordlock.detectQuartalHarmony([60, 65, 70, 75]);

// Utilities
const noteName = chordlock.midiToNoteName(60);  // "C4"
const midiNote = chordlock.noteNameToMidi("C4"); // 60
const transposed = chordlock.transposeChord([60, 64, 67], 7);
```

## 🚢 Deployment

### Static Hosting
Deploy to any static hosting service:
1. Build the WASM module
2. Upload all files from `build_wasm/`
3. Ensure proper MIME types:
   - `.wasm` → `application/wasm`
   - `.js` → `application/javascript`

### CDN Integration
For production, consider using a CDN:
```html
<script src="https://your-cdn.com/chordlock_wasm.js"></script>
```

## 📝 License

GNU Lesser General Public License v3.0 (LGPL-3.0)

Copyright (C) 2024 Leo Kuroshita (@kurogedelic)