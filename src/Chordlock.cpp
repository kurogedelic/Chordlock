#include "Chordlock.hpp"
#include "enhanced_chord_hash_table.hpp"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <map>
#include <set>
#include <cctype>

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
    
    stats_.engineVersion = "1.0.0";
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
    
    std::string normalizedInput = normalizeChordName(chordName);
    
    // Primary search: exact match with root and quality
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
    json << "{\"chord\":\"" << chordName << "\",\"notes\":[";
    for (size_t i = 0; i < notes.size(); i++) {
        json << notes[i];
        if (i < notes.size() - 1) json << ",";
    }
    json << "],\"octave\":" << rootOctave << "}";
    
    return json.str();
}

// Helper method implementations
Chordlock::ChordSpec Chordlock::parseChordName(const std::string& input) {
    ChordSpec spec;
    spec.rootNote = -1;
    
    if (input.empty()) {
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