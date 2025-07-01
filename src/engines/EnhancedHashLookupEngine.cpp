#include "EnhancedHashLookupEngine.hpp"
#include <cstring>
#include <cstdlib>
#include <iostream>

EnhancedHashLookupEngine::EnhancedHashLookupEngine() {
    reset();
}

ChordCandidate EnhancedHashLookupEngine::detectBest(uint32_t) {
    auto weights = velocityProcessor_.processNotes(noteStates_, velocities_);
    auto result = lookupChordEnhanced(weights);
    
    if (result.hasResults()) {
        return result.getBest();
    }
    
    return {"", 0, 0.0f};
}

std::vector<ChordCandidate> EnhancedHashLookupEngine::detectAlternatives(int maxResults, uint32_t) {
    auto weights = velocityProcessor_.processNotes(noteStates_, velocities_);
    auto result = lookupChordEnhanced(weights);
    
    std::vector<ChordCandidate> alternatives;
    int count = std::min(maxResults, static_cast<int>(result.candidates.size()));
    
    for (int i = 0; i < count; i++) {
        alternatives.push_back(result.candidates[i]);
    }
    
    return alternatives;
}

void EnhancedHashLookupEngine::noteOn(uint8_t note, uint32_t) {
    if (note < 128) {
        noteStates_[note] = true;
    }
}

void EnhancedHashLookupEngine::noteOff(uint8_t note) {
    if (note < 128) {
        noteStates_[note] = false;
    }
}

void EnhancedHashLookupEngine::setVelocity(uint8_t note, uint8_t velocity) {
    if (note < 128) {
        velocities_[note] = velocity;
    }
}

void EnhancedHashLookupEngine::reset() {
    for (int i = 0; i < 128; i++) {
        noteStates_[i] = false;
        velocities_[i] = 0;
    }
}

void EnhancedHashLookupEngine::setVelocitySensitivity(bool enabled) {
    velocitySensitive_ = enabled;
}

void EnhancedHashLookupEngine::setSlashChordDetection(bool enabled) {
    slashChordDetection_ = enabled;
}

void EnhancedHashLookupEngine::setKey(int key) {
    currentKey_ = key;
}

bool EnhancedHashLookupEngine::getVelocitySensitivity() const {
    return velocitySensitive_;
}

bool EnhancedHashLookupEngine::getSlashChordDetection() const {
    return slashChordDetection_;
}

int EnhancedHashLookupEngine::getKey() const {
    return currentKey_;
}

std::string EnhancedHashLookupEngine::getEngineName() const {
    return "EnhancedHashLookupEngine";
}

std::string EnhancedHashLookupEngine::getEngineVersion() const {
    return "2.0.0-enhanced";
}

EnhancedHashLookupEngine::EnhancedLookupResult 
EnhancedHashLookupEngine::lookupChordEnhanced(const VelocityProcessor::VelocityWeights& weights) const {
    EnhancedLookupResult result;
    
    uint16_t mask = calculateMask(weights);
    
    // Direct lookup first - these should be highly prioritized
    const EnhancedChordEntry* entry = findEnhancedChord(mask);
    if (entry) {
        float confidence = entry->confidence;
        
        // Apply confidence boost based on velocity analysis
        confidence = calculateConfidenceBoost(entry, weights);
        
        // Strong boost for exact matches
        confidence *= 1.5f;
        
        result.candidates.push_back({
            entry->name,
            mask,
            confidence
        });
    }
    
    // Find alternative chords (subset masks, transpositions, etc.)
    auto alternatives = findAlternativeChords(mask, 5);
    for (const auto& alt : alternatives) {
        // Avoid duplicates
        bool isDuplicate = false;
        for (const auto& existing : result.candidates) {
            if (existing.name == alt.name) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate) {
            result.candidates.push_back(alt);
        }
    }
    
    // Sort by confidence
    std::sort(result.candidates.begin(), result.candidates.end(),
              [](const ChordCandidate& a, const ChordCandidate& b) {
                  return a.confidence > b.confidence;
              });
    
    // Calculate average confidence
    if (!result.candidates.empty()) {
        float totalConfidence = 0.0f;
        for (const auto& candidate : result.candidates) {
            totalConfidence += candidate.confidence;
        }
        result.averageConfidence = totalConfidence / result.candidates.size();
    } else {
        result.averageConfidence = 0.0f;
    }
    
    return result;
}

EnhancedHashLookupEngine::EnhancedLookupResult 
EnhancedHashLookupEngine::lookupChordDirect(uint16_t mask) const {
    EnhancedLookupResult result;
    
    const EnhancedChordEntry* entry = findEnhancedChord(mask);
    if (entry) {
        result.candidates.push_back({
            entry->name,
            mask,
            entry->confidence
        });
        result.averageConfidence = entry->confidence;
    }
    
    return result;
}

void EnhancedHashLookupEngine::setChordFromMIDI(const std::vector<int>& midiNotes, uint8_t baseVelocity) {
    reset();
    
    for (int note : midiNotes) {
        if (note >= 0 && note < 128) {
            noteStates_[note] = true;
            velocities_[note] = baseVelocity + (rand() % 21) - 10; // Add slight variation
        }
    }
}

uint16_t EnhancedHashLookupEngine::getCurrentMask() const {
    return calculateDirectMask();
}

uint16_t EnhancedHashLookupEngine::calculateMask(const VelocityProcessor::VelocityWeights& weights) const {
    uint16_t mask = 0;
    
    if (velocitySensitive_) {
        // Use velocity-weighted mask
        for (int pitch = 0; pitch < 12; pitch++) {
            if (weights.weights[pitch] > 0.1f) {
                mask |= (1 << pitch);
            }
        }
    } else {
        // Use direct mask
        mask = calculateDirectMask();
    }
    
    return mask;
}

uint16_t EnhancedHashLookupEngine::calculateDirectMask() const {
    uint16_t mask = 0;
    
    for (int note = 0; note < 128; note++) {
        if (noteStates_[note]) {
            int pitch = note % 12;
            mask |= (1 << pitch);
        }
    }
    
    return mask;
}

std::vector<ChordCandidate> EnhancedHashLookupEngine::findAlternativeChords(uint16_t mask, int maxResults) const {
    std::vector<ChordCandidate> alternatives;
    
    // Strategy 1: Find subset chords (chords with fewer notes)
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE && alternatives.size() < maxResults; i++) {
        const auto& entry = ENHANCED_CHORD_TABLE[i];
        
        // Check if this chord's mask is a subset of the input mask
        if ((entry.mask & mask) == entry.mask && entry.mask != mask) {
            // Count notes in input vs chord
            int inputNotes = __builtin_popcount(mask);
            int chordNotes = __builtin_popcount(entry.mask);
            int extraNotes = inputNotes - chordNotes;
            
            // Heavy penalty for subset matches - they should be less likely than complete matches
            float confidence = entry.confidence * (0.5f - (extraNotes * 0.1f));
            
            alternatives.push_back({
                entry.name,
                entry.mask,
                confidence
            });
        }
    }
    
    // Strategy 2: Find chords with 1-2 additional notes (superset)
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE && alternatives.size() < maxResults; i++) {
        const auto& entry = ENHANCED_CHORD_TABLE[i];
        
        // Check if input mask is a subset of this chord's mask
        if ((mask & entry.mask) == mask && entry.mask != mask) {
            // Count additional notes
            uint16_t diff = entry.mask ^ mask;
            int additionalNotes = __builtin_popcount(diff);
            
            if (additionalNotes <= 2) { // Only consider chords with 1-2 additional notes
                float confidence = entry.confidence * (additionalNotes == 1 ? 0.6f : 0.4f);
                
                alternatives.push_back({
                    entry.name,
                    entry.mask,
                    confidence
                });
            }
        }
    }
    
    // Strategy 3: Find transpositions (same interval pattern, different root)
    int noteCount = __builtin_popcount(mask);
    if (noteCount >= 3 && alternatives.size() < maxResults) {
        for (int transpose = 1; transpose < 12; transpose++) {
            uint16_t transposedMask = 0;
            
            for (int pitch = 0; pitch < 12; pitch++) {
                if (mask & (1 << pitch)) {
                    int newPitch = (pitch + transpose) % 12;
                    transposedMask |= (1 << newPitch);
                }
            }
            
            const EnhancedChordEntry* entry = findEnhancedChord(transposedMask);
            if (entry) {
                float confidence = entry->confidence * 0.5f; // Lower confidence for transpositions
                
                alternatives.push_back({
                    entry->name,
                    entry->mask,
                    confidence
                });
                
                if (alternatives.size() >= maxResults) break;
            }
        }
    }
    
    return alternatives;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> 
EnhancedHashLookupEngine::findDetailedAlternatives(uint16_t mask, int maxResults) const {
    std::vector<DetailedChordCandidate> detailedCandidates;
    
    int noteCount = __builtin_popcount(mask);
    
    // Handle single note case - return note name only
    if (noteCount == 1) {
        int lowestNote = findLowestNote();
        if (lowestNote >= 0) {
            DetailedChordCandidate candidate;
            candidate.name = formatNoteName(lowestNote);
            candidate.mask = mask;
            candidate.confidence = 1.0f;
            candidate.root = getNoteFromPitch(lowestNote % 12);
            candidate.isInversion = false;
            candidate.inversionDegree = 0;
            candidate.interpretationType = "single_note";
            candidate.matchScore = 1.0f;
            
            detailedCandidates.push_back(candidate);
            return detailedCandidates;
        }
    }
    
    // Strategy 1: Exact matches (PRIORITIZED for theoretical chords like sus2, sus4, etc.)
    const EnhancedChordEntry* exactEntry = findEnhancedChord(mask);
    if (exactEntry) {
        DetailedChordCandidate candidate;
        candidate.name = exactEntry->name;
        candidate.mask = exactEntry->mask;
        candidate.confidence = exactEntry->confidence;
        candidate.root = exactEntry->root;
        candidate.isInversion = false;
        candidate.inversionDegree = 0;
        candidate.interpretationType = "exact";
        candidate.matchScore = 1.0f;
        
        // Apply strong boost for exact matches (should be highest priority)
        candidate.confidence *= 1.5f; // Strong boost for exact matches
        
        // Additional boost for theoretical chords (sus2, sus4, add9, etc.)
        std::string chordName = exactEntry->name;
        if (chordName.find("sus2") != std::string::npos ||
            chordName.find("sus4") != std::string::npos ||
            chordName.find("add") != std::string::npos ||
            chordName.find("dim") != std::string::npos ||
            chordName.find("aug") != std::string::npos) {
            candidate.confidence *= 1.3f; // Additional boost for theoretical chords
        }
        
        detailedCandidates.push_back(candidate);
    }
    
    // Strategy 2: Bass-aware slash chord analysis
    if (noteCount >= 3) {
        int bassNote = findLowestNote();
        if (bassNote >= 0) {
            int bassPitch = bassNote % 12;
            uint16_t upperMask = removeLowestNoteFromMask(mask, bassNote);
            
            if (__builtin_popcount(upperMask) >= 2) {
                // Try to find chord in upper voices
                const EnhancedChordEntry* upperChord = findEnhancedChord(upperMask);
                if (upperChord) {
                    int upperRootPitch = -1;
                    // Try to determine the root of upper chord
                    for (int i = 0; i < 12; i++) {
                        if (upperMask & (1 << i)) {
                            upperRootPitch = i;
                            break;
                        }
                    }
                    
                    if (upperRootPitch >= 0 && bassPitch != upperRootPitch) {
                        // Create slash chord candidate
                        auto slashCandidate = createSlashChordCandidate(upperChord, bassNote, upperChord->confidence);
                        
                        // Reduce slash chord confidence when exact match exists
                        if (exactEntry) {
                            slashCandidate.confidence *= 0.6f; // Strong demotion for slash chords when exact match available
                        }
                        
                        // Boost confidence for slash chords when bass is in much lower register
                        int bassRegister = bassNote / 12;
                        int upperRegister = findHighestNote() / 12;
                        
                        if (upperRegister - bassRegister >= 1) {
                            // Clear register separation - boost slash chord confidence
                            slashCandidate.confidence *= 1.1f; // Reduced from 1.2f
                        }
                        
                        // Boost confidence if upper voices form a complete triad
                        int upperNoteCount = __builtin_popcount(upperMask);
                        if (upperNoteCount >= 3) {
                            slashCandidate.confidence *= 1.05f; // Reduced from 1.1f
                        }
                        
                        detailedCandidates.push_back(slashCandidate);
                    }
                }
            }
        }
    }
    
    // Strategy 3: Inversion detection
    auto inversionCandidates = findEnharmonicEquivalents(mask);
    for (const auto& candidate : inversionCandidates) {
        if (detailedCandidates.size() >= maxResults) break;
        detailedCandidates.push_back(candidate);
    }
    
    // Strategy 4: Subset matches (simplified chords)
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE && detailedCandidates.size() < maxResults; i++) {
        const auto& entry = ENHANCED_CHORD_TABLE[i];
        
        if ((entry.mask & mask) == entry.mask && entry.mask != mask) {
            DetailedChordCandidate candidate;
            candidate.name = entry.name;
            candidate.mask = entry.mask;
            candidate.confidence = entry.confidence * 0.7f;
            candidate.root = entry.root;
            candidate.isInversion = false;
            candidate.inversionDegree = 0;
            candidate.interpretationType = "subset";
            candidate.matchScore = calculateMatchScore(mask, entry.mask);
            candidate.extraNotes = findExtraNotes(mask, entry.mask);
            
            detailedCandidates.push_back(candidate);
        }
    }
    
    // Strategy 5: Superset matches (extended chords)
    for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE && detailedCandidates.size() < maxResults; i++) {
        const auto& entry = ENHANCED_CHORD_TABLE[i];
        
        if ((mask & entry.mask) == mask && entry.mask != mask) {
            uint16_t diff = entry.mask ^ mask;
            int missingNotes = __builtin_popcount(diff);
            
            if (missingNotes <= 2) {
                DetailedChordCandidate candidate;
                candidate.name = entry.name;
                candidate.mask = entry.mask;
                candidate.confidence = entry.confidence * (missingNotes == 1 ? 0.6f : 0.4f);
                candidate.root = entry.root;
                candidate.isInversion = false;
                candidate.inversionDegree = 0;
                candidate.interpretationType = "superset";
                candidate.matchScore = calculateMatchScore(mask, entry.mask);
                candidate.missingNotes = findMissingNotes(mask, entry.mask);
                
                detailedCandidates.push_back(candidate);
            }
        }
    }
    
    // Sort by confidence
    std::sort(detailedCandidates.begin(), detailedCandidates.end(),
              [](const DetailedChordCandidate& a, const DetailedChordCandidate& b) {
                  return a.confidence > b.confidence;
              });
    
    return detailedCandidates;
}

EnhancedHashLookupEngine::EnhancedLookupResult 
EnhancedHashLookupEngine::lookupChordWithDetailedAnalysis(uint16_t mask, int maxResults) const {
    EnhancedLookupResult result;
    
    // Get detailed candidates
    result.detailedCandidates = findDetailedAlternatives(mask, maxResults);
    
    // Convert to simple candidates for backward compatibility
    for (const auto& detailed : result.detailedCandidates) {
        result.candidates.push_back({
            detailed.name,
            detailed.mask,
            detailed.confidence
        });
    }
    
    // Calculate average confidence
    if (!result.candidates.empty()) {
        float totalConfidence = 0.0f;
        for (const auto& candidate : result.candidates) {
            totalConfidence += candidate.confidence;
        }
        result.averageConfidence = totalConfidence / result.candidates.size();
    }
    
    return result;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> 
EnhancedHashLookupEngine::findEnharmonicEquivalents(uint16_t mask) const {
    std::vector<DetailedChordCandidate> equivalents;
    
    // Try all 12 possible roots for the same interval pattern
    for (int root = 0; root < 12; root++) {
        // Check if this root makes sense as a bass note
        if (!(mask & (1 << root))) continue;
        
        // Search for chords that could be inversions of this pattern
        for (size_t i = 0; i < ENHANCED_CHORD_TABLE_SIZE; i++) {
            const auto& entry = ENHANCED_CHORD_TABLE[i];
            
            // Check all possible inversions of this chord
            for (int inversion = 0; inversion < 4; inversion++) {
                uint16_t invertedMask = 0;
                
                // Generate inversion mask (simplified approach)
                for (int pitch = 0; pitch < 12; pitch++) {
                    if (entry.mask & (1 << pitch)) {
                        int newPitch = (pitch - root + 12) % 12;
                        invertedMask |= (1 << newPitch);
                    }
                }
                
                if (invertedMask == mask) {
                    DetailedChordCandidate candidate;
                    candidate.name = std::string(entry.name) + "/" + getNoteFromPitch(root);
                    candidate.mask = mask;
                    candidate.confidence = entry.confidence * 0.8f; // Slight penalty for inversions
                    candidate.root = getNoteFromPitch(root);
                    candidate.isInversion = (inversion > 0);
                    candidate.inversionDegree = inversion;
                    candidate.interpretationType = "inversion";
                    candidate.matchScore = 1.0f;
                    
                    equivalents.push_back(candidate);
                    break;
                }
            }
        }
    }
    
    return equivalents;
}

float EnhancedHashLookupEngine::calculateMatchScore(uint16_t inputMask, uint16_t chordMask) const {
    int inputNotes = __builtin_popcount(inputMask);
    int chordNotes = __builtin_popcount(chordMask);
    int commonNotes = __builtin_popcount(inputMask & chordMask);
    
    if (inputNotes == 0) return 0.0f;
    
    // Calculate match percentage based on input notes
    return static_cast<float>(commonNotes) / static_cast<float>(inputNotes);
}

int EnhancedHashLookupEngine::detectInversionDegree(uint16_t mask, const std::string& chordName) const {
    // Simplified inversion detection
    // Find the lowest note and determine its role in the chord
    for (int pitch = 0; pitch < 12; pitch++) {
        if (mask & (1 << pitch)) {
            // This would need more sophisticated chord theory analysis
            // For now, return 0 (root position)
            return 0;
        }
    }
    return 0;
}

std::vector<int> EnhancedHashLookupEngine::findMissingNotes(uint16_t inputMask, uint16_t expectedMask) const {
    std::vector<int> missing;
    uint16_t missingMask = expectedMask & (~inputMask);
    
    for (int pitch = 0; pitch < 12; pitch++) {
        if (missingMask & (1 << pitch)) {
            missing.push_back(pitch);
        }
    }
    
    return missing;
}

std::vector<int> EnhancedHashLookupEngine::findExtraNotes(uint16_t inputMask, uint16_t expectedMask) const {
    std::vector<int> extra;
    uint16_t extraMask = inputMask & (~expectedMask);
    
    for (int pitch = 0; pitch < 12; pitch++) {
        if (extraMask & (1 << pitch)) {
            extra.push_back(pitch);
        }
    }
    
    return extra;
}

std::string EnhancedHashLookupEngine::getNoteFromPitch(int pitch) const {
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return noteNames[pitch % 12];
}

float EnhancedHashLookupEngine::calculateConfidenceBoost(
    const EnhancedChordEntry* entry, 
    const VelocityProcessor::VelocityWeights& weights) const {
    
    float confidence = entry->confidence;
    
    if (!velocitySensitive_) {
        return confidence;
    }
    
    // Use available weight totals
    float harmonyTotal = weights.totalHarmonicWeight;
    float melodyTotal = weights.totalMelodicWeight;
    
    // Boost confidence if chord notes have strong harmonic content
    float harmonyBoost = 1.0f;
    float chordNoteHarmony = 0.0f;
    int chordNoteCount = 0;
    
    for (int pitch = 0; pitch < 12; pitch++) {
        if (entry->mask & (1 << pitch)) {
            // Use harmonic mask to determine if this pitch is harmonic
            if (weights.harmonicMask & (1 << pitch)) {
                chordNoteHarmony += weights.weights[pitch];
            }
            chordNoteCount++;
        }
    }
    
    if (chordNoteCount > 0 && harmonyTotal > 0.0f) {
        float harmonyRatio = chordNoteHarmony / harmonyTotal;
        harmonyBoost = 1.0f + (harmonyRatio - 0.5f); // Boost if >50% of harmony is in chord notes
        harmonyBoost = std::max(0.5f, std::min(1.5f, harmonyBoost)); // Clamp to reasonable range
    }
    
    return std::min(0.95f, confidence * harmonyBoost);
}

// Enhanced bass-aware analysis methods
int EnhancedHashLookupEngine::findLowestNote() const {
    for (int note = 0; note < 128; note++) {
        if (noteStates_[note]) {
            return note;
        }
    }
    return -1; // No notes found
}

int EnhancedHashLookupEngine::findHighestNote() const {
    for (int note = 127; note >= 0; note--) {
        if (noteStates_[note]) {
            return note;
        }
    }
    return -1; // No notes found
}

std::string EnhancedHashLookupEngine::formatNoteName(int midiNote) const {
    if (midiNote < 0 || midiNote > 127) {
        return "Unknown";
    }
    
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int pitch = midiNote % 12;
    
    return std::string(noteNames[pitch]) + std::to_string(octave);
}

uint16_t EnhancedHashLookupEngine::removeLowestNoteFromMask(uint16_t mask, int lowestNote) const {
    if (lowestNote < 0) return mask;
    
    int lowestPitch = lowestNote % 12;
    uint16_t resultMask = mask;
    
    // Remove the lowest pitch class from the mask
    resultMask &= ~(1 << lowestPitch);
    
    return resultMask;
}

EnhancedHashLookupEngine::DetailedChordCandidate 
EnhancedHashLookupEngine::createSlashChordCandidate(const EnhancedChordEntry* upperChord, int bassNote, float baseConfidence) const {
    DetailedChordCandidate candidate;
    
    std::string bassNoteName = getNoteFromPitch(bassNote % 12);
    candidate.name = std::string(upperChord->name) + "/" + bassNoteName;
    candidate.mask = calculateDirectMask(); // Full mask including bass
    
    // Start with slightly higher confidence for slash chords - they're often more natural
    candidate.confidence = baseConfidence * 0.95f; 
    
    candidate.root = bassNoteName; // Bass note becomes the root for slash chords
    candidate.isInversion = false; // Slash chords are not inversions
    candidate.inversionDegree = 0;
    candidate.interpretationType = "slash_chord";
    
    // Calculate match score based on how well upper voices match the chord
    int upperNotes = __builtin_popcount(upperChord->mask);
    int totalNotes = __builtin_popcount(candidate.mask);
    candidate.matchScore = static_cast<float>(upperNotes + 1) / static_cast<float>(totalNotes); // +1 for bass
    
    return candidate;
}