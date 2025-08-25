#include "ChordNameGenerator.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <iostream>

namespace ChordLock {

// Static chord symbol definitions
const std::unordered_map<std::string, std::string>& ChordNameGenerator::getJazzSymbols() {
    static const std::unordered_map<std::string, std::string> symbols = {
        {"major-triad", ""},
        {"minor-triad", "m"},
        {"diminished-triad", "°"},
        {"augmented-triad", "+"},
        {"sus4-triad", "sus4"},
        {"sus2-triad", "sus2"},
        
        {"dominant-seventh", "7"},
        {"major-seventh", "M7"},
        {"minor-seventh", "m7"},
        {"diminished-seventh", "°7"},
        {"half-diminished-seventh", "ø7"},
        {"minor-major-seventh", "m(M7)"},
        
        {"dominant-ninth", "9"},
        {"major-ninth", "M9"},
        {"minor-ninth", "m9"},
        
        {"dominant-eleventh", "11"},
        {"dominant-eleventh-omit9", "11"},
        {"dominant-eleventh-standard", "11"},
        {"major-eleventh", "M11"},
        {"minor-eleventh", "m11"},
        
        {"dominant-thirteenth", "13"},
        {"dominant-thirteenth-omit9", "13"},
        {"major-thirteenth", "M13"},
        {"minor-thirteenth", "m13"},
        
        {"dominant-seventh-flat9", "7♭9"},
        {"dominant-seventh-sharp9", "7♯9"},
        {"dominant-seventh-sharp11", "7♯11"},
        {"dominant-seventh-flat13", "7♭13"},
        {"dominant-seventh-flat9-sharp11", "7♭9♯11"},
        {"dominant-seventh-sharp5", "7♯5"},
        {"dominant-seventh-sharp11-clean", "7♯11"},
        
        {"major-sixth", "6"},
        {"minor-sixth", "m6"},
        {"six-nine", "6/9"},
        {"add11", "add11"},
        
        {"single-note", ""},
        {"octave", ""},
        
        {"major-ninth-sharp11", "M9♯11"},
        {"minor-ninth-flat13", "m9♭13"},
        {"dominant-altered-scale", "7alt"},
        
        // Add chords
        {"add9", "add9"},
        {"minor-add9", "madd9"},
        {"add11", "add11"},
        {"add13", "add13"},
        
        // Omit chords
        {"major-omit3", "(omit3)"},
        {"minor-omit3", "m(omit3)"},
        {"major-omit5", "(omit5)"},
        {"minor-omit5", "m(omit5)"},
        
        // Complex extended chords
        {"dominant-eleventh-sharp11", "7(9,♯11)"},
        {"dominant-sharp11-thirteenth", "7(♯11,13)"},
        {"dominant-flat9-sharp13", "7(♭9,♯13)"},
        {"minor-flat9-sharp13", "m7(♭9,♯13)"},
        {"minor-flat9-sharp11", "m7(♭9,♯11)"},
        {"major-ninth-add11", "M9(add11)"},
        {"dominant-ninth-omit5-add13", "7(9,13,omit5)"},
        {"dominant-sharp11", "7♯11"},
        {"dominant-thirteenth-omit9", "7(13,omit9)"},
        
        // Critical altered chord symbols for Section 6
        {"dominant-seventh-flat9-sharp11", "7(♭9,♯11)"},
        {"dominant-seventh-flat9-sharp13", "7(♭9,♯13)"},
        {"dominant-seventh-flat9-sharp9", "7(♭9,♯9)"},
        {"dominant-seventh-sharp9-sharp11", "7(♯9,♯11)"},
        {"dominant-seventh-sharp9-sharp13", "7(♯9,♯13)"},
        {"dominant-seventh-flat5-flat9-sharp11", "7(♭5,♭9,♯11)"},
        {"dominant-seventh-sharp5-flat9-sharp11", "7(♯5,♭9,♯11)"},
        {"dominant-seventh-flat5-sharp9-sharp11", "7(♭5,♯9,♯11)"},
        
        // Sus and sixth chords
        {"sus4-add9", "sus4(add9)"},
        {"minor-six-nine", "m6/9"},
        
        // Contemporary harmony symbols
        {"quartal-triad", "4th"},
        {"quartal-modern", "4th"},
        {"tritone-major7", "♭5M7"},
        {"microtonal-cluster", "μ"},
        {"chromatic-3-cluster", "3♭2"},
        {"semitone-tritone-cluster", "♭2♭5"},
        {"minor-major-quartal", "m4th"},
        {"diminished-major7", "°M7"},
        {"chromatic-fifth-cluster", "♭2♭6"},
        {"tritone-fifth-major7", "♭5♭6M7"},
        
        // Advanced contemporary harmony
        {"super-locrian-hexachord", "LocHex"},
        {"lydian-augmented-sixth", "Lyd♯5/6"},
        {"octatonic-fragment", "Oct"},
        
        // Advanced jazz harmony symbols
        {"major-thirteenth-add11", "M13(add11)"},
        {"minor-thirteenth-add11", "m13(add11)"},
        {"altered-dominant-scale-fragment", "7alt"},
        {"whole-tone-dominant", "7+11"},
        {"tritone-substitution-chord", "SubV7"},
        {"lydian-dominant-fragment", "7♯11"},
        {"diminished-whole-tone", "°WT"},
        {"diminished-dominant", "°7"},
        {"quartal-dominant", "7sus"},
        {"minor-major-ninth-sharp11", "m(M9♯11)"},
        
        // Bebop harmony symbols
        {"bebop-dominant-fragment", "7(♭9,9)"},
        {"bebop-minor-fragment", "m7(♭9)"},
        {"bebop-major-fragment", "M7(♯5)"},
        {"bebop-blues-scale", "7blues"},
        {"harmonic-minor-bebop", "m7(♮7)"},
        {"altered-bebop-scale", "7alt(♭9)"},
        
        // Modern jazz voicing symbols
        {"tritone-sub-shell", "SubV"},
        {"symmetric-diminished", "°"},
        {"spread-triad", "spread"},
        {"so-what-voicing", "so"},
        {"upper-structure", "US"},
        {"quartal-voicing-basic", "4th"},
        {"quartal-stack", "4ths"},
        {"minor-eleventh-no-five", "m11(omit5)"},
        
        // Contemporary cluster symbols
        {"minor-second-cluster", "♭2♭6"},
        {"tritone-cluster", "♭5♭6"},
        {"chromatic-edge-cluster", "♭2♯7"},
        {"chromatic-tetrachord", "♭2♭3"},
        {"symmetric-cluster", "sym"},
        
        // Microtonal and quarter-tone symbols
        {"quarter-tone-triad", "qt3"},
        {"quarter-tone-minor", "qtm"},
        {"quarter-tone-neutral", "qtn"},
        {"quarter-tone-augmented", "qt+"},
        {"quarter-tone-seventh", "qt7"},
        {"quarter-tone-wide-third", "qt♯3"},
        {"quarter-tone-tetrachord", "qt4"},
        {"quarter-tone-spread", "qtsp"},
        {"microtonal-tetrachord", "μ4"},
        {"microtonal-cluster-wide", "μcl"},
        {"microtonal-pentachord", "μ5"},
        {"microtonal-scale-fragment", "μsc"},
        {"microtonal-pentatonic", "μpent"},
        {"microtonal-hexachord", "μ6"}
    };
    return symbols;
}

const std::unordered_map<std::string, std::string>& ChordNameGenerator::getPopularSymbols() {
    static const std::unordered_map<std::string, std::string> symbols = {
        {"major-triad", ""},
        {"minor-triad", "m"},
        {"diminished-triad", "dim"},
        {"augmented-triad", "aug"},
        {"sus4-triad", "sus4"},
        {"sus2-triad", "sus2"},
        
        {"dominant-seventh", "7"},
        {"major-seventh", "maj7"},
        {"minor-seventh", "m7"},
        {"diminished-seventh", "dim7"},
        {"half-diminished-seventh", "m7♭5"},
        
        // Add chords
        {"add9", "add9"},
        {"minor-add9", "madd9"},
        {"add11", "add11"},
        {"add13", "add13"}
    };
    return symbols;
}

const std::unordered_map<std::string, std::string>& ChordNameGenerator::getClassicalSymbols() {
    return getJazzSymbols(); // Use jazz symbols for now
}

ChordNameGenerator::ChordNameGenerator(NamingStyle style, KeyContext key)
    : current_style(style), current_key_context(key) {
    // Use MIXED style to allow context-aware accidental selection
    note_converter = std::make_unique<NoteConverter>(AccidentalStyle::MIXED);
}

ChordNameResult ChordNameGenerator::generateChordName(const ChordMatch& match,
                                                     const std::vector<int>& midi_notes,
                                                     const std::vector<int>& intervals) const {
    ChordNameResult result;
    
    // Step 1: Use root from ChordMatch (enrichChordMatch should have set this correctly)
    int theoretical_root = match.root_note_midi;
    
    // If enrichChordMatch didn't set the root, try to detect it
    if (theoretical_root == -1) {
        int root_offset = detectTheoreticalRoot(intervals, match.chord_info.name);
        if (root_offset != -1 && match.bass_note_midi != -1) {
            // Calculate theoretical root by adding offset to bass note
            theoretical_root = (match.bass_note_midi + root_offset) % 12 + (match.bass_note_midi / 12) * 12;
        } else {
            theoretical_root = match.bass_note_midi; // Fallback to bass
        }
    }
    
    // Step 2: Analyze key context for accidental choice
    KeyContext effective_key = (current_key_context == KeyContext::AUTO_DETECT) 
                              ? analyzeKeyContext(midi_notes) 
                              : current_key_context;
    
    AccidentalStyle accidental_style = getAccidentalStyleForKey(effective_key);
    
    // Step 3: Generate root note name
    result.root_note = note_converter->midiToNoteName(theoretical_root, accidental_style, OctaveNotation::NO_OCTAVE);
    
    // Step 4: Generate chord symbol
    result.chord_symbol = generateChordSymbol(match.chord_info.name, current_style);
    
    // Step 5: Analyze inversion and slash chord requirements
    result.inversion_type = analyzeInversion(intervals, match.chord_info.name);
    
    // Only consider it a slash chord if bass is different from root
    bool bass_differs_from_root = (match.bass_note_midi != theoretical_root) && 
                                 (match.bass_note_midi != -1) && (theoretical_root != -1);
    
    // Calculate bass interval correctly using modulo 12
    int bass_interval = 0;
    if (bass_differs_from_root) {
        bass_interval = (match.bass_note_midi - theoretical_root + 12) % 12;
    }
    
    // Force slash notation for detected inversions regardless of other checks
    result.is_slash_chord = bass_differs_from_root || match.is_slash_chord || 
                           shouldUseSlashNotation(match.chord_info.name, result.inversion_type, bass_interval);
    
    // Step 6: Handle bass note for slash chords
    if (result.is_slash_chord && match.bass_note_midi != -1) {
        // For Bb (MIDI 58, 70, 82, etc.), always use flat notation
        int bass_note_class = match.bass_note_midi % 12;
        AccidentalStyle bass_style = accidental_style;
        if (bass_note_class == 10) { // Bb/A#
            bass_style = AccidentalStyle::FLATS;
        }
        result.bass_note = note_converter->midiToNoteName(match.bass_note_midi, bass_style, OctaveNotation::NO_OCTAVE);
    }
    
    // Step 7: Format final chord name
    result.chord_name = result.root_note + result.chord_symbol;
    result.full_name = formatChordName(result.root_note, result.chord_symbol, result.bass_note);
    result.confidence = match.confidence;
    
    return result;
}

int ChordNameGenerator::detectTheoreticalRoot(const std::vector<int>& intervals, const std::string& chord_type) const {
    if (intervals.empty()) return -1;
    
    // For inverted chords, we need to calculate the theoretical root based on interval patterns
    // The intervals are normalized with bass note = 0
    
    if (chord_type == "major-triad" || chord_type == "minor-triad") {
        if (intervals.size() >= 3) {
            // First inversion patterns: [0,3,8] means bass(0) + minor3rd(3) + minor6th(8)
            // This indicates bass is the major 3rd of the original chord
            if (intervals == std::vector<int>{0, 3, 8}) {
                // Bass is 3rd of major triad, root is 4 semitones DOWN (or 8 UP)
                return 8; // Return offset to add to bass note to get root
            }
            // First inversion minor: [0,4,9] means bass(0) + major3rd(4) + minor6th(9)  
            if (intervals == std::vector<int>{0, 4, 9}) {
                // Bass is 3rd of minor triad, root is 3 semitones DOWN (or 9 UP)
                return 9; // Return offset to add to bass note to get root
            }
            // Second inversion patterns: [0,5,9] or [0,5,8]
            if (intervals == std::vector<int>{0, 5, 9}) {
                // Bass is 5th of major triad, root is 7 semitones DOWN (or 5 UP)
                return 5; // Return offset to add to bass note to get root
            }
            if (intervals == std::vector<int>{0, 5, 8}) {
                // Bass is 5th of minor triad, root is 7 semitones DOWN (or 5 UP)  
                return 5; // Return offset to add to bass note to get root
            }
        }
    }
    
    // For seventh chords, more complex logic
    if (chord_type.find("seventh") != std::string::npos) {
        return detectRootFromIntervalPattern(intervals);
    }
    
    // Default: return -1 to indicate no specific root detected
    return -1;
}

int ChordNameGenerator::detectRootFromIntervalPattern(const std::vector<int>& intervals) const {
    // Advanced root detection using interval pattern analysis
    if (intervals.empty()) return -1;
    
    // intervals[0] is always 0 in normalized intervals
    
    // Look for perfect fifth (7 semitones) - strong indicator of root relationship
    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i] == 7) {
            return -1; // Bass is likely root if perfect fifth present
        }
    }
    
    // Look for major third (4 semitones) - second strongest indicator
    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i] == 4) {
            return -1; // Bass is likely root if major third present
        }
    }
    
    // Analyze chord structure to determine most likely root
    // This is where music theory knowledge becomes crucial
    
    return -1; // Default fallback - no specific root detected
}

KeyContext ChordNameGenerator::analyzeKeyContext(const std::vector<int>& midi_notes) const {
    if (midi_notes.empty()) return KeyContext::CHROMATIC;
    
    // Count sharps vs flats tendency in the chord
    int sharp_tendency = 0;
    int flat_tendency = 0;
    
    for (int note : midi_notes) {
        int note_class = note % 12;
        
        // Notes that suggest sharp keys: F#, C#, G#, D#, A#
        if (note_class == 1 || note_class == 6 || note_class == 8 || note_class == 3 || note_class == 10) {
            sharp_tendency++;
        }
        // Notes that suggest flat keys: Bb, Eb, Ab, Db, Gb
        if (note_class == 10 || note_class == 3 || note_class == 8 || note_class == 1 || note_class == 6) {
            flat_tendency++;
        }
    }
    
    // Simple heuristic for key context
    if (sharp_tendency > flat_tendency) {
        return KeyContext::G_MAJOR; // Sharp tendency
    } else if (flat_tendency > sharp_tendency) {
        return KeyContext::F_MAJOR; // Flat tendency  
    } else {
        return KeyContext::C_MAJOR; // Neutral
    }
}

AccidentalStyle ChordNameGenerator::getAccidentalStyleForKey(KeyContext key) const {
    switch (key) {
        case KeyContext::F_MAJOR:
            return AccidentalStyle::FLATS;
        case KeyContext::G_MAJOR:
            return AccidentalStyle::SHARPS;
        case KeyContext::C_MAJOR:
        case KeyContext::AUTO_DETECT:
        case KeyContext::CHROMATIC:
        default:
            return AccidentalStyle::SHARPS; // Default preference
    }
}

std::string ChordNameGenerator::generateChordSymbol(const std::string& chord_type, NamingStyle style) const {
    const auto* symbol_map = &getJazzSymbols();
    
    switch (style) {
        case NamingStyle::JAZZ:
            symbol_map = &getJazzSymbols();
            break;
        case NamingStyle::CLASSICAL:
            symbol_map = &getClassicalSymbols();
            break;
        case NamingStyle::POPULAR:
            symbol_map = &getPopularSymbols();
            break;
        case NamingStyle::MINIMAL:
            symbol_map = &getPopularSymbols();
            break;
    }
    
    auto it = symbol_map->find(chord_type);
    if (it != symbol_map->end()) {
        return it->second;
    }
    
    // Fallback: try to parse chord type and generate symbol
    if (chord_type.find("major") != std::string::npos && chord_type.find("triad") != std::string::npos) {
        return "";
    }
    if (chord_type.find("minor") != std::string::npos && chord_type.find("triad") != std::string::npos) {
        return "m";
    }
    
    return ""; // Unknown chord type
}

int ChordNameGenerator::analyzeInversion(const std::vector<int>& intervals, const std::string& chord_type) const {
    if (intervals.empty()) return 0;
    
    // Root position if starts with 0
    if (intervals[0] == 0) return 0;
    
    // For triads
    if (chord_type.find("triad") != std::string::npos && intervals.size() >= 3) {
        // Check specific inversion patterns
        if ((intervals == std::vector<int>{0, 3, 8}) || (intervals == std::vector<int>{0, 4, 9})) {
            return 1; // First inversion
        }
        if ((intervals == std::vector<int>{0, 5, 9}) || (intervals == std::vector<int>{0, 5, 8})) {
            return 2; // Second inversion
        }
    }
    
    // For seventh chords - more complex analysis needed
    if (chord_type.find("seventh") != std::string::npos) {
        // Simplified: any non-root bass is considered "some inversion"
        return intervals[0] != 0 ? 1 : 0;
    }
    
    return 0;
}

bool ChordNameGenerator::shouldUseSlashNotation(const std::string& chord_type, int inversion, int bass_interval) const {
    // Never use slash notation for augmented triads due to symmetry
    if (chord_type.find("augmented") != std::string::npos) {
        return false;
    }
    
    // Use slash notation for clear inversions of triads and seventh chords
    if (chord_type.find("triad") != std::string::npos && inversion > 0) {
        return true;
    }
    
    if (chord_type.find("seventh") != std::string::npos && inversion > 0) {
        return true;
    }
    
    // Always use slash notation for non-chord tones in bass (not root, 3rd, 5th, or 7th)
    if (bass_interval != 0 && bass_interval != 3 && bass_interval != 4 && bass_interval != 7 && bass_interval != 10) {
        return true;
    }
    
    // For complex chords, use slash notation more conservatively
    if (chord_type.find("ninth") != std::string::npos || 
        chord_type.find("eleventh") != std::string::npos ||
        chord_type.find("thirteenth") != std::string::npos) {
        return false; // Complex chords rarely use slash notation for inversions
    }
    
    return false;
}

std::string ChordNameGenerator::formatChordName(const std::string& root_note, 
                                               const std::string& symbol, 
                                               const std::string& bass_note) const {
    std::string result = root_note + symbol;
    
    if (!bass_note.empty() && bass_note != root_note) {
        result += "/" + bass_note;
    }
    
    return result;
}

std::string ChordNameGenerator::getChordSymbol(const std::string& chord_type) const {
    return generateChordSymbol(chord_type, current_style);
}

bool ChordNameGenerator::isValidChordName(const std::string& chord_name) const {
    return !chord_name.empty() && chord_name != "UNKNOWN";
}

void ChordNameGenerator::warmupCache() const {
    // Pre-compute common chord symbols
    for (const auto& [chord_type, symbol] : getJazzSymbols()) {
        generateChordSymbol(chord_type, NamingStyle::JAZZ);
    }
}

size_t ChordNameGenerator::getCacheSize() const {
    return 0; // No explicit cache in current implementation
}

} // namespace ChordLock