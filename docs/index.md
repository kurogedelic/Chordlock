---
layout: home

hero:
  name: "Chordlock v2.0"
  text: "Advanced Key-Context Chord Detection"
  tagline: Revolutionary MIDI chord detection engine with functional harmony analysis
  image:
    src: /chordlock-logo.svg
    alt: Chordlock
  actions:
    - theme: brand
      text: Get Started
      link: /quick-start
    - theme: alt
      text: API Reference
      link: /api/
    - theme: alt
      text: View on GitHub
      link: https://github.com/kurogedelic/Chordlock

features:
  - icon: 🎯
    title: Key-Context Aware
    details: World-first chord detection that considers musical key context for accurate interpretation
  - icon: 🎼
    title: Roman Numeral Analysis
    details: Automatic functional harmony analysis with proper Roman numeral notation
  - icon: ⚡
    title: High Performance
    details: Sub-millisecond detection times with optimized C++ engine and WebAssembly support
  - icon: 🔧
    title: Multiple Interfaces
    details: CLI, C++ library, WebAssembly, and MCP integration for various use cases
---

## What is Chordlock?

Chordlock is an intelligent MIDI chord detection library that provides **key-context aware** chord analysis. Unlike traditional chord detection systems, Chordlock considers the musical key to provide contextually appropriate chord interpretations.

### Key Innovation

The same notes can represent different chords depending on the musical context:

```bash
# E-C-E-G-D notes
./chordlock -N 52,60,64,67,74 -k C    # → C/E [I] in C major  
./chordlock -N 52,60,64,67,74 -k Em   # → Em7(add♭6) [i7] in E minor
```

### Quick Example

```cpp
#include "Chordlock.hpp"

Chordlock detector;
detector.setKeyContext(0, false); // C major

detector.noteOn(60, 80); // C
detector.noteOn(64, 80); // E  
detector.noteOn(67, 80); // G

auto result = detector.detectChord();
// result.chordName = "C"

auto degree = detector.analyzeCurrentNotesToDegree(0, false);
// degree = "I"
```

## Architecture

Chordlock v2.0 provides a clean, organized structure:

- **`src/`** - Core C++ engine with key-context analysis
- **`build/`** - CLI binary with Roman numeral analysis
- **`web_app/`** - Interactive demo applications  
- **`chordlock-mcp/`** - AI integration via Model Context Protocol
- **`docs/`** - This documentation site

## Core Features

### 🔥 World-First Key-Context Analysis
- **Functional Harmony Priority**: Tonic/Dominant/Subdominant functions get priority boosts
- **Key-Dependent Interpretation**: Same notes = different chords based on musical key
- **Massive Confidence Boosts**: Em7 in Em → i7 with 41.58× boost

### 🎼 Advanced Chord Detection  
- **Complex Chords**: 7th, 9th, 11th, 13th extensions
- **Slash Chords**: Intelligent bass note analysis (`C/E`, `F/G`)
- **Altered Chords**: `G7#5b9`, `Dm7b5`, `C7alt`
- **Polychords**: Advanced harmonic structures

### ⚡ High Performance
- **Sub-millisecond detection**: Optimized C++ engine
- **Real-time processing**: Perfect for live performance  
- **Bitmasking optimization**: Efficient pattern matching
- **Cross-platform**: macOS, Linux, Windows, WebAssembly

Get started with the [Quick Start Guide](/quick-start) or explore the [API Reference](/api/).