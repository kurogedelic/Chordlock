#include "NoteConverter.h"
#include <algorithm>
#include <sstream>
#include <regex>

namespace ChordLock {

// Static initialization
const std::unordered_map<std::string, int> NoteConverter::NOTE_NAME_TO_CLASS = {
    {"C", 0}, {"C#", 1}, {"Db", 1}, {"D", 2}, {"D#", 3}, {"Eb", 3},
    {"E", 4}, {"F", 5}, {"F#", 6}, {"Gb", 6}, {"G", 7}, {"G#", 8},
    {"Ab", 8}, {"A", 9}, {"A#", 10}, {"Bb", 10}, {"B", 11}
};

const std::unordered_map<std::string, int> NoteConverter::FLAT_NOTE_TO_CLASS = {
    {"C", 0}, {"Db", 1}, {"D", 2}, {"Eb", 3}, {"E", 4}, {"F", 5},
    {"Gb", 6}, {"G", 7}, {"Ab", 8}, {"A", 9}, {"Bb", 10}, {"B", 11}
};

NoteConverter::NoteConverter() 
    : NoteConverter(AccidentalStyle::SHARPS, OctaveNotation::SCIENTIFIC) {
}

NoteConverter::NoteConverter(AccidentalStyle default_style, OctaveNotation default_notation)
    : default_accidental_style(default_style)
    , default_octave_notation(default_notation)
    , current_key_signature("C") {
    
    buildLookupTables();
    buildReverseMap();
    initializeKeyPreferences();
}

void NoteConverter::buildLookupTables() {
    // Pre-compute all MIDI note conversions for both sharp and flat styles
    for (int midi = 0; midi <= 127; ++midi) {
        // Sharp style
        midi_to_note_sharp[midi] = createNoteInfo(midi, AccidentalStyle::SHARPS);
        
        // Flat style
        midi_to_note_flat[midi] = createNoteInfo(midi, AccidentalStyle::FLATS);
    }
}

void NoteConverter::buildReverseMap() {
    // Build reverse lookup map
    for (int midi = 0; midi <= 127; ++midi) {
        int note_class = midi % 12;
        int octave = (midi / 12) - 1;
        
        // Add both sharp and flat representations
        std::string sharp_name = std::string(SHARP_NAMES[note_class]) + std::to_string(octave);
        std::string flat_name = std::string(FLAT_NAMES[note_class]) + std::to_string(octave);
        
        note_name_to_midi[sharp_name] = midi;
        if (sharp_name != flat_name) {
            note_name_to_midi[flat_name] = midi;
        }
        
        // Also add without octave for note class lookup
        if (octave == 4) { // Use middle octave as default
            note_name_to_midi[SHARP_NAMES[note_class]] = midi;
            if (std::string(SHARP_NAMES[note_class]) != std::string(FLAT_NAMES[note_class])) {
                note_name_to_midi[FLAT_NAMES[note_class]] = midi;
            }
        }
    }
}

void NoteConverter::initializeKeyPreferences() {
    // Sharp keys prefer sharps
    key_preferences["C"] = AccidentalStyle::SHARPS;
    key_preferences["G"] = AccidentalStyle::SHARPS;
    key_preferences["D"] = AccidentalStyle::SHARPS;
    key_preferences["A"] = AccidentalStyle::SHARPS;
    key_preferences["E"] = AccidentalStyle::SHARPS;
    key_preferences["B"] = AccidentalStyle::SHARPS;
    key_preferences["F#"] = AccidentalStyle::SHARPS;
    key_preferences["C#"] = AccidentalStyle::SHARPS;
    
    // Flat keys prefer flats
    key_preferences["F"] = AccidentalStyle::FLATS;
    key_preferences["Bb"] = AccidentalStyle::FLATS;
    key_preferences["Eb"] = AccidentalStyle::FLATS;
    key_preferences["Ab"] = AccidentalStyle::FLATS;
    key_preferences["Db"] = AccidentalStyle::FLATS;
    key_preferences["Gb"] = AccidentalStyle::FLATS;
    key_preferences["Cb"] = AccidentalStyle::FLATS;
}

NoteInfo NoteConverter::createNoteInfo(int midi_number, AccidentalStyle style) const {
    NoteInfo info;
    
    if (midi_number < 0 || midi_number > 127) {
        return info; // Invalid
    }
    
    info.midi_number = midi_number;
    info.note_class = midi_number % 12;
    info.octave = (midi_number / 12) - 1;
    info.is_natural = IS_NATURAL[info.note_class];
    info.is_sharp = IS_SHARP[info.note_class];
    info.is_flat = IS_SHARP[info.note_class]; // Same as sharp in 12-TET
    
    // Choose note name based on style
    const auto& note_names = (style == AccidentalStyle::FLATS) ? FLAT_NAMES : SHARP_NAMES;
    info.name_no_octave = note_names[info.note_class];
    info.name = info.name_no_octave + std::to_string(info.octave);
    
    return info;
}

std::string NoteConverter::midiToNoteName(int midi_number, AccidentalStyle style, OctaveNotation notation) const {
    if (!isValidMidiNumber(midi_number)) {
        return "";
    }
    
    // Choose style based on key context if MIXED
    AccidentalStyle actual_style = style;
    if (style == AccidentalStyle::MIXED) {
        actual_style = chooseAccidentalStyle(midi_number % 12, current_key_signature);
    }
    
    const NoteInfo& info = (actual_style == AccidentalStyle::FLATS) ? 
                           midi_to_note_flat[midi_number] : 
                           midi_to_note_sharp[midi_number];
    
    return formatNoteName(info, notation);
}

int NoteConverter::noteNameToMidi(const std::string& note_name) const {
    auto parsed = parseNoteComponents(note_name);
    if (!parsed.valid) {
        return -1;
    }
    
    return parsed.note_class + (parsed.octave + 1) * 12;
}

std::optional<int> NoteConverter::tryNoteNameToMidi(const std::string& note_name) const {
    int result = noteNameToMidi(note_name);
    return (result >= 0) ? std::optional<int>(result) : std::nullopt;
}

std::vector<std::string> NoteConverter::midiToNoteNames(const std::vector<int>& midi_numbers, AccidentalStyle style) const {
    std::vector<std::string> names;
    names.reserve(midi_numbers.size());
    
    for (int midi : midi_numbers) {
        names.push_back(midiToNoteName(midi, style));
    }
    
    return names;
}

std::vector<int> NoteConverter::noteNamesToMidi(const std::vector<std::string>& note_names) const {
    std::vector<int> midi_numbers;
    midi_numbers.reserve(note_names.size());
    
    for (const auto& name : note_names) {
        int midi = noteNameToMidi(name);
        if (midi >= 0) {
            midi_numbers.push_back(midi);
        }
    }
    
    return midi_numbers;
}

std::vector<std::optional<int>> NoteConverter::tryNoteNamesToMidi(const std::vector<std::string>& note_names) const {
    std::vector<std::optional<int>> results;
    results.reserve(note_names.size());
    
    for (const auto& name : note_names) {
        results.push_back(tryNoteNameToMidi(name));
    }
    
    return results;
}

NoteInfo NoteConverter::getNoteInfo(int midi_number) const {
    return getNoteInfo(midi_number, default_accidental_style);
}

NoteInfo NoteConverter::getNoteInfo(int midi_number, AccidentalStyle style) const {
    if (!isValidMidiNumber(midi_number)) {
        return NoteInfo(); // Invalid
    }
    
    return (style == AccidentalStyle::FLATS) ? 
           midi_to_note_flat[midi_number] : 
           midi_to_note_sharp[midi_number];
}

NoteInfo NoteConverter::getNoteInfo(const std::string& note_name) const {
    int midi = noteNameToMidi(note_name);
    if (midi < 0) {
        return NoteInfo(); // Invalid
    }
    
    return getNoteInfo(midi);
}

int NoteConverter::getNoteClass(const std::string& note_name) const {
    auto parsed = parseNoteComponents(note_name);
    return parsed.valid ? parsed.note_class : -1;
}

std::vector<std::string> NoteConverter::getEnharmonicEquivalents(const std::string& note_name) const {
    int midi = noteNameToMidi(note_name);
    if (midi < 0) {
        return {};
    }
    
    return getEnharmonicEquivalents(midi);
}

std::vector<std::string> NoteConverter::getEnharmonicEquivalents(int midi_number) const {
    if (!isValidMidiNumber(midi_number)) {
        return {};
    }
    
    std::vector<std::string> equivalents;
    
    // Add sharp version
    equivalents.push_back(midiToNoteName(midi_number, AccidentalStyle::SHARPS));
    
    // Add flat version if different
    std::string flat_version = midiToNoteName(midi_number, AccidentalStyle::FLATS);
    if (flat_version != equivalents[0]) {
        equivalents.push_back(flat_version);
    }
    
    return equivalents;
}

bool NoteConverter::areEnharmonicEquivalent(const std::string& note1, const std::string& note2) const {
    int midi1 = noteNameToMidi(note1);
    int midi2 = noteNameToMidi(note2);
    
    return (midi1 >= 0 && midi2 >= 0 && midi1 == midi2);
}

int NoteConverter::getInterval(const std::string& note1, const std::string& note2) const {
    int midi1 = noteNameToMidi(note1);
    int midi2 = noteNameToMidi(note2);
    
    if (midi1 < 0 || midi2 < 0) {
        return -1000; // Invalid interval
    }
    
    return midi2 - midi1;
}

std::string NoteConverter::transposeNote(const std::string& note_name, int semitones) const {
    int midi = noteNameToMidi(note_name);
    if (midi < 0) {
        return "";
    }
    
    int new_midi = transposeNote(midi, semitones);
    if (new_midi < 0) {
        return "";
    }
    
    return midiToNoteName(new_midi);
}

std::string NoteConverter::formatNoteName(const NoteInfo& info, OctaveNotation notation) const {
    switch (notation) {
        case OctaveNotation::SCIENTIFIC:
            return info.name;
        case OctaveNotation::HELMHOLTZ:
            // Simplified Helmholtz notation
            return info.name_no_octave + "'";
        case OctaveNotation::MIDI_NUMBER:
            return std::to_string(info.midi_number);
        case OctaveNotation::NO_OCTAVE:
            return info.name_no_octave;
        default:
            return info.name;
    }
}

NoteConverter::ParsedNote NoteConverter::parseNoteComponents(const std::string& note_str) const {
    ParsedNote parsed;
    
    if (note_str.empty()) {
        return parsed;
    }
    
    // Regular expression to parse note name
    // Matches: C, C#, Db, C#4, Db-1, etc.
    std::regex note_regex(R"(([A-G])([#b]?)(-?\d+)?)");
    std::smatch match;
    
    if (!std::regex_match(note_str, match, note_regex)) {
        return parsed;
    }
    
    // Extract components
    std::string base_note = match[1].str();
    std::string accidental = match[2].str();
    std::string octave_str = match[3].str();
    
    // Convert base note to number
    std::unordered_map<std::string, int> base_to_class = {
        {"C", 0}, {"D", 2}, {"E", 4}, {"F", 5}, {"G", 7}, {"A", 9}, {"B", 11}
    };
    
    auto it = base_to_class.find(base_note);
    if (it == base_to_class.end()) {
        return parsed;
    }
    
    parsed.note_class = it->second;
    
    // Apply accidental
    if (accidental == "#") {
        parsed.note_class = (parsed.note_class + 1) % 12;
    } else if (accidental == "b") {
        parsed.note_class = (parsed.note_class - 1 + 12) % 12;
    }
    
    // Parse octave (default to 4 if not specified)
    if (!octave_str.empty()) {
        try {
            parsed.octave = std::stoi(octave_str);
        } catch (const std::exception&) {
            return parsed; // Invalid octave
        }
    } else {
        parsed.octave = 4; // Default octave
    }
    
    // Validate ranges
    if (parsed.octave < -1 || parsed.octave > 9) {
        return parsed;
    }
    
    parsed.valid = true;
    return parsed;
}

AccidentalStyle NoteConverter::chooseAccidentalStyle(int note_class, const std::string& key_context) const {
    auto it = key_preferences.find(key_context);
    if (it != key_preferences.end()) {
        return it->second;
    }
    
    // For chromatic notes, use musical convention
    // Bb (note_class 10) is more commonly used than A#
    if (note_class == 10) { // Bb/A#
        return AccidentalStyle::FLATS;
    }
    
    // Eb (note_class 3) is more common than D#
    if (note_class == 3) { // Eb/D#
        return AccidentalStyle::FLATS;
    }
    
    // Ab (note_class 8) is more common than G#
    if (note_class == 8) { // Ab/G#
        return AccidentalStyle::FLATS;
    }
    
    // Default to sharps for other chromatic notes
    return AccidentalStyle::SHARPS;
}

bool NoteConverter::isValidNoteName(const std::string& note_name) const {
    return parseNoteComponents(note_name).valid;
}

std::vector<std::string> NoteConverter::getAllNoteNames(AccidentalStyle style) const {
    std::vector<std::string> names;
    names.reserve(12);
    
    const auto& note_names = (style == AccidentalStyle::FLATS) ? FLAT_NAMES : SHARP_NAMES;
    
    for (int i = 0; i < 12; ++i) {
        names.push_back(note_names[i]);
    }
    
    return names;
}

std::vector<std::string> NoteConverter::getChromaticScale(const std::string& start_note, AccidentalStyle style) const {
    int start_midi = noteNameToMidi(start_note);
    if (start_midi < 0) {
        return {};
    }
    
    std::vector<std::string> scale;
    scale.reserve(12);
    
    for (int i = 0; i < 12; ++i) {
        int midi = start_midi + i;
        if (midi <= 127) {
            scale.push_back(midiToNoteName(midi, style));
        }
    }
    
    return scale;
}

std::string NoteConverter::normalizeNoteName(const std::string& note_name) {
    // Remove extra whitespace and standardize format
    std::string normalized = note_name;
    
    // Remove whitespace
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::isspace), normalized.end());
    
    // Convert to uppercase for the note letter
    if (!normalized.empty()) {
        normalized[0] = std::toupper(normalized[0]);
    }
    
    return normalized;
}

void NoteConverter::warmupCache() const {
    // All lookup tables are pre-computed, so nothing to warm up
}

size_t NoteConverter::getMemoryUsage() const {
    // Rough estimate of memory usage
    size_t size = 0;
    size += sizeof(midi_to_note_sharp);
    size += sizeof(midi_to_note_flat);
    size += note_name_to_midi.size() * (sizeof(std::string) + sizeof(int));
    size += key_preferences.size() * (sizeof(std::string) + sizeof(AccidentalStyle));
    return size;
}

} // namespace ChordLock