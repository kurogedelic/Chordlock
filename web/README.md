# ChordLock Web Demo

Interactive web demonstration of ChordLock chord identification.

## 🌐 Live Demo

**GitHub Pages**: https://kurogedelic.github.io/ChordLock/web/

## 🎯 Features

- **Instant Chord Identification**: Enter MIDI notes and get chord names
- **Preset Chords**: Quick buttons for common chord patterns
- **Inversion Detection**: Recognizes slash chords (C/E, C/G)
- **Real-time Processing**: Client-side JavaScript implementation
- **Mobile Friendly**: Responsive design for all devices

## 🎹 Usage

1. Enter MIDI note numbers separated by commas (e.g., `60,64,67`)
2. Click "Identify Chord" or use preset buttons
3. View the identified chord name and details

### Example Inputs

- **C Major**: `60,64,67`
- **C Minor**: `60,63,67`
- **C7**: `60,64,67,70`
- **C/E (first inversion)**: `64,67,72`
- **Dm7**: `62,65,69,72`

## 🚀 How It Works

The web demo uses a simplified JavaScript implementation of ChordLock's chord identification algorithm:

1. **Normalize Notes**: Convert MIDI notes to semitone intervals
2. **Pattern Matching**: Compare against built-in chord database
3. **Root Detection**: Identify root note and handle inversions
4. **Name Generation**: Create proper chord notation

## 📊 Supported Chords

- **Triads**: Major, Minor, Diminished, Augmented
- **Suspended**: Sus2, Sus4
- **Sevenths**: 7, M7, m7, m7♭5, dim7
- **Extended**: 6, add9, 9, M9, m9
- **Altered**: 7♭9, 7♯9, 7♯11, 7♯13
- **Inversions**: First and second inversions as slash chords

## 🔧 Technical Details

- **Pure JavaScript**: No external dependencies
- **Client-side Processing**: No server required
- **Fast Performance**: ~1-5ms identification time
- **Pattern Database**: 50+ chord patterns with inversions

## 📱 Browser Support

- Chrome 60+
- Firefox 55+
- Safari 11+
- Edge 79+

## 🎵 About ChordLock

ChordLock is an ultra-fast chord identification engine written in C++. This web demo showcases a subset of its capabilities in a browser-friendly format.

For the full C++ library with advanced features, see: [ChordLock Repository](https://github.com/kurogedelic/ChordLock)