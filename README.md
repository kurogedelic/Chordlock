# Chordlock

the chords detective.

![Chordlock_m](https://github.com/user-attachments/assets/477aea18-f064-417e-8a6c-e11dfe203c44)



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

### Command Line (macOS)

Build and run the CLI version:

```bash
# Build the CLI
./build_cli.sh

# Run with MIDI input
./chordlock_cli

# Run without slash chord detection
./chordlock_cli --no-slash

# Run with velocity sensitivity
./chordlock_cli --velocity
```

### Test Program

Run the test program to verify chord detection:

```bash
# Build the test program
./build_test.sh

# Run tests
./test_chordlock
```

### Building WebAssembly

Requires Emscripten:

```bash
./compile.sh
```

## License

LGPL-3.0
