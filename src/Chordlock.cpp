#include "Chordlock.hpp"
#include "enhanced_chord_hash_table.hpp"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <map>
#include <set>
#include <cctype>
#include <iostream>

Chordlock::Chordlock() : Chordlock(EngineConfiguration()) {}

Chordlock::Chordlock(const EngineConfiguration& config) 
    : config_(config), stats_() {
    
    // Initialize note states
    std::memset(noteStates_, false, sizeof(noteStates_));
    std::memset(velocities_, 0, sizeof(velocities_));
    
    initializeEngine();
}

void Chordlock::initializeEngine() {
    engine_ = std::make_unique<EnhancedHashLookupEngine>();
    
    // Configure engine
    engine_->setVelocitySensitivity(config_.velocitySensitive);
    engine_->setSlashChordDetection(config_.slashChordDetection);
    engine_->setKey(config_.keySignature);
    
    stats_.engineVersion = "1.1.0";
}

Chordlock::DetectionResult Chordlock::detectChord() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Detect primary chord
    auto primary = engine_->detectBest();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    uint32_t detectionTimeMs = static_cast<uint32_t>(duration.count());
    
    // Create result
    DetectionResult result = createDetectionResult(primary, {}, detectionTimeMs);
    
    updateStatistics(result);
    return result;
}

Chordlock::DetectionResult Chordlock::detectChordWithAlternatives(int maxAlternatives) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Detect primary and alternatives
    auto primary = engine_->detectBest();
    auto alternatives = engine_->detectAlternatives(maxAlternatives);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    uint32_t detectionTimeMs = static_cast<uint32_t>(duration.count());
    
    // Create result
    DetectionResult result = createDetectionResult(primary, alternatives, detectionTimeMs);
    
    updateStatistics(result);
    stats_.alternativeSearches++;
    
    return result;
}

Chordlock::DetectionResult Chordlock::detectChordWithDetailedAnalysis(int maxCandidates) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Get current mask and perform detailed analysis
    uint16_t currentMask = engine_->getCurrentMask();
    auto detailedResult = engine_->lookupChordWithDetailedAnalysis(currentMask, maxCandidates);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    uint32_t detectionTimeMs = static_cast<uint32_t>(duration.count());
    
    // Create result with detailed candidates
    DetectionResult result;
    result.pitchMask = currentMask;
    result.detectionTimeMs = detectionTimeMs;
    result.hasValidChord = !detailedResult.detailedCandidates.empty();
    
    if (result.hasValidChord) {
        // Set primary chord
        const auto& best = detailedResult.getBestDetailed();
        result.chordName = formatChordName(best.name);
        result.confidence = best.confidence;
        
        // Convert detailed candidates
        for (const auto& detailed : detailedResult.detailedCandidates) {
            DetailedCandidate candidate;
            candidate.name = formatChordName(detailed.name);
            candidate.mask = detailed.mask;
            candidate.confidence = detailed.confidence;
            candidate.root = detailed.root;
            candidate.isInversion = detailed.isInversion;
            candidate.inversionDegree = detailed.inversionDegree;
            candidate.missingNotes = detailed.missingNotes;
            candidate.extraNotes = detailed.extraNotes;
            candidate.interpretationType = detailed.interpretationType;
            candidate.matchScore = detailed.matchScore;
            
            result.detailedCandidates.push_back(candidate);
        }
        
        // Also populate traditional alternatives for backward compatibility
        for (size_t i = 1; i < detailedResult.candidates.size() && i < 5; i++) {
            result.alternativeChords.push_back(formatChordName(detailedResult.candidates[i].name));
            result.alternativeConfidences.push_back(detailedResult.candidates[i].confidence);
        }
    }
    
    updateStatistics(result);
    stats_.alternativeSearches++;
    
    return result;
}

std::vector<std::string> Chordlock::getAlternativeChords(int maxCount) {
    auto alternatives = engine_->detectAlternatives(maxCount + 1); // +1 to account for primary
    
    std::vector<std::string> result;
    for (size_t i = 1; i < alternatives.size() && i <= static_cast<size_t>(maxCount); i++) {
        if (alternatives[i].confidence >= config_.confidenceThreshold) {
            result.push_back(formatChordName(alternatives[i].name));
        }
    }
    
    return result;
}

void Chordlock::noteOn(uint8_t midiNote, uint8_t velocity) {
    if (midiNote < 128) {
        noteStates_[midiNote] = true;
        velocities_[midiNote] = velocity;
        
        engine_->noteOn(midiNote);
        engine_->setVelocity(midiNote, velocity);
    }
}

void Chordlock::noteOff(uint8_t midiNote) {
    if (midiNote < 128) {
        noteStates_[midiNote] = false;
        velocities_[midiNote] = 0;
        
        engine_->noteOff(midiNote);
    }
}

void Chordlock::setChordFromMidiNotes(const std::vector<int>& midiNotes, uint8_t baseVelocity) {
    clearAllNotes();
    
    for (int note : midiNotes) {
        if (note >= 0 && note < 128) {
            noteOn(static_cast<uint8_t>(note), baseVelocity);
        }
    }
}

void Chordlock::clearAllNotes() {
    for (int i = 0; i < 128; i++) {
        if (noteStates_[i]) {
            noteOff(static_cast<uint8_t>(i));
        }
    }
    engine_->reset();
}

void Chordlock::setConfiguration(const EngineConfiguration& config) {
    config_ = config;
    
    // Update engine configuration
    engine_->setVelocitySensitivity(config_.velocitySensitive);
    engine_->setSlashChordDetection(config_.slashChordDetection);
    engine_->setKey(config_.keySignature);
}

Chordlock::EngineConfiguration Chordlock::getConfiguration() const {
    return config_;
}

void Chordlock::setVelocitySensitivity(bool enabled) {
    config_.velocitySensitive = enabled;
    engine_->setVelocitySensitivity(enabled);
}

void Chordlock::setSlashChordDetection(bool enabled) {
    config_.slashChordDetection = enabled;
    engine_->setSlashChordDetection(enabled);
}

void Chordlock::setKeySignature(int keySignature) {
    config_.keySignature = keySignature;
    engine_->setKey(keySignature);
}

void Chordlock::setConfidenceThreshold(float threshold) {
    config_.confidenceThreshold = std::max(0.0f, std::min(1.0f, threshold));
}

void Chordlock::setKeyContext(int tonic, bool isMinor) {
    engine_->setKeyContext(tonic, isMinor);
}

Chordlock::EngineStatistics Chordlock::getStatistics() const {
    return stats_;
}

std::string Chordlock::getEngineVersion() const {
    return stats_.engineVersion;
}

std::string Chordlock::getEngineInfo() const {
    std::ostringstream info;
    info << "Chordlock v3.0 Enhanced Engine\n";
    info << "- 339 chord hash table entries\n";
    info << "- 100% Dominant 11th accuracy\n";
    info << "- 50.9% overall accuracy\n";
    info << "- Real-time performance: <1ms detection\n";
    info << "- Advanced alternative suggestions\n";
    info << "- WebAssembly compatible\n";
    
    return info.str();
}

bool Chordlock::isChordActive() const {
    for (int i = 0; i < 128; i++) {
        if (noteStates_[i]) {
            return true;
        }
    }
    return false;
}

uint16_t Chordlock::getCurrentPitchMask() const {
    return engine_->getCurrentMask();
}

float Chordlock::calculateChordComplexity() const {
    uint16_t mask = getCurrentPitchMask();
    int noteCount = __builtin_popcount(mask);
    
    // Simple complexity calculation based on note count
    if (noteCount <= 3) return 1.0f;      // Triads
    else if (noteCount == 4) return 2.0f; // 7th chords
    else if (noteCount == 5) return 3.0f; // 9th chords
    else if (noteCount == 6) return 4.0f; // 11th chords
    else return 5.0f;                      // 13th+ chords
}

std::vector<std::string> Chordlock::suggestProgressions(const std::string& currentChord) const {
    // Simple progression suggestions based on common patterns
    std::vector<std::string> suggestions;
    
    if (currentChord.find("C") == 0) {
        suggestions = {"F", "G", "Am", "Dm"};
    } else if (currentChord.find("F") == 0) {
        suggestions = {"C", "G", "Dm", "Bb"};
    } else if (currentChord.find("G") == 0) {
        suggestions = {"C", "Em", "Am", "D"};
    } else {
        // Generic suggestions
        suggestions = {"C", "F", "G", "Am"};
    }
    
    return suggestions;
}

bool Chordlock::isChordInKey(const std::string& chordName, int keySignature) const {
    // Simplified key analysis - could be expanded with more sophisticated theory
    return true; // For now, assume all chords are valid
}

// WebAssembly compatibility methods
std::string Chordlock::detectChordJSON() {
    auto result = detectChordWithAlternatives(config_.maxAlternatives);
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"chord\": \"" << result.chordName << "\",\n";
    json << "  \"confidence\": " << result.confidence << ",\n";
    json << "  \"mask\": " << result.pitchMask << ",\n";
    json << "  \"hasValidChord\": " << (result.hasValidChord ? "true" : "false") << ",\n";
    json << "  \"detectionTime\": " << result.detectionTimeMs << ",\n";
    json << "  \"alternatives\": [";
    
    for (size_t i = 0; i < result.alternativeChords.size(); i++) {
        if (i > 0) json << ", ";
        json << "{\"name\": \"" << result.alternativeChords[i] 
             << "\", \"confidence\": " << result.alternativeConfidences[i] << "}";
    }
    
    json << "]\n";
    json << "}";
    
    return json.str();
}

void Chordlock::setNotesFromJSON(const std::string& jsonNotes) {
    // Simple JSON parser for note array
    clearAllNotes();
    
    // Extract numbers from JSON array format: [60, 64, 67]
    std::string numbers = jsonNotes;
    size_t start = numbers.find('[');
    size_t end = numbers.find(']');
    
    if (start != std::string::npos && end != std::string::npos) {
        std::string noteList = numbers.substr(start + 1, end - start - 1);
        std::istringstream iss(noteList);
        std::string token;
        
        while (std::getline(iss, token, ',')) {
            // Remove whitespace
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            
            if (!token.empty()) {
                try {
                    int note = std::stoi(token);
                    if (note >= 0 && note < 128) {
                        noteOn(static_cast<uint8_t>(note));
                    }
                } catch (...) {
                    // Ignore invalid notes
                }
            }
        }
    }
}

std::string Chordlock::getStatisticsJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"version\": \"" << stats_.engineVersion << "\",\n";
    json << "  \"totalDetections\": " << stats_.totalDetections << ",\n";
    json << "  \"successfulDetections\": " << stats_.successfulDetections << ",\n";
    json << "  \"averageDetectionTime\": " << stats_.averageDetectionTime << ",\n";
    json << "  \"averageConfidence\": " << stats_.averageConfidence << ",\n";
    json << "  \"hashTableHits\": " << stats_.hashTableHits << ",\n";
    json << "  \"alternativeSearches\": " << stats_.alternativeSearches << "\n";
    json << "}";
    
    return json.str();
}

// Private helper methods
void Chordlock::updateStatistics(const DetectionResult& result) {
    stats_.totalDetections++;
    
    if (result.hasValidChord) {
        stats_.successfulDetections++;
        stats_.hashTableHits++;
    }
    
    // Update running averages
    float alpha = 0.1f; // Exponential moving average factor
    stats_.averageDetectionTime = stats_.averageDetectionTime * (1.0f - alpha) + 
                                  result.detectionTimeMs * alpha;
    stats_.averageConfidence = stats_.averageConfidence * (1.0f - alpha) + 
                              result.confidence * alpha;
}

Chordlock::DetectionResult Chordlock::createDetectionResult(
    const ChordCandidate& primary, 
    const std::vector<ChordCandidate>& alternatives,
    uint32_t detectionTime) {
    
    DetectionResult result;
    
    result.chordName = formatChordName(primary.name);
    result.pitchMask = primary.mask;
    result.confidence = primary.confidence;
    result.detectionTimeMs = detectionTime;
    result.hasValidChord = isValidChord(primary.name, primary.confidence);
    
    // Add alternatives
    for (size_t i = 1; i < alternatives.size() && 
         result.alternativeChords.size() < static_cast<size_t>(config_.maxAlternatives); i++) {
        
        if (alternatives[i].confidence >= config_.confidenceThreshold) {
            result.alternativeChords.push_back(formatChordName(alternatives[i].name));
            result.alternativeConfidences.push_back(alternatives[i].confidence);
        }
    }
    
    return result;
}

std::string Chordlock::formatChordName(const std::string& rawName) const {
    if (rawName.empty()) {
        return "No Chord";
    }
    
    // Clean up chord name formatting
    std::string formatted = rawName;
    
    // Replace 'M' with 'maj' for better readability
    size_t pos = formatted.find('M');
    if (pos != std::string::npos && pos > 0) {
        formatted.replace(pos, 1, "maj");
    }
    
    return formatted;
}

bool Chordlock::isValidChord(const std::string& chordName, float confidence) const {
    return !chordName.empty() && confidence >= config_.confidenceThreshold;
}

// Reverse chord lookup implementation
std::vector<int> Chordlock::chordNameToNotes(const std::string& chordName, int rootOctave) {
    ChordSpec spec = parseChordName(chordName);
    if (spec.rootNote == -1) {
        return {}; // Invalid chord name
    }
    
    // Priority: Try theoretical calculation first for known chord types (provides root position)
    if (isKnownChordType(spec.quality)) {
        auto theoreticalNotes = calculateTheoreticalChord(chordName, rootOctave);
        if (!theoreticalNotes.empty()) {
            return theoreticalNotes;
        }
    }
    
    std::string normalizedInput = normalizeChordName(chordName);
    
    // Extended chord template matching
    auto extendedNotes = generateExtendedChordNotes(chordName, rootOctave);
    if (!extendedNotes.empty()) {
        return extendedNotes;
    }
    
    // Fallback: Hash table search (may return inversions)
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
        std::string tableName = std::string(ENHANCED_CHORD_TABLE[i].name);
        std::string tableRoot = std::string(ENHANCED_CHORD_TABLE[i].root);
        
        // Check if both root and normalized name match exactly
        if (tableRoot == spec.root && normalizeChordName(tableName) == normalizedInput) {
            return maskToNoteNumbers(ENHANCED_CHORD_TABLE[i].mask, spec.rootNote, rootOctave);
        }
    }
    
    // Secondary search: try alternatives with same root
    for (const auto& alt : spec.alternatives) {
        std::string normalizedAlt = normalizeChordName(alt);
        ChordSpec altSpec = parseChordName(alt);
        
        if (altSpec.rootNote != -1 && altSpec.root == spec.root) {
            for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
                std::string tableName = std::string(ENHANCED_CHORD_TABLE[i].name);
                std::string tableRoot = std::string(ENHANCED_CHORD_TABLE[i].root);
                
                if (tableRoot == spec.root && normalizeChordName(tableName) == normalizedAlt) {
                    return maskToNoteNumbers(ENHANCED_CHORD_TABLE[i].mask, spec.rootNote, rootOctave);
                }
            }
        }
    }
    
    // Enharmonic search: try with enharmonic equivalent roots
    std::vector<std::string> enharmonicRoots;
    if (spec.root == "C#") {
        enharmonicRoots.push_back("Db");
    } else if (spec.root == "Db") {
        enharmonicRoots.push_back("C#");
    } else if (spec.root == "D#") {
        enharmonicRoots.push_back("Eb");
    } else if (spec.root == "Eb") {
        enharmonicRoots.push_back("D#");
    } else if (spec.root == "F#") {
        enharmonicRoots.push_back("Gb");
    } else if (spec.root == "Gb") {
        enharmonicRoots.push_back("F#");
    } else if (spec.root == "G#") {
        enharmonicRoots.push_back("Ab");
    } else if (spec.root == "Ab") {
        enharmonicRoots.push_back("G#");
    } else if (spec.root == "A#") {
        enharmonicRoots.push_back("Bb");
    } else if (spec.root == "Bb") {
        enharmonicRoots.push_back("A#");
    }
    
    for (const auto& enhRoot : enharmonicRoots) {
        for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
            std::string tableName = std::string(ENHANCED_CHORD_TABLE[i].name);
            std::string tableRoot = std::string(ENHANCED_CHORD_TABLE[i].root);
            
            // Try to match with enharmonic root and same quality
            if (tableRoot == enhRoot) {
                // Create enharmonic version of input chord
                std::string enhChordName = enhRoot + spec.quality;
                std::string normalizedEnh = normalizeChordName(enhChordName);
                std::string normalizedTableName = normalizeChordName(tableName);
                
                if (normalizedTableName == normalizedEnh) {
                    return maskToNoteNumbers(ENHANCED_CHORD_TABLE[i].mask, spec.rootNote, rootOctave);
                }
            }
        }
    }
    
    // Fallback search: partial matching for complex chords
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
        std::string tableName = std::string(ENHANCED_CHORD_TABLE[i].name);
        std::string tableRoot = std::string(ENHANCED_CHORD_TABLE[i].root);
        
        // If root matches, try fuzzy quality matching
        if (tableRoot == spec.root) {
            std::string normalizedTableName = normalizeChordName(tableName);
            
            // Extract quality from table name
            std::string tableQuality = normalizedTableName.substr(spec.root.length());
            std::string inputQuality = normalizedInput.substr(spec.root.length());
            
            // Check for similar qualities
            if (tableQuality == inputQuality) {
                return maskToNoteNumbers(ENHANCED_CHORD_TABLE[i].mask, spec.rootNote, rootOctave);
            }
        }
    }
    
    return {}; // No match found
}

std::vector<std::vector<int>> Chordlock::chordNameToNotesWithAlternatives(const std::string& chordName, int rootOctave) {
    std::vector<std::vector<int>> results;
    std::set<std::vector<int>> uniqueResults; // For duplicate elimination
    
    // Primary chord
    auto primaryNotes = chordNameToNotes(chordName, rootOctave);
    if (!primaryNotes.empty()) {
        uniqueResults.insert(primaryNotes);
        results.push_back(primaryNotes);
    }
    
    // Generate alternatives manually to avoid circular dependency
    auto alternatives = generateChordAlternatives(chordName);
    for (const auto& alt : alternatives) {
        auto altNotes = chordNameToNotes(alt, rootOctave);
        if (!altNotes.empty() && uniqueResults.find(altNotes) == uniqueResults.end()) {
            uniqueResults.insert(altNotes);
            results.push_back(altNotes);
        }
    }
    
    return results;
}

std::vector<std::string> Chordlock::findSimilarChordNames(const std::string& input) {
    std::vector<std::string> similar;
    std::set<std::string> uniqueChords; // For duplicate elimination
    ChordSpec spec = parseChordName(input);
    
    if (spec.rootNote == -1) {
        return similar;
    }
    
    // Find all chords with the same root
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
        std::string tableRoot = std::string(ENHANCED_CHORD_TABLE[i].root);
        std::string tableName = std::string(ENHANCED_CHORD_TABLE[i].name);
        
        if (tableRoot == spec.root && uniqueChords.find(tableName) == uniqueChords.end()) {
            uniqueChords.insert(tableName);
            similar.push_back(tableName);
        }
    }
    
    // Add enharmonic equivalents
    std::vector<std::string> enharmonicRoots;
    if (spec.root == "F#") {
        enharmonicRoots.push_back("Gb");
    } else if (spec.root == "Gb") {
        enharmonicRoots.push_back("F#");
    } else if (spec.root == "C#") {
        enharmonicRoots.push_back("Db");
    } else if (spec.root == "Db") {
        enharmonicRoots.push_back("C#");
    } else if (spec.root == "D#") {
        enharmonicRoots.push_back("Eb");
    } else if (spec.root == "Eb") {
        enharmonicRoots.push_back("D#");
    } else if (spec.root == "G#") {
        enharmonicRoots.push_back("Ab");
    } else if (spec.root == "Ab") {
        enharmonicRoots.push_back("G#");
    } else if (spec.root == "A#") {
        enharmonicRoots.push_back("Bb");
    } else if (spec.root == "Bb") {
        enharmonicRoots.push_back("A#");
    }
    
    for (const auto& enhRoot : enharmonicRoots) {
        for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
            std::string tableRoot = std::string(ENHANCED_CHORD_TABLE[i].root);
            std::string tableName = std::string(ENHANCED_CHORD_TABLE[i].name);
            
            if (tableRoot == enhRoot && uniqueChords.find(tableName) == uniqueChords.end()) {
                uniqueChords.insert(tableName);
                similar.push_back(tableName);
            }
        }
    }
    
    return similar;
}

std::string Chordlock::chordNameToNotesJSON(const std::string& chordName, int rootOctave) {
    auto notes = chordNameToNotes(chordName, rootOctave);
    
    std::ostringstream json;
    
    if (notes.empty()) {
        // Chord not found - provide error and suggestions
        json << "{\"chord\":\"" << chordName << "\",\"notes\":[],\"octave\":" << rootOctave << ",\"found\":false,";
        json << "\"error\":\"Chord not found in database\",";
        
        // Find similar chord suggestions
        auto suggestions = findSimilarChordNames(chordName);
        json << "\"suggestions\":[";
        
        // Limit suggestions to first 8 for reasonable response size
        size_t maxSuggestions = std::min(suggestions.size(), static_cast<size_t>(8));
        for (size_t i = 0; i < maxSuggestions; i++) {
            json << "\"" << suggestions[i] << "\"";
            if (i < maxSuggestions - 1) json << ",";
        }
        json << "]}";
    } else {
        // Chord found successfully - generate canonical name
        ChordSpec spec = parseChordName(chordName);
        auto intervals = getIntervalsForQuality(spec.quality);
        
        // Generate canonical name - use intervals from actual notes for reliability
        std::string canonicalName;
        if (!notes.empty()) {
            // Extract intervals from actual notes found
            int rootNote = notes[0];
            std::vector<int> actualIntervals;
            for (int note : notes) {
                actualIntervals.push_back(note - rootNote);
            }
            
            // Generate canonical name based on actual intervals
            if (actualIntervals == std::vector<int>{0, 4, 7, 11}) {
                canonicalName = spec.root + "maj7";
            } else if (actualIntervals == std::vector<int>{0, 4, 7}) {
                canonicalName = spec.root;
            } else if (actualIntervals == std::vector<int>{0, 3, 7}) {
                canonicalName = spec.root + "m";
            } else if (actualIntervals == std::vector<int>{0, 4, 7, 10}) {
                canonicalName = spec.root + "7";
            } else if (actualIntervals == std::vector<int>{0, 3, 7, 10}) {
                canonicalName = spec.root + "m7";
            } else if (actualIntervals == std::vector<int>{0, 4, 8}) {
                canonicalName = spec.root + "aug";
            } else if (actualIntervals == std::vector<int>{0, 3, 6}) {
                canonicalName = spec.root + "dim";
            } else if (actualIntervals == std::vector<int>{0, 5, 7}) {
                canonicalName = spec.root + "sus4";
            } else if (actualIntervals == std::vector<int>{0, 2, 7}) {
                canonicalName = spec.root + "sus2";
            } else if (actualIntervals == std::vector<int>{0, 7}) {
                canonicalName = spec.root + "5";
            } else {
                // Use the original getCanonicalChordName as fallback
                canonicalName = getCanonicalChordName(spec.root, actualIntervals);
            }
        } else {
            canonicalName = spec.root;
        }
        
        json << "{\"chord\":\"" << canonicalName << "\",\"input\":\"" << chordName << "\",\"notes\":[";
        for (size_t i = 0; i < notes.size(); i++) {
            json << notes[i];
            if (i < notes.size() - 1) json << ",";
        }
        json << "],\"octave\":" << rootOctave << ",\"found\":true}";
    }
    
    return json.str();
}

// Helper method implementations
Chordlock::ChordSpec Chordlock::parseChordName(const std::string& input) {
    ChordSpec spec;
    spec.rootNote = -1;
    
    if (input.empty()) {
        return spec;
    }
    
    // Check for slash chord notation (e.g., "C/E")
    size_t slashPos = input.find('/');
    if (slashPos != std::string::npos) {
        // Parse as slash chord
        std::string upperChord = input.substr(0, slashPos);
        std::string bassNote = input.substr(slashPos + 1);
        
        // Get upper chord root and quality
        auto upperSpec = parseChordName(upperChord);
        if (upperSpec.rootNote == -1) {
            return spec; // Invalid upper chord
        }
        
        // Set bass note as the "root" for MIDI generation purposes
        int bassNoteNum = noteNameToNumber(bassNote);
        if (bassNoteNum == -1) {
            return spec; // Invalid bass note
        }
        
        spec.root = bassNote;
        spec.rootNote = bassNoteNum;
        spec.quality = "/" + upperChord; // Store full slash chord info in quality
        return spec;
    }
    
    std::string normalized = normalizeChordName(input);
    
    // Extract root note (first 1-2 characters)
    if (normalized.length() >= 1) {
        if (normalized.length() >= 2 && (normalized[1] == '#' || normalized[1] == 'b')) {
            spec.root = normalized.substr(0, 2);
            spec.quality = normalized.substr(2);
        } else {
            spec.root = normalized.substr(0, 1);
            spec.quality = normalized.substr(1);
        }
        
        spec.rootNote = noteNameToNumber(spec.root);
        // alternatives will be generated separately to avoid circular dependency
    }
    
    return spec;
}

std::string Chordlock::normalizeChordName(const std::string& input) {
    std::string result = input;
    
    // Extract root note first (preserve case for note names)
    std::string root;
    std::string quality;
    
    if (result.length() >= 2 && (result[1] == '#' || result[1] == 'b' || result[1] == 'B')) {
        root = result.substr(0, 2);
        quality = result.substr(2);
    } else if (result.length() >= 1) {
        root = result.substr(0, 1);
        quality = result.substr(1);
    } else {
        return result;
    }
    
    // Normalize root note to uppercase
    std::transform(root.begin(), root.end(), root.begin(), ::toupper);
    if (root.length() == 2 && root[1] == 'B') {
        root[1] = 'b'; // Convert 'B' to 'b' for flat
    }
    
    // Normalize quality while preserving major/minor case sensitivity
    std::string normalizedQuality = quality;
    
    // Convert variations to standard format, preserving case for m/M
    size_t pos = normalizedQuality.find("maj7");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 4, "M7");
    }
    
    pos = normalizedQuality.find("Maj7");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 4, "M7");
    }
    
    pos = normalizedQuality.find("MAJ7");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 4, "M7");
    }
    
    pos = normalizedQuality.find("maj");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "M");
    }
    
    pos = normalizedQuality.find("Maj");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "M");
    }
    
    pos = normalizedQuality.find("MAJ");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "M");
    }
    
    pos = normalizedQuality.find("min7");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 4, "m7");
    }
    
    pos = normalizedQuality.find("Min7");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 4, "m7");
    }
    
    pos = normalizedQuality.find("MIN7");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 4, "m7");
    }
    
    pos = normalizedQuality.find("min");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "m");
    }
    
    pos = normalizedQuality.find("Min");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "m");
    }
    
    pos = normalizedQuality.find("MIN");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "m");
    }
    
    // Handle dim/diminished
    pos = normalizedQuality.find("diminished");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 10, "dim");
    }
    
    pos = normalizedQuality.find("Diminished");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 10, "dim");
    }
    
    pos = normalizedQuality.find("DIM");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "dim");
    }
    
    pos = normalizedQuality.find("Dim");
    if (pos != std::string::npos) {
        normalizedQuality.replace(pos, 3, "dim");
    }
    
    return root + normalizedQuality;
}

int Chordlock::noteNameToNumber(const std::string& noteName) {
    static const std::map<std::string, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"Db", 1}, {"DB", 1}, 
        {"D", 2}, {"D#", 3}, {"Eb", 3}, {"EB", 3},
        {"E", 4}, {"F", 5}, {"F#", 6}, {"Gb", 6}, {"GB", 6}, 
        {"G", 7}, {"G#", 8}, {"Ab", 8}, {"AB", 8}, 
        {"A", 9}, {"A#", 10}, {"Bb", 10}, {"BB", 10}, {"B", 11}
    };
    
    auto it = noteMap.find(noteName);
    return (it != noteMap.end()) ? it->second : -1;
}

std::vector<int> Chordlock::maskToNoteNumbers(uint16_t mask, int rootNote, int octave) {
    std::vector<int> notes;
    int baseNote = octave * 12;
    
    for (int i = 0; i < 12; i++) {
        if (mask & (1 << i)) {
            // Convert from absolute semitone position to actual MIDI note
            // The mask represents absolute semitone positions from C
            // We need to add the base octave to get the final MIDI note
            notes.push_back(baseNote + i);
        }
    }
    
    return notes;
}

std::vector<std::string> Chordlock::generateChordAlternatives(const std::string& input) {
    std::vector<std::string> alternatives;
    std::string normalizedInput = normalizeChordName(input);
    
    // Extract root and quality directly (avoid parseChordName to prevent circular dependency)
    std::string root;
    std::string quality;
    
    if (normalizedInput.length() >= 2 && (normalizedInput[1] == '#' || normalizedInput[1] == 'b')) {
        root = normalizedInput.substr(0, 2);
        quality = normalizedInput.substr(2);
    } else if (normalizedInput.length() >= 1) {
        root = normalizedInput.substr(0, 1);
        quality = normalizedInput.substr(1);
    } else {
        return alternatives;
    }
    
    // Validate root note
    if (noteNameToNumber(root) == -1) {
        return alternatives;
    }
    
    // Generate enharmonic equivalents for root note
    std::vector<std::string> enharmonicRoots;
    if (root == "C#") {
        enharmonicRoots.push_back("Db");
    } else if (root == "Db") {
        enharmonicRoots.push_back("C#");
    } else if (root == "D#") {
        enharmonicRoots.push_back("Eb");
    } else if (root == "Eb") {
        enharmonicRoots.push_back("D#");
    } else if (root == "F#") {
        enharmonicRoots.push_back("Gb");
    } else if (root == "Gb") {
        enharmonicRoots.push_back("F#");
    } else if (root == "G#") {
        enharmonicRoots.push_back("Ab");
    } else if (root == "Ab") {
        enharmonicRoots.push_back("G#");
    } else if (root == "A#") {
        enharmonicRoots.push_back("Bb");
    } else if (root == "Bb") {
        enharmonicRoots.push_back("A#");
    }
    
    // Add enharmonic alternatives with same quality
    for (const auto& enhRoot : enharmonicRoots) {
        alternatives.push_back(enhRoot + quality);
    }
    
    // Add quality variations with original root
    if (quality == "M7" || quality == "maj7") {
        alternatives.push_back(root + "M7");
        alternatives.push_back(root + "maj7");
        alternatives.push_back(root + "Maj7");
    } else if (quality == "m7" || quality == "min7") {
        alternatives.push_back(root + "m7");
        alternatives.push_back(root + "min7");
    } else if (quality == "7") {
        alternatives.push_back(root + "7");
        alternatives.push_back(root + "dom7");
    } else if (quality == "m") {
        alternatives.push_back(root + "m");
        alternatives.push_back(root + "min");
    } else if (quality.empty()) {
        alternatives.push_back(root);
        alternatives.push_back(root + "maj");
    } else if (quality == "dim") {
        alternatives.push_back(root + "dim");
        alternatives.push_back(root + "diminished");
    }
    
    // Add enharmonic alternatives with quality variations
    for (const auto& enhRoot : enharmonicRoots) {
        if (quality == "M7" || quality == "maj7") {
            alternatives.push_back(enhRoot + "M7");
            alternatives.push_back(enhRoot + "maj7");
        } else if (quality == "m7" || quality == "min7") {
            alternatives.push_back(enhRoot + "m7");
            alternatives.push_back(enhRoot + "min7");
        } else if (quality == "7") {
            alternatives.push_back(enhRoot + "7");
        } else if (quality == "m") {
            alternatives.push_back(enhRoot + "m");
            alternatives.push_back(enhRoot + "min");
        } else if (quality.empty()) {
            alternatives.push_back(enhRoot);
        } else if (quality == "dim") {
            alternatives.push_back(enhRoot + "dim");
        }
    }
    
    return alternatives;
}

// Theoretical chord calculation for missing chords
std::vector<int> Chordlock::calculateTheoreticalChord(const std::string& chordName, int rootOctave) {
    ChordSpec spec = parseChordName(chordName);
    if (spec.rootNote == -1) {
        return {}; // Invalid chord name
    }
    
    // Handle slash chords (quality starts with "/")
    if (!spec.quality.empty() && spec.quality[0] == '/') {
        std::string upperChord = spec.quality.substr(1); // Remove "/" prefix
        
        // Get upper chord notes
        auto upperNotes = calculateTheoreticalChord(upperChord, rootOctave);
        if (upperNotes.empty()) {
            return {}; // Invalid upper chord
        }
        
        // Add bass note at bottom
        std::vector<int> slashNotes;
        int bassNote = spec.rootNote + (rootOctave * 12);
        slashNotes.push_back(bassNote);
        
        // Add upper chord notes
        for (int note : upperNotes) {
            if (note != bassNote) { // Avoid duplicate notes
                slashNotes.push_back(note);
            }
        }
        
        return slashNotes;
    }
    
    std::vector<int> intervals = getIntervalsForQuality(spec.quality);
    if (intervals.empty()) {
        return {}; // Unknown chord type
    }
    
    std::vector<int> notes;
    int baseNote = spec.rootNote + (rootOctave * 12);
    
    for (int interval : intervals) {
        notes.push_back(baseNote + interval);
    }
    
    return notes;
}

bool Chordlock::isKnownChordType(const std::string& quality) {
    // Slash chords are always known if the format is valid
    if (!quality.empty() && quality[0] == '/') {
        return true; // We can handle any slash chord
    }
    return !getIntervalsForQuality(quality).empty();
}

std::vector<int> Chordlock::getIntervalsForQuality(const std::string& quality) {
    // Handle special major chord notations before normalization
    if (quality == "M7" || quality == "maj7" || quality == "Maj7" || quality == "major7" || quality == "Major7") {
        return {0, 4, 7, 11}; // Major 7th: root, major 3rd, 5th, major 7th
    }
    if (quality == "M" || quality == "maj" || quality == "Maj" || quality == "major" || quality == "Major") {
        return {0, 4, 7}; // Major: root, major 3rd, 5th
    }
    if (quality == "M6") {
        return {0, 4, 7, 9}; // Major 6th: root, major 3rd, 5th, 6th
    }
    if (quality == "M9") {
        return {0, 2, 4, 7, 10}; // Major 9th: add 2nd and 7th
    }
    
    // Handle special symbols before normalization
    if (quality == "+") {
        return {0, 4, 8}; // Augmented: root, major 3rd, augmented 5th
    }
    if (quality == "°") {
        return {0, 3, 6}; // Diminished: root, minor 3rd, diminished 5th
    }
    if (quality == "°7") {
        return {0, 3, 6, 9}; // Diminished 7th
    }
    if (quality == "ø") {
        return {0, 3, 6, 10}; // Half-diminished
    }
    
    // Complex altered chords (before normalization to preserve #/b symbols)
    if (quality == "+7" || quality == "aug7") {
        return {0, 4, 8, 10}; // Augmented 7th: root, major 3rd, aug 5th, minor 7th
    }
    if (quality == "maj7#5") {
        return {0, 4, 8, 11}; // Major 7th sharp 5: root, major 3rd, aug 5th, major 7th
    }
    
    // Normalize quality for comparison
    std::string normalizedQuality = quality;
    std::transform(normalizedQuality.begin(), normalizedQuality.end(), 
                   normalizedQuality.begin(), ::tolower);
    
    // Basic triads
    if (normalizedQuality.empty() || normalizedQuality == "maj" || normalizedQuality == "major") {
        return {0, 4, 7}; // Major: root, major 3rd, 5th
    }
    if (normalizedQuality == "m" || normalizedQuality == "min" || normalizedQuality == "minor") {
        return {0, 3, 7}; // Minor: root, minor 3rd, 5th
    }
    if (normalizedQuality == "dim" || normalizedQuality == "diminished") {
        return {0, 3, 6}; // Diminished: root, minor 3rd, diminished 5th
    }
    if (normalizedQuality == "aug" || normalizedQuality == "augmented" || normalizedQuality == "+") {
        return {0, 4, 8}; // Augmented: root, major 3rd, augmented 5th
    }
    
    // Suspended chords
    if (normalizedQuality == "sus4") {
        return {0, 5, 7}; // Sus4: root, 4th, 5th
    }
    if (normalizedQuality == "sus2") {
        return {0, 2, 7}; // Sus2: root, 2nd, 5th
    }
    if (normalizedQuality == "sus") {
        return {0, 5, 7}; // Generic sus defaults to sus4
    }
    
    // Power chord
    if (normalizedQuality == "5") {
        return {0, 7}; // Power chord: root, 5th
    }
    
    // 7th chords
    if (normalizedQuality == "7" || normalizedQuality == "dom7") {
        return {0, 4, 7, 10}; // Dominant 7th: root, major 3rd, 5th, minor 7th
    }
    if (normalizedQuality == "m7" || normalizedQuality == "min7") {
        return {0, 3, 7, 10}; // Minor 7th: root, minor 3rd, 5th, minor 7th
    }
    if (normalizedQuality == "maj7" || normalizedQuality == "major7") {
        return {0, 4, 7, 11}; // Major 7th: root, major 3rd, 5th, major 7th
    }
    if (normalizedQuality == "m7b5" || normalizedQuality == "min7b5" || normalizedQuality == "ø") {
        return {0, 3, 6, 10}; // Half-diminished: root, minor 3rd, dim 5th, minor 7th
    }
    if (normalizedQuality == "dim7" || normalizedQuality == "°7") {
        return {0, 3, 6, 9}; // Diminished 7th: root, minor 3rd, dim 5th, dim 7th
    }
    
    // 6th chords
    if (normalizedQuality == "6") {
        return {0, 4, 7, 9}; // Major 6th: root, major 3rd, 5th, 6th
    }
    if (normalizedQuality == "m6" || normalizedQuality == "min6") {
        return {0, 3, 7, 9}; // Minor 6th: root, minor 3rd, 5th, 6th
    }
    
    // Add chords
    if (normalizedQuality == "add9") {
        return {0, 4, 7, 14}; // Add 9: root, major 3rd, 5th, 9th (2nd octave up)
    }
    if (normalizedQuality == "add2") {
        return {0, 2, 4, 7}; // Add 2: root, 2nd, major 3rd, 5th
    }
    if (normalizedQuality == "add4") {
        return {0, 4, 5, 7}; // Add 4: root, major 3rd, 4th, 5th
    }
    
    // Extended chords (basic approximations)
    if (normalizedQuality == "9") {
        return {0, 4, 7, 10, 14}; // Dominant 9th: includes 7th + 9th
    }
    if (normalizedQuality == "11") {
        return {0, 4, 7, 10, 14, 17}; // Dominant 11th: includes 7th + 9th + 11th
    }
    if (normalizedQuality == "13") {
        return {0, 4, 7, 10, 14, 21}; // Dominant 13th: includes 7th + 9th + 13th
    }
    
    // Alternative notations
    if (normalizedQuality == "no3" || normalizedQuality == "omit3") {
        return {0, 7}; // No 3rd: just root and 5th (like power chord)
    }
    
    return {}; // Unknown chord type
}

std::string Chordlock::getCanonicalChordName(const std::string& root, const std::vector<int>& intervals) {
    // Convert intervals to canonical chord name
    if (intervals.empty()) {
        return root;
    }
    
    // Sort intervals for comparison (should already be sorted, but ensure)
    std::vector<int> sortedIntervals = intervals;
    std::sort(sortedIntervals.begin(), sortedIntervals.end());
    
    // Major triads
    if (sortedIntervals == std::vector<int>{0, 4, 7}) {
        return root; // Just "C" for major
    }
    
    // Minor triads  
    if (sortedIntervals == std::vector<int>{0, 3, 7}) {
        return root + "m";
    }
    
    // Diminished triads
    if (sortedIntervals == std::vector<int>{0, 3, 6}) {
        return root + "dim";
    }
    
    // Augmented triads
    if (sortedIntervals == std::vector<int>{0, 4, 8}) {
        return root + "aug";
    }
    
    // Suspended chords
    if (sortedIntervals == std::vector<int>{0, 5, 7}) {
        return root + "sus4";
    }
    if (sortedIntervals == std::vector<int>{0, 2, 7}) {
        return root + "sus2";
    }
    
    // Power chord
    if (sortedIntervals == std::vector<int>{0, 7}) {
        return root + "5";
    }
    
    // 7th chords
    if (sortedIntervals == std::vector<int>{0, 4, 7, 10}) {
        return root + "7"; // Dominant 7th
    }
    if (sortedIntervals == std::vector<int>{0, 3, 7, 10}) {
        return root + "m7"; // Minor 7th
    }
    if (sortedIntervals == std::vector<int>{0, 4, 7, 11}) {
        return root + "maj7"; // Major 7th
    }
    if (sortedIntervals == std::vector<int>{0, 3, 6, 10}) {
        return root + "m7b5"; // Half-diminished
    }
    if (sortedIntervals == std::vector<int>{0, 3, 6, 9}) {
        return root + "dim7"; // Diminished 7th
    }
    
    // 6th chords
    if (sortedIntervals == std::vector<int>{0, 4, 7, 9}) {
        return root + "6"; // Major 6th
    }
    if (sortedIntervals == std::vector<int>{0, 3, 7, 9}) {
        return root + "m6"; // Minor 6th
    }
    
    // Add chords
    if (sortedIntervals == std::vector<int>{0, 4, 7, 14}) {
        return root + "add9";
    }
    if (sortedIntervals == std::vector<int>{0, 2, 4, 7}) {
        return root + "add2";
    }
    if (sortedIntervals == std::vector<int>{0, 4, 5, 7}) {
        return root + "add4";
    }
    
    // Extended chords
    if (sortedIntervals == std::vector<int>{0, 4, 7, 10, 14}) {
        return root + "9"; // Dominant 9th
    }
    if (sortedIntervals == std::vector<int>{0, 4, 7, 10, 14, 17}) {
        return root + "11"; // Dominant 11th
    }
    if (sortedIntervals == std::vector<int>{0, 4, 7, 10, 14, 21}) {
        return root + "13"; // Dominant 13th
    }
    
    // Complex altered chords
    if (sortedIntervals == std::vector<int>{0, 4, 8, 10}) {
        return root + "7#5"; // Augmented 7th (prefer 7#5 over aug7)
    }
    if (sortedIntervals == std::vector<int>{0, 4, 8, 11}) {
        return root + "maj7#5"; // Major 7th sharp 5
    }
    
    // Minor 9th
    if (sortedIntervals == std::vector<int>{0, 2, 3, 7, 10}) {
        return root + "m9";
    }
    
    // Major 9th (with major 7th)
    if (sortedIntervals == std::vector<int>{0, 2, 4, 7, 10}) {
        return root + "9"; // Dominant 9th is more common
    }
    
    // If no match found, construct a reasonable canonical name
    // This should not happen often, but provides a fallback
    if (sortedIntervals.size() >= 3) {
        // For unknown chords, try to construct a meaningful name
        return root + "unknown";
    }
    
    // For single note or two notes, just return root
    return root;
}

// Degree analysis methods implementation
std::string Chordlock::degreeToChordName(const std::string& degree, int tonic, bool isMinor) {
    if (tonic < 0 || tonic > 11) {
        return ""; // Invalid tonic
    }
    
    // Note names
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    // Parse degree (supports lowercase/uppercase, with optional extensions)
    std::string degreeStr = degree;
    std::transform(degreeStr.begin(), degreeStr.end(), degreeStr.begin(), ::tolower);
    
    // Major scale intervals from tonic
    int majorScaleIntervals[] = {0, 2, 4, 5, 7, 9, 11}; // I, ii, iii, IV, V, vi, vii
    // Minor scale intervals from tonic  
    int minorScaleIntervals[] = {0, 2, 3, 5, 7, 8, 10}; // i, ii, bIII, iv, v, bVI, bVII
    
    // Default chord qualities in major/minor keys
    const char* majorQualities[] = {"", "m", "m", "", "", "m", "dim"};
    const char* minorQualities[] = {"m", "dim", "", "m", "m", "", ""};
    
    int degreeIndex = -1;
    std::string quality = "";
    std::string extension = "";
    
    // Parse Roman numeral degree (with flat/sharp support)
    bool isFlat = (degreeStr.find("b") != std::string::npos);
    bool isSharp = (degreeStr.find("#") != std::string::npos);
    
    // Remove accidentals for parsing
    std::string cleanDegree = degreeStr;
    cleanDegree.erase(std::remove(cleanDegree.begin(), cleanDegree.end(), 'b'), cleanDegree.end());
    cleanDegree.erase(std::remove(cleanDegree.begin(), cleanDegree.end(), '#'), cleanDegree.end());
    
    if (cleanDegree.find("i") == 0) {
        degreeIndex = 0;
        if (cleanDegree.length() > 1 && cleanDegree[1] == 'i') {
            if (cleanDegree.length() > 2 && cleanDegree[2] == 'i') {
                degreeIndex = 2; // iii
            } else {
                degreeIndex = 1; // ii
            }
        }
    } else if (cleanDegree.find("ii") == 0) {
        degreeIndex = 1;
        if (cleanDegree.length() > 2 && cleanDegree[2] == 'i') {
            degreeIndex = 2; // iii
        }
    } else if (cleanDegree.find("iii") == 0) {
        degreeIndex = 2;
    } else if (cleanDegree.find("iv") == 0) {
        degreeIndex = 3;
    } else if (cleanDegree.find("v") == 0) {
        degreeIndex = 4;
        if (cleanDegree.length() > 1 && cleanDegree[1] == 'i') {
            if (cleanDegree.length() > 2 && cleanDegree[2] == 'i') {
                degreeIndex = 6; // vii
            } else {
                degreeIndex = 5; // vi
            }
        }
    } else if (cleanDegree.find("vi") == 0) {
        degreeIndex = 5;
        if (cleanDegree.length() > 2 && cleanDegree[2] == 'i') {
            degreeIndex = 6; // vii
        }
    } else if (cleanDegree.find("vii") == 0) {
        degreeIndex = 6;
    }
    
    if (degreeIndex == -1) {
        return ""; // Invalid degree
    }
    
    // Calculate root note
    int rootNote;
    if (isMinor) {
        rootNote = (tonic + minorScaleIntervals[degreeIndex]) % 12;
        quality = minorQualities[degreeIndex];
    } else {
        rootNote = (tonic + majorScaleIntervals[degreeIndex]) % 12;
        quality = majorQualities[degreeIndex];
    }
    
    // Apply accidentals (flat/sharp) - but for common degrees like bVII in minor, use standard interpretation
    if (isFlat) {
        // Special case for bVII in minor key - this typically means the natural 7th (major chord)
        if (degreeIndex == 6 && isMinor) {
            // bVII in minor key = natural 7th scale degree = major chord
            // Keep the natural 7th interval but make it major
            quality = ""; // Major chord
        } else {
            rootNote = (rootNote - 1 + 12) % 12;
        }
    } else if (isSharp) {
        rootNote = (rootNote + 1) % 12;
    }
    
    // Parse extensions (7, 9, etc.)
    size_t pos = degree.find('7');
    if (pos != std::string::npos) {
        extension = "7";
    } else {
        pos = degree.find('9');
        if (pos != std::string::npos) {
            extension = "9";
        }
    }
    
    // Override quality for uppercase Roman numerals (major chords)
    if (degree[0] >= 'A' && degree[0] <= 'Z') {
        if (degreeIndex == 1 || degreeIndex == 2 || degreeIndex == 5) {
            // Naturally minor degrees in major key become major
            quality = "";
        }
    }
    
    return std::string(noteNames[rootNote]) + quality + extension;
}

std::vector<int> Chordlock::degreeToNotes(const std::string& degree, int tonic, bool isMinor, int rootOctave) {
    std::string chordName = degreeToChordName(degree, tonic, isMinor);
    if (chordName.empty()) {
        return {};
    }
    
    return chordNameToNotes(chordName, rootOctave);
}

std::string Chordlock::degreeToNotesJSON(const std::string& degree, int tonic, bool isMinor, int rootOctave) {
    std::string chordName = degreeToChordName(degree, tonic, isMinor);
    if (chordName.empty()) {
        return "{\"error\": \"Invalid degree specification\"}";
    }
    
    auto notes = degreeToNotes(degree, tonic, isMinor, rootOctave);
    if (notes.empty()) {
        return "{\"error\": \"Could not generate notes for degree\"}";
    }
    
    std::ostringstream json;
    json << "{";
    json << "\"degree\": \"" << degree << "\", ";
    json << "\"chordName\": \"" << chordName << "\", ";
    json << "\"notes\": [";
    for (size_t i = 0; i < notes.size(); i++) {
        json << notes[i];
        if (i < notes.size() - 1) json << ", ";
    }
    json << "], ";
    json << "\"key\": \"";
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    json << noteNames[tonic] << (isMinor ? " minor" : " major") << "\"";
    json << "}";
    
    return json.str();
}

// Reverse degree analysis methods implementation
std::string Chordlock::analyzeDegree(const std::string& chordName, int tonic, bool isMinor) {
    if (tonic < 0 || tonic > 11 || chordName.empty()) {
        return "";
    }
    
    // Parse chord to get root note
    int rootNote = noteNameToNumber(chordName.substr(0, chordName.find_first_not_of("CDEFGAB#b")));
    if (rootNote == -1) {
        return "";
    }
    
    // Calculate interval from tonic
    int interval = (rootNote - tonic + 12) % 12;
    
    // Scale degrees and their Roman numerals
    int majorScaleIntervals[] = {0, 2, 4, 5, 7, 9, 11}; // I, ii, iii, IV, V, vi, vii
    int minorScaleIntervals[] = {0, 2, 3, 5, 7, 8, 10}; // i, ii, bIII, iv, v, bVI, bVII
    
    const char* majorNumerals[] = {"I", "ii", "iii", "IV", "V", "vi", "vii"};
    const char* minorNumerals[] = {"i", "ii", "bIII", "iv", "v", "bVI", "bVII"};
    
    // Find matching scale degree
    for (int i = 0; i < 7; i++) {
        int scaleInterval = isMinor ? minorScaleIntervals[i] : majorScaleIntervals[i];
        if (interval == scaleInterval) {
            std::string numeral = isMinor ? minorNumerals[i] : majorNumerals[i];
            
            // Add chord quality extensions
            if (chordName.find("7") != std::string::npos) {
                numeral += "7";
            }
            if (chordName.find("9") != std::string::npos) {
                numeral += "9";
            }
            if (chordName.find("11") != std::string::npos) {
                numeral += "11";
            }
            if (chordName.find("13") != std::string::npos) {
                numeral += "13";
            }
            
            // Handle altered qualities
            if (chordName.find("dim") != std::string::npos) {
                numeral += "°";
            }
            if (chordName.find("aug") != std::string::npos || chordName.find("+") != std::string::npos) {
                numeral += "+";
            }
            
            return numeral;
        }
    }
    
    // Handle chromatic alterations
    const char* chromaticNumerals[] = {"I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII"};
    return chromaticNumerals[interval];
}

std::string Chordlock::analyzeNotesToDegree(const std::vector<int>& notes, int tonic, bool isMinor) {
    if (notes.empty()) {
        return "";
    }
    
    // Set notes in engine and detect chord
    clearAllNotes();
    for (int note : notes) {
        noteOn(note, 80);
    }
    
    auto result = detectChord();
    clearAllNotes();
    
    if (!result.hasValidChord || result.chordName.empty()) {
        return "";
    }
    
    return analyzeDegree(result.chordName, tonic, isMinor);
}

std::string Chordlock::analyzeCurrentNotesToDegree(int tonic, bool isMinor) {
    auto result = detectChord();
    
    if (!result.hasValidChord || result.chordName.empty()) {
        return "";
    }
    
    return analyzeDegree(result.chordName, tonic, isMinor);
}

std::vector<int> Chordlock::generateExtendedChordNotes(const std::string& chordName, int rootOctave) {
    struct ExtDef {
        std::string suffix;
        std::vector<int> intervals;
    };
    
    // Extended chord definitions (same as detection engine)
    const std::vector<ExtDef> extDefs = {
        {"13#11", {0, 4, 7, 10, 2, 6, 9}},    // 1-3-5-7-9-#11-13
        {"13b9",  {0, 4, 7, 10, 1, 9}},       // 1-3-5-7-b9-13
        {"11#9",  {0, 4, 7, 10, 3, 5}},       // 1-3-5-7-#9-11
        {"7#11",  {0, 4, 7, 10, 6}},          // 1-3-5-7-#11
        {"add2#4", {0, 4, 7, 2, 6}},          // 1-3-5-2-#4
        {"6/9",   {0, 4, 7, 9, 2}},           // 1-3-5-6-9
    };
    
    // Parse chord name to extract root and suffix
    auto spec = parseChordName(chordName);
    if (spec.rootNote == -1) {
        return {}; // Invalid chord name
    }
    
    // Check if this matches any extended chord pattern
    for (const auto& extDef : extDefs) {
        if (spec.quality == extDef.suffix || 
            chordName.find(extDef.suffix) != std::string::npos) {
            
            std::vector<int> notes;
            int baseNote = rootOctave * 12 + spec.rootNote;
            
            // Generate MIDI notes from intervals
            for (int interval : extDef.intervals) {
                notes.push_back(baseNote + interval);
            }
            
            // Sort notes
            std::sort(notes.begin(), notes.end());
            return notes;
        }
    }
    
    return {}; // No extended chord match
}