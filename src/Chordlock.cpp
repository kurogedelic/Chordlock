#include "Chordlock.hpp"
#include <algorithm>
#include <sstream>
#include <cstring>

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