# CLI Interface

The Chordlock command-line interface provides full access to key-context chord detection and Roman numeral analysis.

## Installation

Build from source:

```bash
g++ -std=c++17 -O2 -I./src -I./src/engines -I./src/interfaces \
    -I./src/processors -I./src/utils src/cli_main.cpp src/Chordlock.cpp \
    src/engines/EnhancedHashLookupEngine.cpp src/processors/VelocityProcessor.cpp \
    -o build/chordlock -framework CoreMIDI -framework CoreFoundation
```

## Basic Usage

```bash
./build/chordlock [options]
```

## Options

### Core Options

| Option | Description | Example |
|--------|-------------|---------|
| `-N, --notes <notes>` | Analyze specific MIDI notes | `-N 60,64,67` |
| `-k, --key <key>` | Set key context | `-k C`, `-k Am` |
| `-d, --degree <degree>` | Generate chord from degree | `-d "V7"` |
| `-c, --chord <name>` | Convert chord name to notes | `-c "Cmaj7"` |
| `-f, --file <filename>` | Read notes from file | `-f chords.txt` |

### Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `-n, --no-slash` | Disable slash chord detection | Enabled |
| `-v, --velocity` | Enable velocity sensitivity | Disabled |
| `-h, --help` | Show help message | - |

## Note Analysis

Analyze MIDI notes with automatic Roman numeral analysis:

```bash
# Basic chord detection
./build/chordlock -N 60,64,67 -k C
# Output:
# Notes: 60, 64, 67
# Detected Chord: C (confidence: 21.87)
# Roman Numeral: I (in C major)

# Complex chord with alternatives
./build/chordlock -N 55,59,62,65 -k C
# Output:
# Notes: 55, 59, 62, 65  
# Detected Chord: G7 (confidence: 3.90)
# Roman Numeral: V7 (in C major)
# 
# Alternative interpretations:
#   Bdim/G (0.93) [vii°]
#   D7/G (0.64) [ii7]
```

### Note Input Formats

```bash
# Comma-separated
./build/chordlock -N 60,64,67

# Bracketed format  
./build/chordlock -N "[60, 64, 67]"

# Spaces are ignored
./build/chordlock -N "60, 64, 67"
```

## Key Context

Key context is **required** for Roman numeral analysis and affects chord interpretation:

```bash
# Major keys
./build/chordlock -N 60,64,67 -k C     # C major
./build/chordlock -N 60,64,67 -k G     # G major
./build/chordlock -N 60,64,67 -k F#    # F# major

# Minor keys (append 'm')
./build/chordlock -N 57,60,64 -k Am    # A minor
./build/chordlock -N 57,60,64 -k Em    # E minor
./build/chordlock -N 57,60,64 -k Bm    # B minor
```

### Key Context Effects

Same notes, different interpretations:

```bash
# E-C-E-G-D notes in different keys
./build/chordlock -N 52,60,64,67,74 -k C
# → C/E (30.00) [I] - Tonic with bass boost

./build/chordlock -N 52,60,64,67,74 -k Em  
# → Em7(add♭6) (41.58) [i7] - Tonic i7 with massive boost
```

## Degree Generation

Generate chords from Roman numerals (requires `-k`):

```bash
# Basic degrees
./build/chordlock -d "I" -k C      # → C [48, 52, 55]
./build/chordlock -d "V" -k C      # → G [55, 59, 62]
./build/chordlock -d "vi" -k C     # → Am [57, 60, 64]

# Extended chords
./build/chordlock -d "V7" -k C     # → G7 [55, 59, 62, 65]
./build/chordlock -d "ii7" -k C    # → Dm7 [50, 53, 57, 60]
./build/chordlock -d "I9" -k C     # → C9 [48, 52, 55, 58, 62]

# Minor key degrees
./build/chordlock -d "i" -k Am     # → Am [57, 60, 64]
./build/chordlock -d "V7" -k Am    # → E7 [52, 56, 59, 62]
./build/chordlock -d "bVII" -k Am  # → G [55, 59, 62]
```

### Degree Validation

Degree generation requires key context:

```bash
./build/chordlock -d "V7"
# Error: Degree analysis (-d) requires key context (-k)
# Example: ./build/chordlock -d "V7" -k C
```

## Chord Name Conversion

Convert chord names to MIDI notes:

```bash
# Basic chords
./build/chordlock -c "C"           # → [48, 52, 55]
./build/chordlock -c "Am"          # → [57, 60, 64]
./build/chordlock -c "G7"          # → [55, 59, 62, 65]

# Complex chords
./build/chordlock -c "Cmaj7"       # → [48, 52, 55, 59]
./build/chordlock -c "Dm7b5"       # → [50, 53, 56, 60]
./build/chordlock -c "F#m7"        # → [54, 57, 61, 64]

# Alternative notations
./build/chordlock -c "CM7"         # → [48, 52, 55, 59] (fallback)
./build/chordlock -c "Dmin7"       # → [50, 53, 57, 60] (normalized)
```

## File Input

Process multiple chord sets from a file:

```bash
# Create chord file
echo "60,64,67" > chords.txt
echo "55,59,62,65" >> chords.txt  
echo "57,60,64" >> chords.txt

# Process file
./build/chordlock -f chords.txt -k C
```

File format:
- One chord per line
- MIDI notes as comma-separated values
- Comments start with `#`
- Empty lines are ignored

```
# Example progression in C major
60,64,67        # I
57,60,64        # vi  
53,57,60        # IV
55,59,62,65     # V7
```

## Output Formats

### Standard Output

```
Chordlock Command Line Interface
================================
Configuration:
  Slash chords: enabled
  Velocity sensitivity: disabled
  Key context: C major

Notes: 60, 64, 67
Detected Chord: C (confidence: 21.87)
Roman Numeral: I (in C major)
```

### JSON Output (via degree generation)

```bash
./build/chordlock -d "V7" -k C
# Includes JSON format:
# {"degree": "V7", "chordName": "G7", "notes": [55, 59, 62, 65], "key": "C major"}
```

## Configuration

### Slash Chord Detection

```bash
# Enable slash chords (default)
./build/chordlock -N 52,60,64,67

# Disable slash chords
./build/chordlock -N 52,60,64,67 -n
```

### Velocity Sensitivity

```bash
# Enable velocity sensitivity
./build/chordlock -N 60,64,67 -v

# Use with MIDI input (when available)
./build/chordlock -v
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Invalid arguments or general error |

## Examples

### Jazz ii-V-I Progression

```bash
./build/chordlock -d "ii7" -k C    # → Dm7
./build/chordlock -d "V7" -k C     # → G7  
./build/chordlock -d "I7" -k C     # → Cmaj7
```

### Modal Analysis

```bash
# Same chord in different keys
./build/chordlock -N 50,53,57,60 -k C   # → Dm7 [ii7]
./build/chordlock -N 50,53,57,60 -k Bb  # → Dm7 [iii7]
./build/chordlock -N 50,53,57,60 -k F   # → Dm7 [vi7]
```

### Edge Case Testing

```bash
# Test key-context dependent interpretation
./build/chordlock -N 55,60,65,69 -k C   # → F [IV] in C major
./build/chordlock -N 55,60,65,69 -k Gb  # → F/G [different context]
```

## Performance

- **Single chord**: ~1ms
- **With alternatives**: ~2ms  
- **File processing**: ~1ms per chord
- **Memory usage**: <10MB

The CLI interface provides the full power of Chordlock's key-context analysis in a simple command-line tool perfect for testing, batch processing, and integration into larger workflows.