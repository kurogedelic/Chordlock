# Claude Code + Chordlock MCP Setup Guide

## Quick Setup

### Method 1: Config File (Recommended)
Create config file at `~/.config/claude-code/claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "chordlock": {
      "command": "node",
      "args": [
        "/Users/kurogedelic/Chordlock/chordlock-mcp/dist/index.js"
      ],
      "env": {}
    }
  }
}
```

### Method 2: Alternative Location
If Method 1 doesn't work, try `~/Library/Application Support/Claude Code/claude_desktop_config.json`

### Method 3: Environment Variable
Set environment variable in your shell:

```bash
export CLAUDE_MCP_CONFIG="/Users/kurogedelic/.claude-code-mcp.json"
```

## Available Tools

Once configured, you can use these music analysis tools in Claude Code:

### 🎯 detect_chord
```
"Detect the chord from MIDI notes: 60, 64, 67, 71"
```

### 🎵 chord_to_notes  
```
"What MIDI notes make up a Dm7b5 chord?"
```

### 🎼 chord_progression
```
"Generate a jazz ii-V-I progression in Bb major"
```

### 🔍 find_similar_chords
```
"What are alternative chords to F#m?"
```

### 📊 analyze_harmony
```
"Analyze this chord progression: C - Am - F - G"
```

## Verification

To verify the setup works:

1. Start a new Claude Code session
2. Ask: "What MIDI notes are in a C major chord?"
3. You should see the MCP tool being used

## Troubleshooting

- **Server not starting**: Check that Node.js is installed and the path to index.js is correct
- **Permission errors**: Make sure the files are executable: `chmod +x dist/index.js`
- **JSON errors**: The stdout pollution fix should resolve parsing issues

## Example Usage in Claude Code

```
Human: Generate chord progressions for a bossa nova song in A minor

Claude Code will automatically use the chordlock MCP tools to:
1. Generate appropriate chord sequences
2. Convert chord names to MIDI notes  
3. Suggest harmonic alternatives
4. Provide Roman numeral analysis
```