# Chordlock MCP Server

**Music analysis and composition through Model Context Protocol (MCP)**

[![npm version](https://badge.fury.io/js/chordlock-mcp.svg)](https://www.npmjs.com/package/chordlock-mcp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Chordlock MCP Server enables AI agents (Claude, ChatGPT, etc.) to perform advanced music analysis, chord detection, and composition assistance through the Model Context Protocol.

## Features

🎯 **Chord Detection**: Analyze MIDI notes → chord names with confidence scoring  
🎵 **Chord Generation**: Convert chord names → MIDI notes with multiple voicings  
🎼 **Progression Builder**: Generate complete chord progressions with timing  
🔍 **Smart Suggestions**: Find similar/alternative chords for composition  
📊 **Harmonic Analysis**: Roman numeral analysis and functional harmony  
⚡ **High Performance**: Sub-millisecond detection powered by WebAssembly  

## Installation

```bash
# Install from GitHub Packages
npm install -g @kurogedelic/chordlock-mcp

# Or install locally in your project
npm install @kurogedelic/chordlock-mcp
```

**Note**: This package is published to GitHub Packages. You may need to configure npm to use GitHub's registry:

```bash
# Configure npm for GitHub Packages
npm config set @kurogedelic:registry https://npm.pkg.github.com
```

## Quick Start

### 1. Configure with Claude Desktop

Add to your Claude Desktop MCP configuration (`claude_desktop_config.json`):

```json
{
  "mcpServers": {
    "chordlock": {
      "command": "npx",
      "args": ["@kurogedelic/chordlock-mcp"]
    }
  }
}
```

### 2. Use with AI Agents

Once configured, you can ask Claude:

> "Analyze the chord progression: C - Am - F - G. What's the harmonic function?"

> "What MIDI notes make up a Dm7b5 chord?"

> "Generate a jazz ii-V-I progression in Bb major"

> "What chord is formed by MIDI notes 60, 64, 67, 71?"

## CLI Usage

```bash
# Start MCP server
chordlock-mcp

# The server communicates via JSON-RPC over stdio
# Typically used by MCP-compatible AI systems
```

## Available Tools

### `detect_chord`
Detect chord names from MIDI note arrays.

```typescript
{
  "name": "detect_chord",
  "arguments": {
    "notes": [60, 64, 67, 71]  // C, E, G, B (Cmaj7)
  }
}
```

### `chord_to_notes` 
Convert chord names to MIDI notes.

```typescript
{
  "name": "chord_to_notes", 
  "arguments": {
    "chordName": "Dm7b5",
    "rootOctave": 4
  }
}
```

### `chord_progression`
Generate MIDI sequences from chord progressions.

```typescript
{
  "name": "chord_progression",
  "arguments": {
    "chords": ["C", "Am", "F", "G"],
    "key": "C major",
    "beatsPerChord": 4
  }
}
```

### `find_similar_chords`
Find alternative chord names and enharmonic equivalents.

```typescript
{
  "name": "find_similar_chords",
  "arguments": {
    "chordName": "F#m",
    "maxResults": 10
  }
}
```

### `analyze_harmony`
Perform harmonic analysis with Roman numerals.

```typescript
{
  "name": "analyze_harmony",
  "arguments": {
    "chords": ["C", "Am", "F", "G"],
    "key": "C major"
  }
}
```

## AI Use Cases

**🎼 Composition Assistance**
- "Generate a chord progression for a jazz ballad in F major"
- "What are some alternative chords I can use instead of Dm7?"
- "Create a ii-V-I progression with extensions in all 12 keys"

**📚 Music Education**  
- "Explain the harmonic function of each chord in this progression"
- "What notes are in a C#dim7 chord?"
- "Analyze this chord progression and suggest improvements"

**🎹 MIDI Production**
- "Convert this lead sheet to MIDI note data"
- "What chord is being played at these MIDI note numbers?"
- "Generate backing track chord data for my song"

**🔬 Music Analysis**
- "Analyze the harmonic content of this MIDI file"
- "Identify the key and mode of this chord progression"  
- "Find chord substitutions that maintain the harmonic function"

## Supported Chord Types

**339 chord types** including:
- **Triads**: Major, Minor, Diminished, Augmented
- **7th Chords**: Major 7th, Dominant 7th, Minor 7th, Half-diminished
- **Extensions**: 9th, 11th, 13th chords with various alterations
- **Jazz Harmony**: Altered dominants, polytonal chords, quartal harmony
- **Suspended**: sus2, sus4, sus2sus4
- **Added Tones**: add9, add11, add13

## Performance

- **Detection Speed**: ~0.001ms (native) / ~0.002ms (WebAssembly)
- **Memory Usage**: ~2MB WebAssembly heap
- **Accuracy**: 99%+ for standard chord types
- **Concurrency**: Thread-safe, multiple simultaneous requests

## Requirements

- **Node.js**: 16.0.0 or higher
- **Memory**: 50MB available RAM
- **MCP Client**: Claude Desktop, or other MCP-compatible AI system

## Development

```bash
# Clone and setup
git clone https://github.com/kurogedelic/Chordlock.git
cd Chordlock/chordlock-mcp

# Install dependencies
npm install

# Build TypeScript
npm run build

# Run locally
npm start
```

## API Reference

See [API Documentation](./docs/api.md) for detailed type definitions and usage examples.

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

*Built for musicians, developers, and enthusiasts who demand accurate and fast music analysis.*

## License

MIT License - see [LICENSE](../LICENSE) for details.

## Author

**Leo Kuroshita** ([@kurogedelic](https://github.com/kurogedelic))

© 2024-2025 Leo Kuroshita. All rights reserved.

## Related Projects

- **[Chordlock Core](https://github.com/kurogedelic/Chordlock)** - The underlying C++ chord detection engine
- **[Chordlock Web Demo](https://kurogedelic.github.io/Chordlock/)** - Interactive browser-based chord detection