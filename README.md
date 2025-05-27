# Chordlock

Real-time MIDI chord detection library with WebAssembly support.

## Demo

https://kurogedelic.github.io/Chordlock/

## Features

- Real-time chord detection from MIDI input
- Support for complex chords (7th, 9th, 11th, 13th)
- Slash chord detection
- WebAssembly for browser support
- Consistent chord notation

## Usage

### Web Browser

Open the demo page and allow MIDI access. Play chords on your MIDI keyboard to see real-time detection.

### Building from Source

Requires Emscripten:

```bash
./compile.sh
```

## License

LGPL-3.0