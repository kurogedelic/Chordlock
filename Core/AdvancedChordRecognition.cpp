#include "AdvancedChordRecognition.h"
#include <unordered_set>
#include <sstream>
#include <numeric>

namespace ChordLock {

// ==================== ExtendedChordInfo Implementation ====================

std::string AdvancedChordRecognition::ExtendedChordInfo::getFullName() const {
    std::stringstream ss;
    
    // Base chord
    ss << primary_chord;
    
    // Add alterations
    for (const auto& alt : alterations) {
        ss << alt;
    }
    
    // Add extensions
    for (const auto& ext : extensions) {
        ss << "(" << ext << ")";
    }
    
    // Add omissions
    if (!omissions.empty()) {
        ss << "(no";
        for (const auto& om : omissions) {
            ss << om;
        }
        ss << ")";
    }
    
    // Add bass note for slash chords
    if (!bass_note.empty() && bass_note != primary_chord.substr(0, bass_note.length())) {
        ss << "/" << bass_note;
    }
    
    // Add secondary chord for polychords
    if (is_polychord && !secondary_chord.empty()) {
        ss << "|" << secondary_chord;
    }
    
    return ss.str();
}

std::string AdvancedChordRecognition::ExtendedChordInfo::getSymbol() const {
    std::stringstream ss;
    
    // Extract root note
    size_t pos = 0;
    while (pos < primary_chord.length() && 
           (std::isalpha(primary_chord[pos]) || primary_chord[pos] == '#' || primary_chord[pos] == 'b')) {
        ss << primary_chord[pos++];
    }
    
    // Add chord quality symbol
    if (primary_chord.find("major") != std::string::npos) {
        ss << "maj";
    } else if (primary_chord.find("minor") != std::string::npos) {
        ss << "m";
    } else if (primary_chord.find("diminished") != std::string::npos) {
        ss << "°";
    } else if (primary_chord.find("augmented") != std::string::npos) {
        ss << "+";
    }
    
    // Add extensions as numbers
    for (const auto& ext : extensions) {
        if (ext == "ninth") ss << "9";
        else if (ext == "eleventh") ss << "11";
        else if (ext == "thirteenth") ss << "13";
    }
    
    // Add alterations
    for (const auto& alt : alterations) {
        ss << alt;
    }
    
    return ss.str();
}

// ==================== AdvancedChordRecognition Implementation ====================

AdvancedChordRecognition::AdvancedChordRecognition() 
    : database_(std::make_unique<ChordDatabase>()),
      current_mode_(RecognitionMode::ADAPTIVE) {
    
    // Initialize chord probabilities based on common usage
    chord_probability_["major"] = 0.25f;
    chord_probability_["minor"] = 0.20f;
    chord_probability_["dominant-seventh"] = 0.15f;
    chord_probability_["minor-seventh"] = 0.12f;
    chord_probability_["major-seventh"] = 0.10f;
    chord_probability_["diminished"] = 0.05f;
    chord_probability_["augmented"] = 0.03f;
    chord_probability_["sus4"] = 0.05f;
    chord_probability_["sus2"] = 0.05f;
}

AdvancedChordRecognition::ExtendedChordInfo 
AdvancedChordRecognition::recognize(const std::vector<int>& midi_notes, RecognitionMode mode) {
    ExtendedChordInfo result;
    result.mode_used = mode;
    
    // Calculate intervals
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    if (sorted_notes.empty()) {
        result.confidence = 0.0f;
        return result;
    }
    
    // Extract bass note
    int bass_note = sorted_notes[0];
    result.bass_note = std::to_string(bass_note % 12);  // Simplified for now
    
    // Calculate intervals from bass
    std::vector<int> intervals;
    for (size_t i = 0; i < sorted_notes.size(); ++i) {
        intervals.push_back((sorted_notes[i] - bass_note) % 12);
    }
    
    // Remove duplicates and sort
    std::sort(intervals.begin(), intervals.end());
    intervals.erase(std::unique(intervals.begin(), intervals.end()), intervals.end());
    
    // Choose recognition strategy based on mode
    switch (mode) {
        case RecognitionMode::JAZZ:
            result = recognizeJazzChord(midi_notes);
            break;
        case RecognitionMode::CLASSICAL:
            result = recognizeClassicalChord(midi_notes);
            break;
        case RecognitionMode::CONTEMPORARY:
            result = recognizeContemporaryChord(midi_notes);
            break;
        case RecognitionMode::ADAPTIVE:
            result = recognizeWithAI(midi_notes);
            break;
        default:
            // Try standard recognition first
            if (auto match = database_->findExactMatch(intervals)) {
                result.primary_chord = match->chord_info.name;
                result.confidence = match->confidence;
            }
            break;
    }
    
    // Detect special characteristics
    result.is_quartal = detectQuartalHarmony(midi_notes).is_quartal;
    result.is_cluster = isClusterChord(intervals);
    result.tonal_ambiguity = calculateTonalAmbiguity(intervals);
    
    // Detect extensions and alterations
    result.extensions = detectJazzExtensions(intervals);
    result.alterations = detectAlterations(intervals);
    result.is_rootless_voicing = isRootlessVoicing(intervals);
    
    return result;
}

AdvancedChordRecognition::ExtendedChordInfo 
AdvancedChordRecognition::recognizeJazzChord(const std::vector<int>& midi_notes) {
    ExtendedChordInfo result;
    
    // Sort notes
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    // Calculate intervals
    std::vector<int> intervals;
    int root = sorted_notes[0];
    for (int note : sorted_notes) {
        intervals.push_back((note - root) % 12);
    }
    
    // Remove duplicates
    std::sort(intervals.begin(), intervals.end());
    intervals.erase(std::unique(intervals.begin(), intervals.end()), intervals.end());
    
    // Check for rootless voicing
    result.is_rootless_voicing = isRootlessVoicing(intervals);
    
    // Detect chord type with jazz omissions allowed
    bool has_minor_third = std::find(intervals.begin(), intervals.end(), 3) != intervals.end();
    bool has_major_third = std::find(intervals.begin(), intervals.end(), 4) != intervals.end();
    bool has_perfect_fifth = std::find(intervals.begin(), intervals.end(), 7) != intervals.end();
    bool has_minor_seventh = std::find(intervals.begin(), intervals.end(), 10) != intervals.end();
    bool has_major_seventh = std::find(intervals.begin(), intervals.end(), 11) != intervals.end();
    
    // Determine base chord quality
    if (has_major_third && has_major_seventh) {
        result.primary_chord = "major-seventh";
    } else if (has_major_third && has_minor_seventh) {
        result.primary_chord = "dominant-seventh";
    } else if (has_minor_third && has_minor_seventh) {
        result.primary_chord = "minor-seventh";
    } else if (has_minor_third && has_major_seventh) {
        result.primary_chord = "minor-major-seventh";
    } else if (has_major_third) {
        result.primary_chord = "major";
    } else if (has_minor_third) {
        result.primary_chord = "minor";
    } else {
        result.primary_chord = "indeterminate";
    }
    
    // Detect extensions
    result.extensions = detectJazzExtensions(intervals);
    result.alterations = detectAlterations(intervals);
    
    // Allow common jazz omissions
    if (!has_perfect_fifth && result.primary_chord != "indeterminate") {
        result.omissions.push_back("5");
        result.confidence = 0.85f;  // Slightly lower confidence for omitted 5th
    } else {
        result.confidence = 0.95f;
    }
    
    // Check for upper structure triads
    if (auto upper = detectUpperStructure(midi_notes)) {
        result.is_upper_structure = true;
        result.secondary_chord = upper->second;
    }
    
    return result;
}

std::vector<std::string> 
AdvancedChordRecognition::detectJazzExtensions(const std::vector<int>& intervals) {
    std::vector<std::string> extensions;
    
    // Check for common jazz extensions
    if (std::find(intervals.begin(), intervals.end(), 2) != intervals.end()) {
        extensions.push_back("9");
    }
    if (std::find(intervals.begin(), intervals.end(), 1) != intervals.end()) {
        extensions.push_back("b9");
    }
    if (std::find(intervals.begin(), intervals.end(), 3) != intervals.end() &&
        intervals.size() > 4) {  // #9 only if not the third
        extensions.push_back("#9");
    }
    if (std::find(intervals.begin(), intervals.end(), 5) != intervals.end()) {
        extensions.push_back("11");
    }
    if (std::find(intervals.begin(), intervals.end(), 6) != intervals.end()) {
        extensions.push_back("#11");
    }
    if (std::find(intervals.begin(), intervals.end(), 9) != intervals.end()) {
        extensions.push_back("13");
    }
    if (std::find(intervals.begin(), intervals.end(), 8) != intervals.end()) {
        extensions.push_back("b13");
    }
    
    return extensions;
}

std::vector<std::string> 
AdvancedChordRecognition::detectAlterations(const std::vector<int>& intervals) {
    std::vector<std::string> alterations;
    
    // Check for altered 5ths
    bool has_perfect_fifth = std::find(intervals.begin(), intervals.end(), 7) != intervals.end();
    bool has_flat_fifth = std::find(intervals.begin(), intervals.end(), 6) != intervals.end();
    bool has_sharp_fifth = std::find(intervals.begin(), intervals.end(), 8) != intervals.end();
    
    if (has_flat_fifth && !has_perfect_fifth) {
        alterations.push_back("b5");
    }
    if (has_sharp_fifth && !has_perfect_fifth) {
        alterations.push_back("#5");
    }
    
    return alterations;
}

bool AdvancedChordRecognition::isRootlessVoicing(const std::vector<int>& intervals) {
    // Common rootless voicings don't include the root (0)
    // They typically start from the 3rd or 7th
    return intervals.empty() || (intervals[0] != 0 && 
           (intervals[0] == 3 || intervals[0] == 4 ||  // 3rd
            intervals[0] == 10 || intervals[0] == 11)); // 7th
}

AdvancedChordRecognition::ExtendedChordInfo 
AdvancedChordRecognition::detectQuartalHarmony(const std::vector<int>& midi_notes) {
    ExtendedChordInfo result;
    
    // Sort notes
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    // Check intervals between consecutive notes
    int fourth_count = 0;
    int total_intervals = 0;
    
    for (size_t i = 1; i < sorted_notes.size(); ++i) {
        int interval = (sorted_notes[i] - sorted_notes[i-1]) % 12;
        if (interval == 5) {  // Perfect fourth
            fourth_count++;
        }
        total_intervals++;
    }
    
    // If more than 60% of intervals are fourths, it's quartal
    float fourth_ratio = total_intervals > 0 ? 
                        static_cast<float>(fourth_count) / total_intervals : 0.0f;
    
    result.is_quartal = fourth_ratio > 0.6f;
    
    if (result.is_quartal) {
        result.primary_chord = "quartal-voicing";
        result.confidence = 0.8f + (fourth_ratio - 0.6f);  // Higher ratio = higher confidence
        
        // Check for "So What" chord pattern
        if (sorted_notes.size() >= 5) {
            std::vector<int> intervals;
            for (size_t i = 1; i < sorted_notes.size(); ++i) {
                intervals.push_back((sorted_notes[i] - sorted_notes[0]) % 12);
            }
            
            // So What chord: root, P4, P4, P4, M3
            if (intervals.size() >= 4 &&
                intervals[0] == 5 && intervals[1] == 10 && 
                intervals[2] == 3 && intervals[3] == 7) {
                result.primary_chord = "so-what-chord";
                result.confidence = 0.95f;
            }
        }
    }
    
    return result;
}

bool AdvancedChordRecognition::isClusterChord(const std::vector<int>& intervals) {
    // Cluster chords have many adjacent semitones or whole tones
    int adjacent_count = 0;
    
    for (size_t i = 1; i < intervals.size(); ++i) {
        int diff = intervals[i] - intervals[i-1];
        if (diff <= 2) {  // Semitone or whole tone
            adjacent_count++;
        }
    }
    
    // If more than 70% of intervals are adjacent, it's a cluster
    return intervals.size() > 0 && 
           (static_cast<float>(adjacent_count) / (intervals.size() - 1)) > 0.7f;
}

float AdvancedChordRecognition::calculateTonalAmbiguity(const std::vector<int>& intervals) {
    // Calculate how ambiguous the tonal center is
    // Based on presence of defining intervals
    
    float clarity = 0.0f;
    
    // Major/minor third provides clarity
    if (std::find(intervals.begin(), intervals.end(), 3) != intervals.end() ||
        std::find(intervals.begin(), intervals.end(), 4) != intervals.end()) {
        clarity += 0.3f;
    }
    
    // Perfect fifth provides clarity
    if (std::find(intervals.begin(), intervals.end(), 7) != intervals.end()) {
        clarity += 0.2f;
    }
    
    // Seventh provides some clarity
    if (std::find(intervals.begin(), intervals.end(), 10) != intervals.end() ||
        std::find(intervals.begin(), intervals.end(), 11) != intervals.end()) {
        clarity += 0.1f;
    }
    
    // Tritone adds ambiguity
    if (std::find(intervals.begin(), intervals.end(), 6) != intervals.end()) {
        clarity -= 0.2f;
    }
    
    // Many chromatic intervals add ambiguity
    int chromatic_count = 0;
    for (int interval : intervals) {
        if (interval == 1 || interval == 6 || interval == 8 || interval == 11) {
            chromatic_count++;
        }
    }
    clarity -= (chromatic_count * 0.1f);
    
    // Convert clarity to ambiguity (1 - clarity)
    float ambiguity = 1.0f - std::max(0.0f, std::min(1.0f, clarity));
    
    return ambiguity;
}

AdvancedChordRecognition::ExtendedChordInfo 
AdvancedChordRecognition::recognizeClassicalChord(const std::vector<int>& midi_notes) {
    ExtendedChordInfo result;
    
    // Classical recognition focuses on voice leading and functional harmony
    // Implementation would include:
    // - Neapolitan chords
    // - Augmented sixth chords (Italian, French, German)
    // - Secondary dominants
    // - Chromatic mediants
    
    result.primary_chord = "classical-chord";
    result.confidence = 0.7f;
    
    return result;
}

AdvancedChordRecognition::ExtendedChordInfo 
AdvancedChordRecognition::recognizeContemporaryChord(const std::vector<int>& midi_notes) {
    ExtendedChordInfo result;
    
    // Contemporary recognition includes:
    // - Messiaen modes
    // - Spectral harmony
    // - Neo-Riemannian transformations
    // - Extended techniques
    
    result.primary_chord = "contemporary-chord";
    result.confidence = 0.6f;
    
    return result;
}

AdvancedChordRecognition::ExtendedChordInfo 
AdvancedChordRecognition::recognizeWithAI(const std::vector<int>& midi_notes) {
    ExtendedChordInfo result;
    
    // First try standard recognition
    result = recognize(midi_notes, RecognitionMode::STRICT);
    
    // If confidence is low, try other modes
    if (result.confidence < 0.7f) {
        auto jazz_result = recognize(midi_notes, RecognitionMode::JAZZ);
        if (jazz_result.confidence > result.confidence) {
            result = jazz_result;
        }
    }
    
    // Check learned patterns
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    std::vector<int> intervals;
    if (!sorted_notes.empty()) {
        int root = sorted_notes[0];
        for (int note : sorted_notes) {
            intervals.push_back((note - root) % 12);
        }
        
        auto it = learned_patterns_.find(intervals);
        if (it != learned_patterns_.end()) {
            result.primary_chord = it->second;
            result.confidence = 0.9f;  // High confidence for learned patterns
        }
    }
    
    return result;
}

std::optional<std::pair<std::string, std::string>> 
AdvancedChordRecognition::detectUpperStructure(const std::vector<int>& midi_notes) {
    if (midi_notes.size() < 6) {
        return std::nullopt;
    }
    
    // Split notes into lower and upper voices
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    size_t split_point = sorted_notes.size() / 2;
    std::vector<int> lower(sorted_notes.begin(), sorted_notes.begin() + split_point);
    std::vector<int> upper(sorted_notes.begin() + split_point, sorted_notes.end());
    
    // Convert to intervals for matching
    std::vector<int> lower_intervals, upper_intervals;
    if (!lower.empty()) {
        int lower_root = lower[0];
        for (int note : lower) {
            lower_intervals.push_back((note - lower_root) % 12);
        }
    }
    if (!upper.empty()) {
        int upper_root = upper[0];
        for (int note : upper) {
            upper_intervals.push_back((note - upper_root) % 12);
        }
    }
    
    // Try to identify both parts
    auto lower_chord = database_->findExactMatch(lower_intervals);
    auto upper_chord = database_->findExactMatch(upper_intervals);
    
    if (lower_chord && upper_chord) {
        return std::make_pair(lower_chord->chord_info.name, 
                             upper_chord->chord_info.name);
    }
    
    return std::nullopt;
}

void AdvancedChordRecognition::learnPattern(const std::vector<int>& midi_notes, 
                                           const std::string& correct_chord) {
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    if (!sorted_notes.empty()) {
        std::vector<int> intervals;
        int root = sorted_notes[0];
        for (int note : sorted_notes) {
            intervals.push_back((note - root) % 12);
        }
        
        learned_patterns_[intervals] = correct_chord;
        
        // Update chord probability
        if (chord_probability_.find(correct_chord) != chord_probability_.end()) {
            chord_probability_[correct_chord] += 0.01f;  // Slight increase
        } else {
            chord_probability_[correct_chord] = 0.05f;   // New chord type
        }
    }
}

std::optional<std::pair<AdvancedChordRecognition::ExtendedChordInfo, 
                       AdvancedChordRecognition::ExtendedChordInfo>>
AdvancedChordRecognition::detectPolychord(const std::vector<int>& midi_notes) {
    // Polychords require at least 6 notes (3 for each chord)
    if (midi_notes.size() < 6) {
        return std::nullopt;
    }
    
    std::vector<int> sorted_notes = midi_notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    // Try different split points to find two distinct chords
    for (size_t split = 3; split <= sorted_notes.size() - 3; ++split) {
        std::vector<int> lower(sorted_notes.begin(), sorted_notes.begin() + split);
        std::vector<int> upper(sorted_notes.begin() + split, sorted_notes.end());
        
        // Check if there's a gap between the two groups (typical of polychords)
        int gap = upper[0] - lower[lower.size() - 1];
        if (gap < 3) continue;  // Too close together, likely not a polychord
        
        // Recognize each part independently
        ExtendedChordInfo lower_info = recognize(lower, RecognitionMode::ADAPTIVE);
        ExtendedChordInfo upper_info = recognize(upper, RecognitionMode::ADAPTIVE);
        
        // Both parts must be recognizable with good confidence
        if (lower_info.confidence > 0.7f && upper_info.confidence > 0.7f) {
            lower_info.is_polychord = true;
            upper_info.is_polychord = true;
            return std::make_pair(lower_info, upper_info);
        }
    }
    
    return std::nullopt;
}

AdvancedChordRecognition::ExtendedChordInfo
AdvancedChordRecognition::recognizeMicrotonal(const std::vector<float>& frequencies) {
    ExtendedChordInfo result;
    
    if (frequencies.empty()) {
        result.confidence = 0.0f;
        return result;
    }
    
    // Convert frequencies to cents from the lowest frequency
    float base_freq = frequencies[0];
    std::vector<float> cents_intervals;
    
    for (float freq : frequencies) {
        float cents = 1200.0f * std::log2(freq / base_freq);
        cents_intervals.push_back(cents);
    }
    
    // Check for quarter-tone intervals
    bool has_quarter_tones = false;
    for (float cents : cents_intervals) {
        float remainder = std::fmod(cents, 100.0f);
        if (remainder > 40.0f && remainder < 60.0f) {
            has_quarter_tones = true;
            break;
        }
    }
    
    // Check for just intonation ratios
    bool is_just_intonation = true;
    const std::vector<float> just_ratios = {
        1.0f,     // Unison
        1.125f,   // Major second (9:8)
        1.2f,     // Minor third (6:5)
        1.25f,    // Major third (5:4)
        1.333f,   // Perfect fourth (4:3)
        1.5f,     // Perfect fifth (3:2)
        1.667f,   // Minor sixth (5:3)
        1.875f,   // Major seventh (15:8)
        2.0f      // Octave
    };
    
    for (size_t i = 1; i < frequencies.size(); ++i) {
        float ratio = frequencies[i] / base_freq;
        bool found_match = false;
        
        for (float just_ratio : just_ratios) {
            if (std::abs(ratio - just_ratio) < 0.01f) {
                found_match = true;
                break;
            }
        }
        
        if (!found_match) {
            is_just_intonation = false;
        }
    }
    
    // Set chord properties based on analysis
    if (has_quarter_tones) {
        result.primary_chord = "quarter-tone-chord";
        result.modal_context = "microtonal";
        result.confidence = 0.8f;
    } else if (is_just_intonation) {
        result.primary_chord = "just-intonation-chord";
        result.modal_context = "just-intonation";
        result.confidence = 0.9f;
    } else {
        result.primary_chord = "microtonal-chord";
        result.modal_context = "non-12tet";
        result.confidence = 0.7f;
    }
    
    result.tonal_ambiguity = has_quarter_tones ? 0.8f : 0.3f;
    
    return result;
}

AdvancedChordRecognition::ExtendedChordInfo
AdvancedChordRecognition::recognizeInContext(
    const std::vector<int>& midi_notes,
    const std::optional<std::vector<int>>& previous_chord,
    const std::optional<std::vector<int>>& next_chord) {
    
    ExtendedChordInfo result = recognize(midi_notes, current_mode_);
    
    // Adjust confidence based on harmonic context
    if (previous_chord && next_chord) {
        float voice_leading_prev = analyzeVoiceLeading(previous_chord.value(), midi_notes);
        float voice_leading_next = analyzeVoiceLeading(midi_notes, next_chord.value());
        
        // Good voice leading increases confidence
        float context_boost = (voice_leading_prev + voice_leading_next) * harmonic_context_weight_;
        result.confidence = std::min(1.0f, result.confidence + context_boost);
    }
    
    return result;
}

std::string AdvancedChordRecognition::detectModalInterchange(
    const std::vector<int>& midi_notes,
    const std::string& key_context) {
    
    // Simplified modal interchange detection
    // Would need full implementation with key signatures and mode detection
    
    ExtendedChordInfo chord_info = recognize(midi_notes, RecognitionMode::ADAPTIVE);
    
    // Check if chord belongs to parallel modes
    if (key_context.find("major") != std::string::npos) {
        if (chord_info.primary_chord.find("minor") != std::string::npos) {
            return "borrowed-from-parallel-minor";
        }
    } else if (key_context.find("minor") != std::string::npos) {
        if (chord_info.primary_chord.find("major") != std::string::npos) {
            return "borrowed-from-parallel-major";
        }
    }
    
    return "diatonic";
}

float AdvancedChordRecognition::analyzeVoiceLeading(
    const std::vector<int>& chord1,
    const std::vector<int>& chord2) {
    
    if (chord1.empty() || chord2.empty()) {
        return 0.0f;
    }
    
    // Calculate total movement between chords
    int total_movement = 0;
    size_t compared_notes = std::min(chord1.size(), chord2.size());
    
    for (size_t i = 0; i < compared_notes; ++i) {
        int movement = std::abs(chord2[i] - chord1[i]);
        // Prefer smaller movements (good voice leading)
        total_movement += movement;
    }
    
    // Lower movement = better voice leading
    float avg_movement = static_cast<float>(total_movement) / compared_notes;
    float quality = 1.0f - (avg_movement / 12.0f);  // Normalize to 0-1
    
    return std::max(0.0f, std::min(1.0f, quality));
}

std::string AdvancedChordRecognition::detectHarmonicFunction(
    const std::vector<int>& midi_notes,
    const std::string& key) {
    
    ExtendedChordInfo chord_info = recognize(midi_notes, RecognitionMode::CLASSICAL);
    
    // Simplified harmonic function detection
    // Would need full implementation with Roman numeral analysis
    
    if (chord_info.primary_chord.find("major") != std::string::npos) {
        if (key == chord_info.primary_chord.substr(0, 1)) {
            return "Tonic";
        } else {
            return "Subdominant";
        }
    } else if (chord_info.primary_chord.find("dominant") != std::string::npos) {
        return "Dominant";
    } else if (chord_info.primary_chord.find("diminished") != std::string::npos) {
        return "Leading-tone";
    }
    
    return "Unknown";
}

} // namespace ChordLock