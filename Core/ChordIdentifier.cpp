#include "ChordIdentifier.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iostream>

namespace ChordLock {

ChordIdentifier::ChordIdentifier() 
    : ChordIdentifier(IdentificationMode::STANDARD) {
}

ChordIdentifier::ChordIdentifier(IdentificationMode mode)
    : current_mode(mode)
    , enable_slash_chord_detection(true)
    , enable_inversion_detection(true)
    , enable_tension_analysis(false)
    , preferred_naming_style("standard")
    , total_identifications(0)
    , cache_hits(0)
    , total_processing_time(0) {
    
    interval_engine = std::make_unique<IntervalEngine>();
    chord_database = std::make_unique<ChordDatabase>();
    note_converter = std::make_unique<NoteConverter>(AccidentalStyle::SHARPS);
    name_generator = std::make_unique<ChordNameGenerator>(NamingStyle::JAZZ, KeyContext::AUTO_DETECT);
    
    // Set confidence threshold based on mode
    switch (mode) {
        case IdentificationMode::FAST:
            min_confidence_threshold = DEFAULT_FAST_THRESHOLD;
            enable_tension_analysis = false;
            break;
        case IdentificationMode::STANDARD:
            min_confidence_threshold = DEFAULT_STANDARD_THRESHOLD;
            break;
        case IdentificationMode::COMPREHENSIVE:
            min_confidence_threshold = DEFAULT_COMPREHENSIVE_THRESHOLD;
            enable_tension_analysis = true;
            break;
        case IdentificationMode::ANALYTICAL:
            min_confidence_threshold = DEFAULT_ANALYTICAL_THRESHOLD;
            enable_tension_analysis = true;
            break;
    }
}

bool ChordIdentifier::initialize(const std::string& chord_dict_path, const std::string& aliases_path) {
    if (!chord_database) {
        return false;
    }
    
    bool success = chord_database->loadFromYaml(chord_dict_path, aliases_path);
    
    if (success && !chord_database->validateDatabase()) {
        std::cerr << "Warning: Chord database validation failed" << std::endl;
    }
    
    return success;
}

bool ChordIdentifier::isInitialized() const {
    return chord_database && interval_engine && chord_database->getChordCount() > 0;
}

ChordIdentificationResult ChordIdentifier::identify(const std::vector<int>& midi_notes) const {
    return identify(midi_notes, current_mode);
}

ChordIdentificationResult ChordIdentifier::identify(const std::vector<int>& midi_notes, IdentificationMode mode) const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ChordIdentificationResult result;
    result.input_notes = midi_notes;
    result.note_names = getNoteNames(midi_notes);
    
    if (!isInitialized()) {
        result.chord_name = "ERROR: Not initialized";
        return result;
    }
    
    // Validate input
    if (midi_notes.empty() || midi_notes.size() > 16) {
        result.chord_name = "INVALID";
        return result;
    }
    
    // Route to appropriate identification method
    switch (mode) {
        case IdentificationMode::FAST:
            result = identifyFast(midi_notes);
            break;
        case IdentificationMode::STANDARD:
            result = identifyStandard(midi_notes);
            break;
        case IdentificationMode::COMPREHENSIVE:
            result = identifyComprehensive(midi_notes);
            break;
        case IdentificationMode::ANALYTICAL:
            result = identifyAnalytical(midi_notes);
            break;
    }
    
    // Calculate processing time
    auto end_time = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Update statistics
    total_identifications++;
    total_processing_time += result.processing_time;
    
    return result;
}

ChordIdentificationResult ChordIdentifier::identify(const std::vector<int>& midi_notes, int specified_bass) const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ChordIdentificationResult result;
    result.input_notes = midi_notes;
    result.note_names = getNoteNames(midi_notes);
    
    if (!isInitialized()) {
        result.chord_name = "ERROR: Not initialized";
        return result;
    }
    
    // Calculate intervals with specified bass
    auto interval_result = interval_engine->calculateIntervals(midi_notes, specified_bass);
    result.identified_intervals = interval_result.intervals;
    result.bass_note_name = midiToNoteName(specified_bass);
    
    // Find chord match
    auto matches = chord_database->findMatches(interval_result.intervals, enable_inversion_detection);
    
    if (!matches.empty()) {
        const auto& best_match = matches[0];
        
        if (best_match.confidence >= min_confidence_threshold) {
            result.chord_name = best_match.chord_info.name;
            result.confidence = best_match.confidence;
            result.is_inversion = best_match.is_inversion;
            result.chord_quality = best_match.chord_info.category;
            result.chord_category = best_match.chord_info.category;
            
            // Check for slash chord
            if (enable_slash_chord_detection && interval_result.bass_note != interval_result.root_note) {
                result.is_slash_chord = true;
                result.bass_note_name = midiToNoteName(interval_result.bass_note);
            }
            
            // Get alternative names
            result.alternative_names = findAlternativeNames(best_match);
        }
    }
    
    if (result.chord_name.empty()) {
        result.chord_name = "UNKNOWN";
        result.confidence = 0.0f;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    total_identifications++;
    total_processing_time += result.processing_time;
    
    return result;
}

ChordIdentificationResult ChordIdentifier::identifyFast(const std::vector<int>& midi_notes) const {
    ChordIdentificationResult result;
    result.input_notes = midi_notes;
    result.note_names = getNoteNames(midi_notes);
    
    // Fast path: exact match only
    auto interval_result = interval_engine->calculateIntervals(midi_notes);
    result.identified_intervals = interval_result.intervals;
    
    auto exact_match = chord_database->findExactMatch(interval_result.intervals);
    
    if (exact_match && exact_match->confidence >= min_confidence_threshold) {
        // Enrich the match with detailed analysis
        enrichChordMatch(*exact_match, interval_result, midi_notes);
        
        // Generate proper chord name using ChordNameGenerator
        auto chord_name_result = name_generator->generateChordName(*exact_match, midi_notes, interval_result.intervals);
        
        result.chord_name = exact_match->chord_info.name;  // Internal name for compatibility
        result.theoretical_name = chord_name_result.chord_name;  // Actual chord name like "Cm7"
        result.full_display_name = chord_name_result.full_name;  // With slash notation
        result.root_note = chord_name_result.root_note;
        result.chord_symbol = chord_name_result.chord_symbol;
        result.inversion_type = chord_name_result.inversion_type;
        
        result.confidence = exact_match->confidence;
        result.chord_quality = exact_match->chord_info.category;
        result.chord_category = exact_match->chord_info.category;
        result.is_slash_chord = chord_name_result.is_slash_chord;
        result.is_inversion = exact_match->is_inversion;
        
        // Set bass note for slash chords
        if (chord_name_result.is_slash_chord) {
            result.bass_note_name = chord_name_result.bass_note;
        }
        
        result.used_cache = true;
    } else {
        result.chord_name = "UNKNOWN";
        result.theoretical_name = "UNKNOWN";
        result.full_display_name = "UNKNOWN";
        result.confidence = 0.0f;
    }
    
    return result;
}

ChordIdentificationResult ChordIdentifier::identifyStandard(const std::vector<int>& midi_notes) const {
    ChordIdentificationResult result;
    result.input_notes = midi_notes;
    result.note_names = getNoteNames(midi_notes);
    
    auto interval_result = interval_engine->calculateIntervals(midi_notes);
    result.identified_intervals = interval_result.intervals;
    
    // Try exact match first
    auto matches = chord_database->findMatches(interval_result.intervals, enable_inversion_detection);
    
    if (!matches.empty()) {
        auto best_match = matches[0];  // Remove const to allow modification
        
        if (best_match.confidence >= min_confidence_threshold) {
            // Enrich the match with detailed analysis
            enrichChordMatch(best_match, interval_result, midi_notes);
            
            // Generate proper chord name using ChordNameGenerator
            auto chord_name_result = name_generator->generateChordName(best_match, midi_notes, interval_result.intervals);
            
            result.chord_name = best_match.chord_info.name;  // Internal name
            result.theoretical_name = chord_name_result.chord_name;  // "Cm7"
            result.full_display_name = chord_name_result.full_name;  // "Cm7/Bb"
            result.root_note = chord_name_result.root_note;
            result.chord_symbol = chord_name_result.chord_symbol;
            result.inversion_type = chord_name_result.inversion_type;
            
            result.confidence = best_match.confidence;
            result.is_inversion = best_match.is_inversion;
            result.chord_quality = best_match.chord_info.category;
            result.chord_category = best_match.chord_info.category;
            result.is_slash_chord = chord_name_result.is_slash_chord;
            
            // Set bass note for slash chords
            if (chord_name_result.is_slash_chord) {
                result.bass_note_name = chord_name_result.bass_note;
            }
            
            result.alternative_names = findAlternativeNames(best_match);
        }
    }
    
    if (result.chord_name.empty()) {
        result.chord_name = "UNKNOWN";
        result.confidence = 0.0f;
    }
    
    return result;
}

ChordIdentificationResult ChordIdentifier::identifyComprehensive(const std::vector<int>& midi_notes) const {
    auto result = identifyStandard(midi_notes);
    
    // If standard identification failed or has low confidence, try more analysis
    if (result.confidence < 0.8f) {
        // Try with tensions
        if (enable_tension_analysis) {
            auto tension_matches = chord_database->findWithTensions(result.identified_intervals);
            if (!tension_matches.empty() && tension_matches[0].confidence > result.confidence) {
                const auto& best_tension = tension_matches[0];
                result.chord_name = best_tension.chord_info.name;
                result.confidence = best_tension.confidence;
                result.chord_quality = best_tension.chord_info.category;
            }
        }
        
        // Try with omissions
        auto omission_matches = chord_database->findWithOmissions(result.identified_intervals);
        if (!omission_matches.empty() && omission_matches[0].confidence > result.confidence) {
            const auto& best_omission = omission_matches[0];
            result.chord_name = best_omission.chord_info.name + "(omit)";
            result.confidence = best_omission.confidence * 0.8f; // Penalty for omission
        }
    }
    
    return result;
}

ChordIdentificationResult ChordIdentifier::identifyAnalytical(const std::vector<int>& midi_notes) const {
    auto result = identifyComprehensive(midi_notes);
    
    // Get multiple candidates for analysis
    auto all_matches = chord_database->findBestMatches(result.identified_intervals, 5);
    
    for (const auto& match : all_matches) {
        if (match.confidence >= min_confidence_threshold) {
            std::string alt_name = match.chord_info.name;
            if (match.is_inversion) alt_name += " (inv)";
            result.alternative_names.push_back(alt_name);
        }
    }
    
    return result;
}

std::string ChordIdentifier::determineSlashChord(const IntervalResult& interval_result, const ChordMatch& base_match) const {
    if (interval_result.bass_note == interval_result.root_note) {
        return ""; // Not a slash chord
    }
    
    // Check if bass note is significantly lower (more than an octave)
    if (interval_result.bass_note < interval_result.root_note - 12) {
        std::string root_name = midiToNoteName(interval_result.root_note);
        std::string bass_name = midiToNoteName(interval_result.bass_note);
        
        // Extract just the note class (no octave)
        root_name = root_name.substr(0, root_name.find_last_of("0123456789"));
        bass_name = bass_name.substr(0, bass_name.find_last_of("0123456789"));
        
        return base_match.chord_info.name + "/" + bass_name;
    }
    
    return "";
}

std::vector<std::string> ChordIdentifier::findAlternativeNames(const ChordMatch& match) const {
    std::vector<std::string> alternatives;
    
    // Get aliases from database
    auto aliases = chord_database->getAliases(match.chord_info.name);
    alternatives.insert(alternatives.end(), aliases.begin(), aliases.end());
    
    return alternatives;
}

std::vector<ChordIdentificationResult> ChordIdentifier::identifyBatch(const std::vector<std::vector<int>>& note_sequences) const {
    std::vector<ChordIdentificationResult> results;
    results.reserve(note_sequences.size());
    
    for (const auto& notes : note_sequences) {
        results.push_back(identify(notes));
    }
    
    return results;
}

ChordIdentificationResult ChordIdentifier::transpose(const ChordIdentificationResult& result, int semitones) const {
    ChordIdentificationResult transposed = result;
    
    // Transpose input notes
    transposed.input_notes = transposeNotes(result.input_notes, semitones);
    transposed.note_names = getNoteNames(transposed.input_notes);
    
    // Transpose bass note if present
    if (!result.bass_note_name.empty()) {
        int bass_midi = -1; // Would need to parse from bass_note_name
        if (bass_midi >= 0) {
            int new_bass = bass_midi + semitones;
            if (new_bass >= 0 && new_bass <= 127) {
                transposed.bass_note_name = midiToNoteName(new_bass);
            }
        }
    }
    
    // Chord intervals remain the same, only root changes
    return transposed;
}

void ChordIdentifier::clearCaches() {
    if (interval_engine) {
        interval_engine->clearCache();
    }
    if (chord_database) {
        chord_database->clearCaches();
    }
    
    // Reset performance stats
    cache_hits = 0;
}

void ChordIdentifier::warmupCaches(const std::vector<std::vector<int>>& common_patterns) {
    if (interval_engine) {
        interval_engine->warmupCache(common_patterns);
    }
    if (chord_database) {
        chord_database->warmupCache(common_patterns);
    }
}

std::vector<std::string> ChordIdentifier::getSupportedStyles() const {
    return {"standard", "jazz", "classical", "popular", "minimal"};
}

std::vector<std::string> ChordIdentifier::getAvailableChords() const {
    return chord_database ? chord_database->getAllChordNames() : std::vector<std::string>{};
}

ChordIdentifier::PerformanceStats ChordIdentifier::getPerformanceStats() const {
    PerformanceStats stats;
    stats.total_identifications = total_identifications;
    stats.cache_hits = cache_hits;
    stats.cache_hit_rate = (total_identifications > 0) ? 
                           static_cast<float>(cache_hits) / static_cast<float>(total_identifications) : 0.0f;
    stats.total_processing_time = total_processing_time;
    stats.average_processing_time = (total_identifications > 0) ?
                                   std::chrono::microseconds(total_processing_time.count() / total_identifications) :
                                   std::chrono::microseconds(0);
    
    return stats;
}

void ChordIdentifier::resetPerformanceStats() {
    total_identifications = 0;
    cache_hits = 0;
    total_processing_time = std::chrono::microseconds(0);
}

bool ChordIdentifier::validateConfiguration() const {
    if (!isInitialized()) {
        return false;
    }
    
    if (min_confidence_threshold < 0.0f || min_confidence_threshold > 1.0f) {
        return false;
    }
    
    return true;
}

std::string ChordIdentifier::getConfigurationSummary() const {
    std::stringstream ss;
    ss << "ChordIdentifier Configuration:\n";
    ss << "  Mode: " << static_cast<int>(current_mode) << "\n";
    ss << "  Min Confidence: " << min_confidence_threshold << "\n";
    ss << "  Slash Chords: " << (enable_slash_chord_detection ? "enabled" : "disabled") << "\n";
    ss << "  Inversions: " << (enable_inversion_detection ? "enabled" : "disabled") << "\n";
    ss << "  Tensions: " << (enable_tension_analysis ? "enabled" : "disabled") << "\n";
    ss << "  Naming Style: " << preferred_naming_style << "\n";
    ss << "  Initialized: " << (isInitialized() ? "yes" : "no") << "\n";
    
    if (isInitialized()) {
        ss << "  Chord Count: " << chord_database->getChordCount() << "\n";
    }
    
    return ss.str();
}

std::string ChordIdentificationResult::getDisplayName(const std::string& style) const {
    if (style == "minimal") {
        // Return minimal notation
        return chord_name;
    } else if (style == "jazz") {
        // Jazz-style notation preferences
        return getFullName();
    } else {
        // Standard notation
        return getFullName();
    }
}

// Enhanced analysis methods implementation

void ChordIdentifier::enrichChordMatch(ChordMatch& match, const IntervalResult& interval_result, const std::vector<int>& /* midi_notes */) const {
    if (!note_converter) return;
    
    // Set MIDI note information
    match.bass_note_midi = interval_result.bass_note;
    match.root_note_midi = interval_result.root_note;
    
    // Enhanced inversion detection before slash chord determination
    std::string actual_chord_name = match.chord_info.name;
    std::string root_note_name = "";
    
    // Try to detect common inversions and convert to proper slash notation
    if (interval_result.intervals.size() >= 3) {
        // Check for major triad inversions
        if (interval_result.intervals == std::vector<int>{0, 3, 8}) {
            // First inversion of major triad (3rd in bass) - [0,3,8] = bass + minor3rd + minor6th
            // Root is 8 semitones up from bass (minor6th becomes major3rd in root position)
            actual_chord_name = "major-triad";
            match.root_note_midi = match.bass_note_midi + 8; // Minor sixth up to get root
            match.is_slash_chord = true;
        } else if (interval_result.intervals == std::vector<int>{0, 5, 9}) {
            // Second inversion of major triad (5th in bass) - [0,5,9] = bass + perfect4th + major6th
            // Root is 5 semitones up from bass 
            actual_chord_name = "major-triad";
            match.root_note_midi = match.bass_note_midi + 5; // Perfect fourth up to get root
            match.is_slash_chord = true;
        }
        // Check for minor triad inversions
        else if (interval_result.intervals == std::vector<int>{0, 4, 9}) {
            // First inversion of minor triad (3rd in bass) - [0,4,9] = bass + major3rd + major6th
            // Root is 9 semitones up from bass
            actual_chord_name = "minor-triad";
            match.root_note_midi = match.bass_note_midi + 9; // Major sixth up to get root
            match.is_slash_chord = true;
        } else if (interval_result.intervals == std::vector<int>{0, 5, 8}) {
            // Second inversion of minor triad (5th in bass) - [0,5,8] = bass + perfect4th + minor6th
            // Root is 5 semitones up from bass
            actual_chord_name = "minor-triad";
            match.root_note_midi = match.bass_note_midi + 5; // Perfect fourth up to get root
            match.is_slash_chord = true;
        }
    }
    
    // Update chord name if we detected an inversion
    if (match.is_slash_chord && actual_chord_name != match.chord_info.name) {
        match.chord_info.name = actual_chord_name;
        // Re-extract category and quality for the corrected chord
        auto [category, quality] = extractCategoryAndQuality(actual_chord_name);
        match.chord_info.category = category;
        if (!quality.empty()) {
            match.chord_info.category = category + " (" + quality + ")";
        }
    }
    
    // Determine if this is a slash chord (either detected above or bass != root)
    if (!match.is_slash_chord) {
        match.is_slash_chord = (match.bass_note_midi != match.root_note_midi) && 
                              (match.bass_note_midi != -1) && (match.root_note_midi != -1);
    }
    
    // Set bass note name for slash notation
    if (match.bass_note_midi != -1) {
        match.bass_note_name = note_converter->midiToNoteName(match.bass_note_midi, AccidentalStyle::SHARPS, OctaveNotation::NO_OCTAVE);
    }
    
    // Determine inversion type
    match.inversion_type = determineInversionType(interval_result.intervals, match.chord_info.name);
    
    // Extract category and quality
    auto [category, quality] = extractCategoryAndQuality(match.chord_info.name);
    match.chord_info.category = category;
    
    // Set proper quality in the result structure (we need to add this field)
    // For now, store in category as compound info
    if (!quality.empty()) {
        match.chord_info.category = category + " (" + quality + ")";
    }
}

int ChordIdentifier::determineInversionType(const std::vector<int>& intervals, const std::string& chord_type) const {
    if (intervals.empty()) return 0;
    
    // Root position: starts with 0
    if (intervals[0] == 0) return 0;
    
    // For triads
    if (chord_type.find("triad") != std::string::npos && intervals.size() >= 3) {
        // Major triad inversions: [0,4,7] -> [0,3,8] (1st) -> [0,5,9] (2nd)
        // Minor triad inversions: [0,3,7] -> [0,4,9] (1st) -> [0,5,8] (2nd)
        
        if ((intervals.size() >= 3 && intervals[1] == 3 && intervals[2] == 8) ||  // Major 1st
            (intervals.size() >= 3 && intervals[1] == 4 && intervals[2] == 9)) {  // Minor 1st
            return 1; // First inversion
        }
        
        if ((intervals.size() >= 3 && intervals[1] == 5 && intervals[2] == 9) ||  // Major 2nd
            (intervals.size() >= 3 && intervals[1] == 5 && intervals[2] == 8)) {  // Minor 2nd
            return 2; // Second inversion
        }
    }
    
    // For seventh chords
    if (chord_type.find("seventh") != std::string::npos && intervals.size() >= 4) {
        // More complex inversion detection for seventh chords
        // This is a simplified version - could be expanded
        if (intervals[0] != 0) {
            return 1; // Some inversion, simplified
        }
    }
    
    return 0; // Root position (default)
}

std::pair<std::string, std::string> ChordIdentifier::extractCategoryAndQuality(const std::string& chord_name) const {
    // Extract category and quality from chord name
    std::string category, quality;
    
    if (chord_name.find("triad") != std::string::npos) {
        category = "triad";
        
        if (chord_name.find("major") != std::string::npos) {
            quality = "major";
        } else if (chord_name.find("minor") != std::string::npos) {
            quality = "minor";
        } else if (chord_name.find("diminished") != std::string::npos) {
            quality = "diminished";
        } else if (chord_name.find("augmented") != std::string::npos) {
            quality = "augmented";
        } else if (chord_name.find("sus") != std::string::npos) {
            quality = "suspended";
        }
    } else if (chord_name.find("seventh") != std::string::npos) {
        category = "seventh";
        
        if (chord_name.find("major-seventh") != std::string::npos) {
            quality = "major";
        } else if (chord_name.find("minor-seventh") != std::string::npos) {
            quality = "minor";
        } else if (chord_name.find("dominant") != std::string::npos) {
            quality = "dominant";
        } else if (chord_name.find("diminished") != std::string::npos) {
            quality = "diminished";
        } else if (chord_name.find("half-diminished") != std::string::npos) {
            quality = "half-diminished";
        }
    } else if (chord_name.find("ninth") != std::string::npos || 
               chord_name.find("eleventh") != std::string::npos ||
               chord_name.find("thirteenth") != std::string::npos) {
        category = "extended";
        
        if (chord_name.find("major") != std::string::npos) {
            quality = "major";
        } else if (chord_name.find("minor") != std::string::npos) {
            quality = "minor";
        } else if (chord_name.find("dominant") != std::string::npos) {
            quality = "dominant";
        }
    } else if (chord_name.find("scale") != std::string::npos) {
        category = "scale";
        quality = "modal";
    } else if (chord_name.find("quartal") != std::string::npos) {
        category = "quartal";
        quality = "quartal";
    } else if (chord_name.find("cluster") != std::string::npos) {
        category = "cluster";
        quality = "chromatic";
    } else {
        category = "other";
        quality = "other";
    }
    
    return {category, quality};
}


} // namespace ChordLock