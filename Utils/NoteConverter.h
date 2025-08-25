#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace ChordLock {

enum class AccidentalStyle {
    SHARPS,    // C, C#, D, D#, E, F, F#, G, G#, A, A#, B
    FLATS,     // C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
    MIXED,     // Context-dependent (prefer sharps for sharp keys, flats for flat keys)
    MINIMAL    // Prefer natural notes when possible
};

enum class OctaveNotation {
    SCIENTIFIC,  // C4 = middle C (MIDI note 60)
    HELMHOLTZ,   // c' = middle C  
    MIDI_NUMBER, // Raw MIDI number (0-127)
    NO_OCTAVE    // Note name only without octave
};

struct NoteInfo {
    std::string name;           // "C#4" or "Db4" etc.
    std::string name_no_octave; // "C#" or "Db" etc.
    int midi_number;            // 0-127
    int note_class;             // 0-11 (C=0, C#=1, etc.)
    int octave;                 // -1 to 9 (C4 = middle C)
    bool is_natural;            // true if natural note (C,D,E,F,G,A,B)
    bool is_sharp;              // true if sharp note
    bool is_flat;               // true if flat note (in flat representation)
    
    NoteInfo() : midi_number(-1), note_class(-1), octave(-1), 
                 is_natural(false), is_sharp(false), is_flat(false) {}
};

class NoteConverter {
private:
    // Static lookup tables for maximum performance
    static constexpr std::array<const char*, 12> SHARP_NAMES = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    static constexpr std::array<const char*, 12> FLAT_NAMES = {
        "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
    };
    
    static constexpr std::array<bool, 12> IS_NATURAL = {
        true, false, true, false, true, true, false, true, false, true, false, true
    };
    
    static constexpr std::array<bool, 12> IS_SHARP = {
        false, true, false, true, false, false, true, false, true, false, true, false
    };
    
    // Precomputed conversion tables (128 entries for all MIDI notes)
    std::array<NoteInfo, 128> midi_to_note_sharp;
    std::array<NoteInfo, 128> midi_to_note_flat;
    
    // Reverse lookup: note name to MIDI number
    std::unordered_map<std::string, int> note_name_to_midi;
    
    // Key signature context for intelligent accidental choice
    std::unordered_map<std::string, AccidentalStyle> key_preferences;
    
    // Current configuration
    AccidentalStyle default_accidental_style;
    OctaveNotation default_octave_notation;
    std::string current_key_signature;
    
    // Initialization helpers
    void buildLookupTables();
    void buildReverseMap();
    void initializeKeyPreferences();
    
    // Internal conversion helpers
    NoteInfo createNoteInfo(int midi_number, AccidentalStyle style) const;
    std::string formatNoteName(const NoteInfo& info, OctaveNotation notation) const;
    int parseNoteString(const std::string& note_str) const;
    
    // Context-aware accidental selection
    AccidentalStyle chooseAccidentalStyle(int note_class, const std::string& key_context) const;
    
public:
    NoteConverter();
    NoteConverter(AccidentalStyle default_style, OctaveNotation default_notation = OctaveNotation::SCIENTIFIC);
    ~NoteConverter() = default;
    
    // Primary conversion methods
    std::string midiToNoteName(int midi_number) const;
    std::string midiToNoteName(int midi_number, AccidentalStyle style) const;
    std::string midiToNoteName(int midi_number, AccidentalStyle style, OctaveNotation notation) const;
    
    int noteNameToMidi(const std::string& note_name) const;
    std::optional<int> tryNoteNameToMidi(const std::string& note_name) const;
    
    // Batch conversion methods
    std::vector<std::string> midiToNoteNames(const std::vector<int>& midi_numbers) const;
    std::vector<std::string> midiToNoteNames(const std::vector<int>& midi_numbers, AccidentalStyle style) const;
    
    std::vector<int> noteNamesToMidi(const std::vector<std::string>& note_names) const;
    std::vector<std::optional<int>> tryNoteNamesToMidi(const std::vector<std::string>& note_names) const;
    
    // Note information methods
    NoteInfo getNoteInfo(int midi_number) const;
    NoteInfo getNoteInfo(int midi_number, AccidentalStyle style) const;
    NoteInfo getNoteInfo(const std::string& note_name) const;
    
    // Note class operations (ignore octave)
    int getNoteClass(int midi_number) const { return midi_number % 12; }
    int getNoteClass(const std::string& note_name) const;
    std::string getNoteClassName(int note_class, AccidentalStyle style = AccidentalStyle::SHARPS) const;
    
    // Octave operations
    int getOctave(int midi_number) const { return (midi_number / 12) - 1; }
    int setOctave(int note_class, int octave) const { return note_class + (octave + 1) * 12; }
    
    // Enharmonic operations
    std::vector<std::string> getEnharmonicEquivalents(const std::string& note_name) const;
    std::vector<std::string> getEnharmonicEquivalents(int midi_number) const;
    bool areEnharmonicEquivalent(const std::string& note1, const std::string& note2) const;
    
    // Interval calculations
    int getInterval(const std::string& note1, const std::string& note2) const;
    int getInterval(int midi1, int midi2) const { return midi2 - midi1; }
    std::string transposeNote(const std::string& note_name, int semitones) const;
    int transposeNote(int midi_number, int semitones) const;
    
    // Configuration methods
    void setDefaultAccidentalStyle(AccidentalStyle style) { default_accidental_style = style; }
    AccidentalStyle getDefaultAccidentalStyle() const { return default_accidental_style; }
    
    void setDefaultOctaveNotation(OctaveNotation notation) { default_octave_notation = notation; }
    OctaveNotation getDefaultOctaveNotation() const { return default_octave_notation; }
    
    void setKeySignature(const std::string& key) { current_key_signature = key; }
    std::string getKeySignature() const { return current_key_signature; }
    
    // Validation methods
    bool isValidMidiNumber(int midi_number) const { return midi_number >= 0 && midi_number <= 127; }
    bool isValidNoteName(const std::string& note_name) const;
    bool isValidNoteClass(int note_class) const { return note_class >= 0 && note_class <= 11; }
    
    // Utility methods
    std::vector<std::string> getAllNoteNames(AccidentalStyle style = AccidentalStyle::SHARPS) const;
    std::vector<std::string> getChromaticScale(const std::string& start_note, AccidentalStyle style) const;
    
    // Performance methods
    void warmupCache() const; // Pre-populate any lazy-loaded data
    size_t getMemoryUsage() const;
    
    // Static utility methods
    static bool isSharpNote(int note_class) { return IS_SHARP[note_class]; }
    static bool isFlatNote(int note_class) { return IS_SHARP[note_class]; } // Same as sharp in 12-TET
    static bool isNaturalNote(int note_class) { return IS_NATURAL[note_class]; }
    static std::string normalizeNoteName(const std::string& note_name);
    
private:
    // Static data for parsing note names
    static const std::unordered_map<std::string, int> NOTE_NAME_TO_CLASS;
    static const std::unordered_map<std::string, int> FLAT_NOTE_TO_CLASS;
    
    // Helper for string parsing
    struct ParsedNote {
        int note_class;
        int octave;
        bool valid;
        
        ParsedNote() : note_class(-1), octave(-1), valid(false) {}
        ParsedNote(int nc, int oct) : note_class(nc), octave(oct), valid(true) {}
    };
    
    ParsedNote parseNoteComponents(const std::string& note_str) const;
};

// Inline implementations for hot path methods
inline std::string NoteConverter::midiToNoteName(int midi_number) const {
    return midiToNoteName(midi_number, default_accidental_style, default_octave_notation);
}

inline std::string NoteConverter::midiToNoteName(int midi_number, AccidentalStyle style) const {
    return midiToNoteName(midi_number, style, default_octave_notation);
}

inline std::string NoteConverter::getNoteClassName(int note_class, AccidentalStyle style) const {
    if (note_class < 0 || note_class >= 12) return "";
    
    const auto& names = (style == AccidentalStyle::FLATS) ? FLAT_NAMES : SHARP_NAMES;
    return std::string(names[note_class]);
}

inline int NoteConverter::transposeNote(int midi_number, int semitones) const {
    int result = midi_number + semitones;
    return (result >= 0 && result <= 127) ? result : -1;
}

inline std::vector<std::string> NoteConverter::midiToNoteNames(const std::vector<int>& midi_numbers) const {
    return midiToNoteNames(midi_numbers, default_accidental_style);
}

} // namespace ChordLock