#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <string>
#include <memory>
#include "Core/ChordIdentifier.h"
#include "Core/AdvancedChordRecognition.h"
#include "Utils/NoteConverter.h"

using namespace emscripten;
using namespace ChordLock;

class ChordLockWASM {
private:
    std::unique_ptr<ChordIdentifier> identifier;
    std::unique_ptr<AdvancedChordRecognition> advancedRecognizer;
    std::unique_ptr<NoteConverter> noteConverter;
    
public:
    ChordLockWASM() {
        identifier = std::make_unique<ChordIdentifier>(IdentificationMode::COMPREHENSIVE);
        advancedRecognizer = std::make_unique<AdvancedChordRecognition>();
        noteConverter = std::make_unique<NoteConverter>();
        
        // Initialize with compiled tables (no YAML in WASM)
        identifier->initialize("", "");
    }
    
    // Basic chord identification
    val identifyChord(const val& notes_array) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        if (notes.empty()) {
            return val::object();
        }
        
        auto result = identifier->identify(notes);
        
        // Convert result to JavaScript object
        val js_result = val::object();
        js_result.set("chordName", result.chord_name);
        js_result.set("fullName", result.full_display_name);
        js_result.set("symbol", result.chord_symbol);
        js_result.set("rootNote", result.root_note);
        js_result.set("bassNote", result.bass_note_name);
        js_result.set("quality", result.chord_quality);
        js_result.set("category", result.chord_category);
        js_result.set("confidence", result.confidence);
        js_result.set("isInversion", result.is_inversion);
        js_result.set("isSlashChord", result.is_slash_chord);
        js_result.set("inversionType", result.inversion_type);
        
        // Convert intervals to JS array
        val intervals = val::array();
        for (size_t i = 0; i < result.identified_intervals.size(); ++i) {
            intervals.set(i, result.identified_intervals[i]);
        }
        js_result.set("intervals", intervals);
        
        // Convert note names to JS array
        val noteNames = val::array();
        for (size_t i = 0; i < result.note_names.size(); ++i) {
            noteNames.set(i, result.note_names[i]);
        }
        js_result.set("noteNames", noteNames);
        
        // Add alternatives
        val alternatives = val::array();
        for (size_t i = 0; i < result.alternative_names.size(); ++i) {
            alternatives.set(i, result.alternative_names[i]);
        }
        js_result.set("alternatives", alternatives);
        
        // Processing time in microseconds
        js_result.set("processingTime", result.processing_time.count());
        
        return js_result;
    }
    
    // Advanced jazz chord recognition
    val identifyJazzChord(const val& notes_array) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        auto result = advancedRecognizer->recognize(notes, 
            AdvancedChordRecognition::RecognitionMode::JAZZ);
        
        val js_result = val::object();
        js_result.set("primaryChord", result.primary_chord);
        js_result.set("secondaryChord", result.secondary_chord);
        js_result.set("bassNote", result.bass_note);
        js_result.set("confidence", result.confidence);
        js_result.set("isPolychord", result.is_polychord);
        js_result.set("isQuartal", result.is_quartal);
        js_result.set("isCluster", result.is_cluster);
        js_result.set("isRootless", result.is_rootless_voicing);
        js_result.set("hasAlteredExtensions", result.has_altered_extensions);
        js_result.set("tonalAmbiguity", result.tonal_ambiguity);
        
        // Extensions
        val extensions = val::array();
        for (size_t i = 0; i < result.extensions.size(); ++i) {
            extensions.set(i, result.extensions[i]);
        }
        js_result.set("extensions", extensions);
        
        // Alterations
        val alterations = val::array();
        for (size_t i = 0; i < result.alterations.size(); ++i) {
            alterations.set(i, result.alterations[i]);
        }
        js_result.set("alterations", alterations);
        
        return js_result;
    }
    
    // Detect polychords
    val detectPolychord(const val& notes_array) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        auto polychord = advancedRecognizer->detectPolychord(notes);
        
        if (!polychord.has_value()) {
            return val::null();
        }
        
        val js_result = val::object();
        
        val lower = val::object();
        lower.set("chord", polychord->first.primary_chord);
        lower.set("confidence", polychord->first.confidence);
        js_result.set("lower", lower);
        
        val upper = val::object();
        upper.set("chord", polychord->second.primary_chord);
        upper.set("confidence", polychord->second.confidence);
        js_result.set("upper", upper);
        
        return js_result;
    }
    
    // Detect quartal harmony
    val detectQuartalHarmony(const val& notes_array) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        auto result = advancedRecognizer->detectQuartalHarmony(notes);
        
        val js_result = val::object();
        js_result.set("isQuartal", result.is_quartal);
        js_result.set("chordName", result.primary_chord);
        js_result.set("confidence", result.confidence);
        
        return js_result;
    }
    
    // Convert MIDI note to note name
    std::string midiToNoteName(int midi_note) {
        return noteConverter->midiToNoteName(midi_note);
    }
    
    // Convert note name to MIDI
    int noteNameToMidi(const std::string& note_name) {
        return noteConverter->noteNameToMidi(note_name);
    }
    
    // Get all intervals from a chord
    val getIntervals(const val& notes_array) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        if (notes.size() < 2) {
            return val::array();
        }
        
        std::sort(notes.begin(), notes.end());
        int root = notes[0];
        
        val intervals = val::array();
        for (size_t i = 0; i < notes.size(); ++i) {
            intervals.set(i, (notes[i] - root) % 12);
        }
        
        return intervals;
    }
    
    // Transpose chord
    val transposeChord(const val& notes_array, int semitones) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        val transposed = val::array();
        for (size_t i = 0; i < notes.size(); ++i) {
            int new_note = notes[i] + semitones;
            // Clamp to MIDI range
            new_note = std::max(0, std::min(127, new_note));
            transposed.set(i, new_note);
        }
        
        return transposed;
    }
    
    // Get chord suggestions (simplified for WASM)
    val getChordSuggestions(const val& notes_array) {
        std::vector<int> notes = vecFromJSArray<int>(notes_array);
        
        val suggestions = val::array();
        
        // Common chord progressions
        auto result = identifier->identify(notes);
        
        if (result.chord_name.find("major") != std::string::npos) {
            suggestions.set(0, "ii (Subdominant)");
            suggestions.set(1, "V (Dominant)");
            suggestions.set(2, "vi (Relative minor)");
        } else if (result.chord_name.find("minor") != std::string::npos) {
            suggestions.set(0, "iv (Subdominant minor)");
            suggestions.set(1, "V (Dominant)");
            suggestions.set(2, "VII (Subtonic)");
        } else if (result.chord_name.find("dominant") != std::string::npos) {
            suggestions.set(0, "I (Tonic resolution)");
            suggestions.set(1, "vi (Deceptive cadence)");
            suggestions.set(2, "IV (Plagal)");
        }
        
        return suggestions;
    }
    
    // Performance benchmark
    val runBenchmark() {
        std::vector<std::vector<int>> test_chords = {
            {60, 64, 67},        // C major
            {60, 63, 67},        // C minor
            {60, 64, 67, 70},    // C7
            {60, 64, 67, 71},    // Cmaj7
            {60, 64, 67, 70, 62} // C9
        };
        
        val results = val::array();
        
        for (size_t i = 0; i < test_chords.size(); ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int j = 0; j < 1000; ++j) {
                identifier->identify(test_chords[i]);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            val result = val::object();
            result.set("chordIndex", (int)i);
            result.set("avgTime", duration.count() / 1000.0);
            results.set(i, result);
        }
        
        return results;
    }
    
    // Get version info
    val getVersion() {
        val info = val::object();
        info.set("version", "1.0.0");
        info.set("engine", "ChordLock WASM");
        info.set("chordCount", 500);
        info.set("features", val::array());
        
        val features = info["features"];
        features.set(0, "Basic chord recognition");
        features.set(1, "Jazz extensions");
        features.set(2, "Quartal harmony");
        features.set(3, "Polychords");
        features.set(4, "Inversions");
        features.set(5, "Slash chords");
        
        return info;
    }
};

// Emscripten bindings
EMSCRIPTEN_BINDINGS(chordlock_module) {
    class_<ChordLockWASM>("ChordLock")
        .constructor()
        .function("identifyChord", &ChordLockWASM::identifyChord)
        .function("identifyJazzChord", &ChordLockWASM::identifyJazzChord)
        .function("detectPolychord", &ChordLockWASM::detectPolychord)
        .function("detectQuartalHarmony", &ChordLockWASM::detectQuartalHarmony)
        .function("midiToNoteName", &ChordLockWASM::midiToNoteName)
        .function("noteNameToMidi", &ChordLockWASM::noteNameToMidi)
        .function("getIntervals", &ChordLockWASM::getIntervals)
        .function("transposeChord", &ChordLockWASM::transposeChord)
        .function("getChordSuggestions", &ChordLockWASM::getChordSuggestions)
        .function("runBenchmark", &ChordLockWASM::runBenchmark)
        .function("getVersion", &ChordLockWASM::getVersion);
}