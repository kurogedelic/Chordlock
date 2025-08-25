#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <chrono>
#include "IntervalEngine.h"
#include "ChordDatabase.h"
#include "ChordNameGenerator.h"
#include "ErrorHandling.h"
#include "../Utils/NoteConverter.h"

namespace ChordLock {

enum class IdentificationMode {
    FAST,           // Fastest, exact matches only
    STANDARD,       // Standard mode with inversions
    COMPREHENSIVE,  // Full analysis including tensions and fuzzy matching
    ANALYTICAL      // Detailed analysis with multiple candidates
};

struct ChordIdentificationResult {
    std::string chord_name;
    std::string bass_note_name;
    float confidence;
    bool is_slash_chord;
    bool is_inversion;
    std::vector<std::string> alternative_names;
    
    // Enhanced chord naming
    std::string root_note;           // "C", "F#", etc.
    std::string chord_symbol;        // "m7", "7♭9", etc.
    std::string theoretical_name;    // "Cm7", "G7♭9", etc.
    std::string full_display_name;   // "Cm7/Bb" for slash chords
    int inversion_type;              // 0=root, 1=1st, 2=2nd, etc.
    
    // Analysis details
    std::vector<int> identified_intervals;
    std::vector<int> input_notes;
    std::vector<std::string> note_names;
    std::string chord_quality;
    std::string chord_category;
    
    // Performance metrics
    std::chrono::microseconds processing_time;
    bool used_cache;
    
    // Error handling
    std::optional<ErrorInfo> error_info;
    std::vector<ErrorInfo> warnings;
    
    ChordIdentificationResult() 
        : confidence(0.0f), is_slash_chord(false), is_inversion(false),
          inversion_type(0), processing_time(0), used_cache(false) {}
    
    bool isValid() const { return !error_info.has_value() && confidence > 0.0f && !chord_name.empty(); }
    bool hasError() const { return error_info.has_value(); }
    bool hasWarnings() const { return !warnings.empty(); }
    
    void setError(const ErrorInfo& error) { error_info = error; }
    void setError(ErrorCode code, const std::string& message) { error_info = ErrorInfo(code, message); }
    void addWarning(const ErrorInfo& warning) { warnings.push_back(warning); }
    void addWarning(ErrorCode code, const std::string& message) { warnings.emplace_back(code, message); }
    
    std::string getFullName() const;
    std::string getDisplayName(const std::string& style = "standard") const;
};

class ChordIdentifier {
private:
    std::unique_ptr<IntervalEngine> interval_engine;
    std::unique_ptr<ChordDatabase> chord_database;
    std::unique_ptr<NoteConverter> note_converter;
    std::unique_ptr<ChordNameGenerator> name_generator;
    
    // Configuration
    IdentificationMode current_mode;
    float min_confidence_threshold;
    bool enable_slash_chord_detection;
    bool enable_inversion_detection;
    bool enable_tension_analysis;
    std::string preferred_naming_style;
    
    // Performance tracking
    mutable size_t total_identifications;
    mutable size_t cache_hits;
    mutable std::chrono::microseconds total_processing_time;
    
    // Internal processing methods
    ChordIdentificationResult identifyFast(const std::vector<int>& midi_notes) const;
    ChordIdentificationResult identifyStandard(const std::vector<int>& midi_notes) const;
    ChordIdentificationResult identifyComprehensive(const std::vector<int>& midi_notes) const;
    ChordIdentificationResult identifyAnalytical(const std::vector<int>& midi_notes) const;
    
    // Analysis helpers
    std::string determineSlashChord(const IntervalResult& interval_result, const ChordMatch& base_match) const;
    std::vector<std::string> findAlternativeNames(const ChordMatch& match) const;
    std::string formatChordName(const ChordMatch& match, const std::string& root_note, const std::string& bass_note = "") const;
    
    // Enhanced analysis methods
    void enrichChordMatch(ChordMatch& match, const IntervalResult& interval_result, const std::vector<int>& midi_notes) const;
    int determineInversionType(const std::vector<int>& intervals, const std::string& chord_type) const;
    std::pair<std::string, std::string> extractCategoryAndQuality(const std::string& chord_name) const;
    
    // Note name conversion
    std::string midiToNoteName(int midi_note, bool use_sharps = true) const;
    std::vector<std::string> getNoteNames(const std::vector<int>& midi_notes, bool use_sharps = true) const;
    
    // Validation and filtering
    bool isValidChordStructure(const std::vector<int>& intervals) const;
    float calculateOverallConfidence(const ChordMatch& match, const IntervalResult& interval_result) const;
    
public:
    ChordIdentifier();
    ChordIdentifier(IdentificationMode mode);
    ~ChordIdentifier() = default;
    
    // Initialization
    bool initialize(const std::string& chord_dict_path, const std::string& aliases_path = "");
    bool isInitialized() const;
    
    // Primary identification methods
    ChordIdentificationResult identify(const std::vector<int>& midi_notes) const;
    ChordIdentificationResult identify(const std::vector<int>& midi_notes, IdentificationMode mode) const;
    ChordIdentificationResult identify(const std::vector<int>& midi_notes, int specified_bass) const;
    
    // Batch processing
    std::vector<ChordIdentificationResult> identifyBatch(const std::vector<std::vector<int>>& note_sequences) const;
    
    // Error-safe identification methods
    Result<ChordIdentificationResult> identifySafe(const std::vector<int>& midi_notes) const;
    Result<ChordIdentificationResult> identifySafe(const std::vector<int>& midi_notes, IdentificationMode mode) const;
    Result<ChordIdentificationResult> identifySafe(const std::vector<int>& midi_notes, int specified_bass) const;
    Result<std::vector<ChordIdentificationResult>> identifyBatchSafe(const std::vector<std::vector<int>>& note_sequences) const;
    
    // Configuration methods
    void setMode(IdentificationMode mode) { current_mode = mode; }
    IdentificationMode getMode() const { return current_mode; }
    
    void setMinConfidenceThreshold(float threshold) { min_confidence_threshold = std::clamp(threshold, 0.0f, 1.0f); }
    float getMinConfidenceThreshold() const { return min_confidence_threshold; }
    
    void enableSlashChords(bool enable) { enable_slash_chord_detection = enable; }
    void enableInversions(bool enable) { enable_inversion_detection = enable; }
    void enableTensionAnalysis(bool enable) { enable_tension_analysis = enable; }
    
    void setNamingStyle(const std::string& style) { preferred_naming_style = style; }
    std::string getNamingStyle() const { return preferred_naming_style; }
    
    // Utility methods
    std::vector<std::string> getSupportedStyles() const;
    std::vector<std::string> getAvailableChords() const;
    bool isKnownChord(const std::vector<int>& intervals) const;
    
    // Transposition
    ChordIdentificationResult transpose(const ChordIdentificationResult& result, int semitones) const;
    std::vector<int> transposeNotes(const std::vector<int>& notes, int semitones) const;
    
    // Performance and statistics
    void clearCaches();
    void warmupCaches(const std::vector<std::vector<int>>& common_patterns);
    
    struct PerformanceStats {
        size_t total_identifications;
        size_t cache_hits;
        float cache_hit_rate;
        std::chrono::microseconds average_processing_time;
        std::chrono::microseconds total_processing_time;
    };
    
    PerformanceStats getPerformanceStats() const;
    void resetPerformanceStats();
    
    // Validation and debugging
    bool validateConfiguration() const;
    std::string getConfigurationSummary() const;
    void enableDebugMode(bool enable);
    
private:
    // Static lookup tables for note names
    static constexpr std::array<const char*, 12> SHARP_NOTE_NAMES = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    static constexpr std::array<const char*, 12> FLAT_NOTE_NAMES = {
        "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
    };
    
    // Default configurations for different modes
    static constexpr float DEFAULT_FAST_THRESHOLD = 0.9f;
    static constexpr float DEFAULT_STANDARD_THRESHOLD = 0.7f;
    static constexpr float DEFAULT_COMPREHENSIVE_THRESHOLD = 0.5f;
    static constexpr float DEFAULT_ANALYTICAL_THRESHOLD = 0.3f;
};

// Inline implementations for performance-critical methods
inline std::string ChordIdentifier::midiToNoteName(int midi_note, bool use_sharps) const {
    if (!note_converter || midi_note < 0 || midi_note > 127) {
        return "";
    }
    
    AccidentalStyle style = use_sharps ? AccidentalStyle::SHARPS : AccidentalStyle::FLATS;
    return note_converter->midiToNoteName(midi_note, style, OctaveNotation::NO_OCTAVE);
}

inline std::vector<std::string> ChordIdentifier::getNoteNames(const std::vector<int>& midi_notes, bool use_sharps) const {
    std::vector<std::string> note_names;
    note_names.reserve(midi_notes.size());
    
    for (int midi_note : midi_notes) {
        note_names.push_back(midiToNoteName(midi_note, use_sharps));
    }
    
    return note_names;
}

inline bool ChordIdentifier::isKnownChord(const std::vector<int>& intervals) const {
    return chord_database && chord_database->hasChord(intervals);
}

inline std::vector<int> ChordIdentifier::transposeNotes(const std::vector<int>& notes, int semitones) const {
    std::vector<int> transposed;
    transposed.reserve(notes.size());
    
    for (int note : notes) {
        int new_note = note + semitones;
        if (new_note >= 0 && new_note <= 127) {
            transposed.push_back(new_note);
        }
    }
    
    return transposed;
}

inline std::string ChordIdentificationResult::getFullName() const {
    // Use the properly formatted display name if available
    if (!full_display_name.empty() && full_display_name != "UNKNOWN") {
        return full_display_name;
    }
    
    // Use theoretical name if available
    if (!theoretical_name.empty() && theoretical_name != "UNKNOWN") {
        if (is_slash_chord && !bass_note_name.empty()) {
            return theoretical_name + "/" + bass_note_name;
        }
        return theoretical_name;
    }
    
    // Fallback to internal chord name
    if (is_slash_chord && !bass_note_name.empty()) {
        return chord_name + "/" + bass_note_name;
    }
    return chord_name;
}

} // namespace ChordLock