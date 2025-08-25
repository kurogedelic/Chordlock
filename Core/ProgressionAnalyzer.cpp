#include "ProgressionAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <iostream>

namespace ChordLock {

ProgressionAnalyzer::ProgressionAnalyzer() {
    chord_identifier = std::make_unique<ChordIdentifier>(IdentificationMode::COMPREHENSIVE);
    chord_identifier->initialize("interval_dict.yaml");
    
    initializeCommonProgressions();
    initializeChordFunctions();
    initializeKeyProfiles();
}

void ProgressionAnalyzer::initializeCommonProgressions() {
    // Popular chord progressions from research
    common_progressions = {
        {"I", "V", "vi", "IV"},     // Pop progression (C-G-Am-F)
        {"vi", "IV", "I", "V"},     // Relative minor variation
        {"I", "vi", "ii", "V"},     // Circle of fifths
        {"ii", "V", "I"},           // Jazz turnaround
        {"I", "IV", "V", "I"},      // Classical cadence
        {"vi", "ii", "V", "I"},     // Extended jazz
        {"I", "V", "vi", "iii", "IV", "I", "IV", "V"}, // Canon progression
        {"I", "bVII", "IV", "I"},   // Mixolydian
        {"i", "VI", "III", "VII"},  // Natural minor
        {"i", "iv", "V", "i"},      // Minor classical
        {"I", "I", "I", "I", "IV", "IV", "I", "I", "V", "IV", "I", "V"} // 12-bar blues
    };
}

void ProgressionAnalyzer::initializeChordFunctions() {
    // Major key chord functions
    chord_functions_major = {
        {"I", {"I", "Tonic", 1.0f, false, {"ii", "iii", "IV", "V", "vi"}}},
        {"ii", {"ii", "Subdominant", 0.4f, true, {"V", "vii°"}}},
        {"iii", {"iii", "Tonic", 0.3f, false, {"vi", "IV"}}},
        {"IV", {"IV", "Subdominant", 0.7f, true, {"I", "V", "ii"}}},
        {"V", {"V", "Dominant", 0.2f, false, {"I", "vi"}}},
        {"vi", {"vi", "Tonic", 0.6f, true, {"ii", "IV", "V"}}},
        {"vii°", {"vii°", "Dominant", 0.1f, false, {"I"}}}
    };
    
    // Minor key chord functions  
    chord_functions_minor = {
        {"i", {"i", "Tonic", 1.0f, false, {"ii°", "III", "iv", "v", "VI"}}},
        {"ii°", {"ii°", "Subdominant", 0.3f, true, {"V", "v"}}},
        {"III", {"III", "Tonic", 0.5f, true, {"VI", "iv"}}},
        {"iv", {"iv", "Subdominant", 0.7f, true, {"i", "V", "v"}}},
        {"v", {"v", "Dominant", 0.4f, false, {"i", "VI"}}},
        {"V", {"V", "Dominant", 0.2f, false, {"i"}}},  // Harmonic minor
        {"VI", {"VI", "Tonic", 0.6f, true, {"ii°", "iv", "V"}}},
        {"VII", {"VII", "Subtonic", 0.4f, false, {"III", "i"}}}
    };
}

void ProgressionAnalyzer::initializeKeyProfiles() {
    // Krumhansl-Schmuckler key profiles for major keys
    std::vector<float> major_profile = {
        6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88
    };
    
    // Minor key profiles (shifted and adjusted)
    std::vector<float> minor_profile = {
        6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17
    };
    
    // Create profiles for all 12 keys
    for (int i = 0; i < 12; ++i) {
        std::string major_key = std::string(CHROMATIC_SCALE[i]) + " major";
        std::string minor_key = std::string(CHROMATIC_SCALE[i]) + " minor";
        
        for (int j = 0; j < 12; ++j) {
            int rotated_index = (j - i + 12) % 12;
            major_key_profiles[major_key + "_" + std::string(CHROMATIC_SCALE[j])] = major_profile[rotated_index];
            minor_key_profiles[minor_key + "_" + std::string(CHROMATIC_SCALE[j])] = minor_profile[rotated_index];
        }
    }
}

ProgressionAnalysis ProgressionAnalyzer::analyzeProgression(const std::vector<std::vector<int>>& chord_notes) const {
    ProgressionAnalysis analysis;
    
    // Step 1: Identify individual chords
    std::vector<ChordIdentificationResult> chords;
    for (const auto& notes : chord_notes) {
        auto result = chord_identifier->identify(notes);
        chords.push_back(result);
    }
    
    return analyzeProgression(chords);
}

ProgressionAnalysis ProgressionAnalyzer::analyzeProgression(const std::vector<ChordIdentificationResult>& chords) const {
    ProgressionAnalysis analysis;
    
    if (chords.empty()) {
        analysis.type = ProgressionType::UNKNOWN;
        analysis.description = "No chords provided";
        analysis.coherence_score = 0.0f;
        return analysis;
    }
    
    // Step 1: Analyze key
    analysis.primary_key = analyzeKey(chords);
    
    // Step 2: Map chords to functions
    analysis.chord_functions = mapToFunctions(chords, analysis.primary_key);
    
    // Step 3: Detect progression type
    analysis.type = detectProgressionType(analysis.chord_functions);
    
    // Step 4: Calculate coherence
    analysis.coherence_score = calculateCoherence(analysis.chord_functions);
    
    // Step 5: Generate suggestions
    analysis.suggestions = generateSuggestions(analysis.chord_functions, analysis.primary_key);
    
    // Step 6: Create description
    switch (analysis.type) {
        case ProgressionType::I_V_vi_IV:
            analysis.description = "Popular I-V-vi-IV progression (very common in pop music)";
            break;
        case ProgressionType::ii_V_I:
            analysis.description = "Jazz ii-V-I turnaround (fundamental in jazz harmony)";
            break;
        case ProgressionType::BLUES_12_BAR:
            analysis.description = "12-bar blues progression";
            break;
        case ProgressionType::MODAL:
            analysis.description = "Modal progression with characteristic non-diatonic chords";
            break;
        default:
            analysis.description = "Custom chord progression";
            break;
    }
    
    return analysis;
}

KeyAnalysis ProgressionAnalyzer::analyzeKey(const std::vector<ChordIdentificationResult>& chords) const {
    if (chords.empty()) {
        return KeyAnalysis{"C major", 0.0f, "major", {}, 0};
    }
    
    // Extract root notes from chords
    std::vector<std::string> chord_roots;
    for (const auto& chord : chords) {
        if (!chord.root_note.empty()) {
            chord_roots.push_back(chord.root_note);
        }
    }
    
    auto candidates = getAllKeyCandidates(chord_roots);
    
    if (candidates.empty()) {
        return KeyAnalysis{"C major", 0.5f, "major", {}, 0};
    }
    
    // Return the highest confidence key
    return *std::max_element(candidates.begin(), candidates.end(), 
                           [](const KeyAnalysis& a, const KeyAnalysis& b) {
                               return a.confidence < b.confidence;
                           });
}

std::vector<KeyAnalysis> ProgressionAnalyzer::getAllKeyCandidates(const std::vector<std::string>& chord_roots) const {
    std::vector<KeyAnalysis> candidates;
    
    // Test all 24 keys (12 major + 12 minor)
    for (int i = 0; i < 12; ++i) {
        std::string major_key = std::string(CHROMATIC_SCALE[i]) + " major";
        std::string minor_key = std::string(CHROMATIC_SCALE[i]) + " minor";
        
        float major_score = scoreKeyFit(chord_roots, major_key);
        float minor_score = scoreKeyFit(chord_roots, minor_key);
        
        if (major_score > 0.3f) {
            KeyAnalysis major_analysis;
            major_analysis.key = major_key;
            major_analysis.confidence = major_score;
            major_analysis.mode = "major";
            major_analysis.circle_of_fifths_position = (i * 7) % 12 - 6; // Convert to -6 to +6
            candidates.push_back(major_analysis);
        }
        
        if (minor_score > 0.3f) {
            KeyAnalysis minor_analysis;
            minor_analysis.key = minor_key;
            minor_analysis.confidence = minor_score;
            minor_analysis.mode = "minor";
            minor_analysis.circle_of_fifths_position = (i * 7 + 3) % 12 - 6; // Relative minor adjustment
            candidates.push_back(minor_analysis);
        }
    }
    
    return candidates;
}

float ProgressionAnalyzer::scoreKeyFit(const std::vector<std::string>& chord_roots, const std::string& key) const {
    float total_score = 0.0f;
    bool is_major = key.find("major") != std::string::npos;
    
    const auto& profiles = is_major ? major_key_profiles : minor_key_profiles;
    
    for (const auto& root : chord_roots) {
        std::string profile_key = key + "_" + root;
        auto it = profiles.find(profile_key);
        if (it != profiles.end()) {
            total_score += it->second;
        }
    }
    
    // Normalize by number of chords and max possible score
    if (!chord_roots.empty()) {
        total_score /= (chord_roots.size() * 6.35f); // 6.35 is max profile value
    }
    
    return std::min(1.0f, total_score);
}

std::vector<ChordFunction> ProgressionAnalyzer::mapToFunctions(const std::vector<ChordIdentificationResult>& chords, 
                                                              const KeyAnalysis& key) const {
    std::vector<ChordFunction> functions;
    bool is_major = key.mode == "major";
    const auto& function_map = is_major ? chord_functions_major : chord_functions_minor;
    
    for (const auto& chord : chords) {
        std::string roman = chordToRomanNumeral(chord.chord_name, key.key);
        
        auto it = function_map.find(roman);
        if (it != function_map.end()) {
            functions.push_back(it->second);
        } else {
            // Create default function for unknown chords
            ChordFunction unknown_func;
            unknown_func.roman_numeral = roman;
            unknown_func.function_name = "Unknown";
            unknown_func.stability_score = 0.5f;
            unknown_func.is_pivot_chord = true;
            functions.push_back(unknown_func);
        }
    }
    
    return functions;
}

ProgressionType ProgressionAnalyzer::detectProgressionType(const std::vector<ChordFunction>& functions) const {
    if (functions.size() < 3) {
        return ProgressionType::UNKNOWN;
    }
    
    // Convert to roman numeral sequence
    std::vector<std::string> sequence;
    for (const auto& func : functions) {
        sequence.push_back(func.roman_numeral);
    }
    
    // Check for common patterns
    if (sequence.size() >= 4) {
        std::vector<std::string> four_chord = {sequence[0], sequence[1], sequence[2], sequence[3]};
        
        if (four_chord == std::vector<std::string>{"I", "V", "vi", "IV"}) {
            return ProgressionType::I_V_vi_IV;
        }
        if (four_chord == std::vector<std::string>{"vi", "IV", "I", "V"}) {
            return ProgressionType::vi_IV_I_V;
        }
        if (four_chord == std::vector<std::string>{"I", "vi", "ii", "V"}) {
            return ProgressionType::I_vi_ii_V;
        }
    }
    
    if (sequence.size() >= 3) {
        std::vector<std::string> three_chord = {sequence[0], sequence[1], sequence[2]};
        
        if (three_chord == std::vector<std::string>{"ii", "V", "I"} ||
            three_chord == std::vector<std::string>{"ii°", "V", "i"}) {
            return ProgressionType::ii_V_I;
        }
    }
    
    // Check for blues (simplified)
    if (sequence.size() >= 12) {
        bool is_blues = true;
        std::vector<std::string> blues_pattern = {"I", "I", "I", "I", "IV", "IV", "I", "I", "V", "IV", "I", "V"};
        for (size_t i = 0; i < 12 && i < sequence.size(); ++i) {
            if (sequence[i] != blues_pattern[i]) {
                is_blues = false;
                break;
            }
        }
        if (is_blues) return ProgressionType::BLUES_12_BAR;
    }
    
    return ProgressionType::UNKNOWN;
}

float ProgressionAnalyzer::calculateCoherence(const std::vector<ChordFunction>& functions) const {
    if (functions.size() < 2) return 1.0f;
    
    float coherence_sum = 0.0f;
    int connections = 0;
    
    for (size_t i = 0; i < functions.size() - 1; ++i) {
        const auto& current = functions[i];
        const auto& next = functions[i + 1];
        
        // Check if next chord is in the suggested list
        auto it = std::find(current.possible_next.begin(), current.possible_next.end(), next.roman_numeral);
        if (it != current.possible_next.end()) {
            coherence_sum += 1.0f;
        } else {
            // Partial credit for related functions
            if (current.function_name == next.function_name) {
                coherence_sum += 0.5f;
            } else if ((current.function_name == "Dominant" && next.function_name == "Tonic") ||
                      (current.function_name == "Subdominant" && next.function_name == "Dominant")) {
                coherence_sum += 0.7f;
            }
        }
        connections++;
    }
    
    return connections > 0 ? coherence_sum / connections : 1.0f;
}

std::vector<std::string> ProgressionAnalyzer::generateSuggestions(const std::vector<ChordFunction>& functions,
                                                                 const KeyAnalysis& key) const {
    if (functions.empty()) {
        return {"I", "vi", "IV", "V"}; // Default suggestions
    }
    
    const auto& last_function = functions.back();
    std::vector<std::string> suggestions = last_function.possible_next;
    
    // Add context-specific suggestions
    bool is_major = key.mode == "major";
    if (is_major) {
        suggestions.insert(suggestions.end(), {"I", "vi", "IV", "V"});
    } else {
        suggestions.insert(suggestions.end(), {"i", "VI", "iv", "V"});
    }
    
    // Remove duplicates and limit to 5 suggestions
    std::set<std::string> unique_suggestions(suggestions.begin(), suggestions.end());
    std::vector<std::string> result(unique_suggestions.begin(), unique_suggestions.end());
    
    if (result.size() > 5) {
        result.resize(5);
    }
    
    return result;
}

KeyAnalysis ProgressionAnalyzer::detectKey(const std::vector<std::vector<int>>& chord_notes) const {
    std::vector<ChordIdentificationResult> chords;
    for (const auto& notes : chord_notes) {
        chords.push_back(chord_identifier->identify(notes));
    }
    return analyzeKey(chords);
}

std::vector<std::string> ProgressionAnalyzer::suggestNextChords(const std::vector<std::vector<int>>& chord_notes, 
                                                              int max_suggestions) const {
    auto analysis = analyzeProgression(chord_notes);
    auto suggestions = analysis.suggestions;
    
    if (suggestions.size() > static_cast<size_t>(max_suggestions)) {
        suggestions.resize(max_suggestions);
    }
    
    return suggestions;
}

std::string ProgressionAnalyzer::getRomanNumeralAnalysis(const std::vector<std::vector<int>>& chord_notes,
                                                        const std::string& key) const {
    (void)key; // Suppress unused parameter warning
    auto analysis = analyzeProgression(chord_notes);
    
    std::string result = "Key: " + analysis.primary_key.key + " | ";
    for (size_t i = 0; i < analysis.chord_functions.size(); ++i) {
        if (i > 0) result += " - ";
        result += analysis.chord_functions[i].roman_numeral;
    }
    
    return result;
}

std::vector<std::vector<int>> ProgressionAnalyzer::transposeProgression(const std::vector<std::vector<int>>& chord_notes,
                                                                       int semitones) const {
    std::vector<std::vector<int>> transposed;
    
    for (const auto& chord : chord_notes) {
        std::vector<int> transposed_chord;
        for (int note : chord) {
            int new_note = note + semitones;
            if (new_note >= 0 && new_note <= 127) {
                transposed_chord.push_back(new_note);
            }
        }
        transposed.push_back(transposed_chord);
    }
    
    return transposed;
}

std::vector<std::string> ProgressionAnalyzer::getCommonProgressions() const {
    std::vector<std::string> descriptions;
    
    descriptions.push_back("I-V-vi-IV (Pop progression)");
    descriptions.push_back("vi-IV-I-V (Relative minor variation)");
    descriptions.push_back("ii-V-I (Jazz turnaround)");
    descriptions.push_back("I-vi-ii-V (Circle of fifths)");
    descriptions.push_back("I-IV-V-I (Classical cadence)");
    descriptions.push_back("12-bar blues");
    
    return descriptions;
}

bool ProgressionAnalyzer::isValidProgression(const std::vector<std::string>& progression) const {
    // Basic validation - check if progression follows common voice leading principles
    if (progression.size() < 2) return true;
    
    for (size_t i = 0; i < progression.size() - 1; ++i) {
        // Add specific voice leading rules here
        // For now, just return true as basic validation
    }
    
    return true;
}

} // namespace ChordLock