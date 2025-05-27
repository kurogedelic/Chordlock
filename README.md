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
- Direct note number input for chord analysis
- File-based batch processing

## Usage

### Web Browser

Open the demo page and allow MIDI access. Play chords on your MIDI keyboard to see real-time detection.

### Command Line (macOS)

Build and run the CLI version:

```bash
# Build the CLI
./build_cli.sh

# Run with MIDI input (real-time)
./chordlock_cli

# Run without slash chord detection
./chordlock_cli --no-slash

# Run with velocity sensitivity
./chordlock_cli --velocity

# Analyze specific notes directly
./chordlock_cli -N 60,64,67          # C major chord
./chordlock_cli -N "[60, 63, 67]"    # C minor chord

# Process multiple chords from file
./chordlock_cli -f chords.txt
```

#### CLI Options

- `-n, --no-slash` - Disable slash chord detection
- `-v, --velocity` - Enable velocity sensitivity
- `-N, --notes <notes>` - Analyze specific notes (e.g., "60,64,67" or "[60,64,67]")
- `-f, --file <filename>` - Read notes from file (one set per line)
- `-h, --help` - Show help message

#### File Format

Create a text file with note numbers (0-127), one chord per line:

```
# This is a comment
60,64,67        # C major
60,63,67        # C minor
67,71,74,77     # G7
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
