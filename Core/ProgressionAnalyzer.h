#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "ChordIdentifier.h"

namespace ChordLock {

enum class ProgressionType {
    UNKNOWN,
    I_V_vi_IV,      // Pop progression
    ii_V_I,         // Jazz turnaround
    vi_IV_I_V,      // Relative minor variation
    I_vi_ii_V,      // Circle of fifths
    BLUES_12_BAR,   // Blues progression
    MODAL,          // Modal interchange
    CHROMATIC,      // Chromatic mediant
    SECONDARY       // Secondary dominants
};

struct ChordFunction {
    std::string roman_numeral;    // "I", "ii", "V7", etc.
    std::string function_name;    // "Tonic", "Subdominant", "Dominant"
    float stability_score;        // 0.0-1.0, how stable/resolved this chord feels
    bool is_pivot_chord;          // Can modulate to different keys
    std::vector<std::string> possible_next; // Likely next chord functions
};

struct KeyAnalysis {
    std::string key;              // "C major", "A minor"
    float confidence;             // 0.0-1.0
    std::string mode;             // "major", "minor", "dorian", etc.
    std::vector<std::string> scale_degrees; // Notes in the key
    int circle_of_fifths_position; // -7 to +7
};

struct ProgressionAnalysis {
    ProgressionType type;
    std::string description;
    std::vector<ChordFunction> chord_functions;
    KeyAnalysis primary_key;
    std::vector<KeyAnalysis> modulations; // Key changes detected
    float coherence_score;        // How well chords fit together
    std::vector<std::string> suggestions; // Next chord suggestions
};

class ProgressionAnalyzer {
private:
    std::unique_ptr<ChordIdentifier> chord_identifier;
    
    // Key detection algorithms
    std::unordered_map<std::string, float> major_key_profiles;
    std::unordered_map<std::string, float> minor_key_profiles;
    
    // Progression pattern database
    std::vector<std::vector<std::string>> common_progressions;
    
    // Chord function mappings
    std::unordered_map<std::string, ChordFunction> chord_functions_major;
    std::unordered_map<std::string, ChordFunction> chord_functions_minor;
    
    // Analysis helpers
    KeyAnalysis analyzeKey(const std::vector<ChordIdentificationResult>& chords) const;
    ProgressionType detectProgressionType(const std::vector<ChordFunction>& functions) const;
    std::vector<ChordFunction> mapToFunctions(const std::vector<ChordIdentificationResult>& chords, 
                                             const KeyAnalysis& key) const;
    float calculateCoherence(const std::vector<ChordFunction>& functions) const;
    std::vector<std::string> generateSuggestions(const std::vector<ChordFunction>& functions,
                                                const KeyAnalysis& key) const;
    
    // Roman numeral analysis
    std::string chordToRomanNumeral(const std::string& chord_name, const std::string& key) const;
    int getScaleDegree(const std::string& chord_root, const std::string& key) const;
    
    // Key signature analysis
    float scoreKeyFit(const std::vector<std::string>& chord_roots, const std::string& key) const;
    std::vector<KeyAnalysis> getAllKeyCandidates(const std::vector<std::string>& chord_roots) const;
    
public:
    ProgressionAnalyzer();
    ~ProgressionAnalyzer() = default;
    
    // Main analysis methods
    ProgressionAnalysis analyzeProgression(const std::vector<std::vector<int>>& chord_notes) const;
    ProgressionAnalysis analyzeProgression(const std::vector<ChordIdentificationResult>& chords) const;
    
    // Individual analysis components
    KeyAnalysis detectKey(const std::vector<std::vector<int>>& chord_notes) const;
    std::vector<std::string> suggestNextChords(const std::vector<std::vector<int>>& chord_notes, 
                                              int max_suggestions = 5) const;
    
    // Roman numeral analysis
    std::string getRomanNumeralAnalysis(const std::vector<std::vector<int>>& chord_notes,
                                       const std::string& key = "") const;
    
    // Transposition
    std::vector<std::vector<int>> transposeProgression(const std::vector<std::vector<int>>& chord_notes,
                                                      int semitones) const;
    ProgressionAnalysis transposeAnalysis(const ProgressionAnalysis& analysis, int semitones) const;
    
    // Utility methods
    std::vector<std::string> getCommonProgressions() const;
    float getProgressionPopularity(const std::vector<std::string>& progression) const;
    bool isValidProgression(const std::vector<std::string>& progression) const;
    
    // Configuration
    void setChordIdentifier(std::unique_ptr<ChordIdentifier> identifier);
    void loadProgressionDatabase(const std::string& filepath);
    
private:
    // Static data for common progressions
    void initializeCommonProgressions();
    void initializeChordFunctions();
    void initializeKeyProfiles();
    
    // Music theory constants
    static constexpr std::array<const char*, 12> CHROMATIC_SCALE = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    static constexpr std::array<const char*, 7> MAJOR_SCALE_INTERVALS = {
        "I", "ii", "iii", "IV", "V", "vi", "vii°"
    };
    
    static constexpr std::array<const char*, 7> MINOR_SCALE_INTERVALS = {
        "i", "ii°", "III", "iv", "v", "VI", "VII"
    };
};

// Inline utility functions
inline std::string ProgressionAnalyzer::chordToRomanNumeral(const std::string& chord_name, 
                                                           const std::string& key) const {
    // Extract root note from chord name
    std::string root = chord_name.substr(0, chord_name.find_first_not_of("ABCDEFG#b"));
    int degree = getScaleDegree(root, key);
    
    if (degree == -1) return "?";  // Not in key
    
    // Determine major/minor context
    bool is_major_key = key.find("major") != std::string::npos || 
                       (key.find("minor") == std::string::npos && key.length() <= 2);
    
    if (is_major_key) {
        return MAJOR_SCALE_INTERVALS[degree];
    } else {
        return MINOR_SCALE_INTERVALS[degree];
    }
}

inline int ProgressionAnalyzer::getScaleDegree(const std::string& chord_root, 
                                              const std::string& key) const {
    // Extract key root
    std::string key_root = key.substr(0, key.find_first_of(" "));
    
    // Find chromatic distance
    auto key_it = std::find(CHROMATIC_SCALE.begin(), CHROMATIC_SCALE.end(), key_root);
    auto chord_it = std::find(CHROMATIC_SCALE.begin(), CHROMATIC_SCALE.end(), chord_root);
    
    if (key_it == CHROMATIC_SCALE.end() || chord_it == CHROMATIC_SCALE.end()) {
        return -1;
    }
    
    int distance = (chord_it - key_it + 12) % 12;
    
    // Map chromatic distance to scale degree (major scale pattern)
    static const std::array<int, 12> scale_degree_map = {
        0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6  // C to B
    };
    
    return scale_degree_map[distance];
}

} // namespace ChordLock