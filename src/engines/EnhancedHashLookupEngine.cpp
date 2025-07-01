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
        
        // Apply key context boost if available
        if (keyContext_.isSet()) {
            confidence *= calculateKeyBoost(entry->name, mask);
        }
        
        // Strong boost for exact matches
        confidence *= 1.5f;
        
        // Additional boost for 7th chords when key context is available
        if (keyContext_.isSet()) {
            std::string name = entry->name;
            if (name.find("7") != std::string::npos || name.find("maj7") != std::string::npos) {
                confidence *= 2.0f; // Massive boost for direct 7th chord matches in key context
            }
        }
        
        // Analyze extensions for enhanced display
        ChordExtensions extensions = analyzeChordExtensions(mask, entry->name);
        std::string displayName = entry->name;
        if (extensions.hasExtensions()) {
            displayName += extensions.formatExtensions();
        }
        
        result.candidates.push_back({
            displayName,
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
    
    // Add specialized chord detections
    auto addDetailedCandidates = [&](const std::vector<DetailedChordCandidate>& detailedCands) {
        for (const auto& detailed : detailedCands) {
            bool isDuplicate = false;
            for (const auto& existing : result.candidates) {
                if (existing.name == detailed.getDisplayName()) {
                    isDuplicate = true;
                    break;
                }
            }
            
            if (!isDuplicate) {
                ChordCandidate candidate;
                candidate.name = detailed.getDisplayName();
                candidate.mask = detailed.mask;
                candidate.confidence = detailed.confidence;
                candidate.extensions = detailed.extensions;
                
                // Apply key boost if context is available
                if (keyContext_.isSet()) {
                    candidate.confidence *= calculateKeyBoost(detailed.name, detailed.mask);
                }
                
                result.candidates.push_back(candidate);
            }
        }
    };
    
    // Run specialized detections
    addDetailedCandidates(detect6thChords(mask));
    addDetailedCandidates(detectSusChords(mask));
    addDetailedCandidates(detectAugmentedChords(mask));
    addDetailedCandidates(detectDiminishedSeventhChords(mask));
    addDetailedCandidates(detectAlteredSeventhChords(mask));
    
    // Add key-aware analysis if context is available
    if (keyContext_.isSet()) {
        // Step 3: Enhanced tension analysis
        addDetailedCandidates(detectExtendedChords(mask));
        addDetailedCandidates(analyzeRootlessChords(mask));
        addDetailedCandidates(analyzePolychords(mask));
    }
    
    // Sort by confidence with functional harmony priority
    std::sort(result.candidates.begin(), result.candidates.end(),
              [this](const ChordCandidate& a, const ChordCandidate& b) {
                  // Apply functional harmony priority if key context is available
                  if (keyContext_.isSet()) {
                      std::string funcA = keyContext_.getChordFunction(a.name);
                      std::string funcB = keyContext_.getChordFunction(b.name);
                      
                      // Define functional priority levels
                      auto getFunctionalPriority = [](const std::string& func, const std::string& chordName) -> int {
                          // Check if it's a slash chord (lower priority)
                          bool isSlashChord = chordName.find("/") != std::string::npos;
                          
                          // Tonic function gets highest priority
                          if (func == "I" || func == "i") {
                              return isSlashChord ? 5 : 10;
                          }
                          // Dominant function gets second priority
                          else if (func == "V" || func == "v") {
                              return isSlashChord ? 4 : 9;
                          }
                          // Subdominant function gets third priority
                          else if (func == "IV" || func == "iv") {
                              return isSlashChord ? 3 : 8;
                          }
                          // Secondary functions
                          else if (func == "vi" || func == "VI" || func == "ii" || func == "iii") {
                              return isSlashChord ? 2 : 7;
                          }
                          // Secondary dominants
                          else if (func.find("V7/") == 0) {
                              return isSlashChord ? 1 : 6;
                          }
                          // Non-functional chords
                          else {
                              return isSlashChord ? 0 : 1;
                          }
                      };
                      
                      int priorityA = getFunctionalPriority(funcA, a.name);
                      int priorityB = getFunctionalPriority(funcB, b.name);
                      
                      // If functional priorities are different, use them
                      if (priorityA != priorityB) {
                          return priorityA > priorityB;
                      }
                  }
                  
                  // If no key context or same functional priority, use confidence
                  return a.confidence > b.confidence;
              });
    
    // Post-process with additional key context adjustments
    if (keyContext_.isSet() && !result.candidates.empty()) {
        // printf("POST-PROCESS: Key set, %zu candidates\n", result.candidates.size());
        for (auto& candidate : result.candidates) {
            std::string function = keyContext_.getChordFunction(candidate.name);
            bool isSlashChord = candidate.name.find("/") != std::string::npos;
            
            // Debug key function detection
            if (candidate.name.find("Em") == 0 || candidate.name.find("G/E") == 0) {
                printf("POST-DEBUG: chord=%s function='%s' confidence=%f\n", 
                       candidate.name.c_str(), function.c_str(), candidate.confidence);
            }
            
            // Apply massive boost to primary functional harmony that bypassed key boost
            if ((function == "I" || function == "i") && !isSlashChord) {
                candidate.confidence *= 3.0f; // Massive tonic boost
            }
            else if ((function == "V" || function == "v") && !isSlashChord) {
                candidate.confidence *= 2.5f; // Strong dominant boost  
            }
            else if ((function == "IV" || function == "iv") && !isSlashChord) {
                candidate.confidence *= 2.0f; // Subdominant boost
            }
            
            // Additional 7th chord boost for functional harmony
            if (candidate.name.find("7") != std::string::npos && !isSlashChord) {
                if (function == "I" || function == "i") {
                    candidate.confidence *= 1.5f; // i7 gets extra boost
                }
                else if (function == "V" || function == "v") {
                    candidate.confidence *= 1.3f; // V7 gets extra boost
                }
            }
        }
        
        // Re-sort after post-processing
        std::sort(result.candidates.begin(), result.candidates.end(),
                  [](const ChordCandidate& a, const ChordCandidate& b) {
                      return a.confidence > b.confidence;
                  });
    }
    
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
    
    // Debug: findDetailedAlternatives called with mask and noteCount
    
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
        
        // Analyze chord extensions for more complete labeling
        candidate.extensions = analyzeChordExtensions(mask, exactEntry->name);
        
        detailedCandidates.push_back(candidate);
    }
    
    // Special Strategy: 6th chord detection (C6, F6, etc.)
    // Check if this pattern could be a 6th chord (major triad + 6th)
    auto sixthChordCandidates = detect6thChords(mask);
    for (const auto& candidate : sixthChordCandidates) {
        detailedCandidates.push_back(candidate);
    }
    
    // Special Strategy: sus2/sus4 chord detection
    // Check if this pattern could be interpreted as sus2 instead of sus4
    auto susChordCandidates = detectSusChords(mask);
    for (const auto& candidate : susChordCandidates) {
        detailedCandidates.push_back(candidate);
    }
    
    // Special Strategy: augmented chord detection
    // Check if this pattern should prioritize different augmented root based on bass
    auto augmentedChordCandidates = detectAugmentedChords(mask);
    for (const auto& candidate : augmentedChordCandidates) {
        detailedCandidates.push_back(candidate);
    }
    
    // Special Strategy: diminished 7th chord detection
    // Check if this pattern matches a dim7 chord with proper root prioritization
    auto diminishedSeventhCandidates = detectDiminishedSeventhChords(mask);
    for (const auto& candidate : diminishedSeventhCandidates) {
        detailedCandidates.push_back(candidate);
    }
    
    // Special Strategy: altered 7th chord detection
    // Check if this pattern matches complex altered dominant chords
    auto alteredSeventhCandidates = detectAlteredSeventhChords(mask);
    for (const auto& candidate : alteredSeventhCandidates) {
        detailedCandidates.push_back(candidate);
    }
    
    // Strategy 2: Bass-aware slash chord analysis
    if (noteCount >= 3) {
        int bassNote = findLowestNote();
        if (bassNote >= 0) {
            int bassPitch = bassNote % 12;
            uint16_t upperMask = removeLowestNoteFromMask(mask, bassNote);
            
            // Bass analysis debug (uncomment for debugging)
            // std::cerr << "Bass analysis:" << std::endl;
            // std::cerr << "  Full mask: " << std::hex << mask << std::dec << std::endl;
            // std::cerr << "  Bass note: " << bassNote << " (pitch " << bassPitch << ")" << std::endl;
            // std::cerr << "  Upper mask: " << std::hex << upperMask << std::dec << std::endl;
            // std::cerr << "  Upper note count: " << __builtin_popcount(upperMask) << std::endl;
            
            // SMART ANALYSIS: Directly analyze full chord for natural slash chord patterns
            int bassRegister = bassNote / 12;
            int upperRegister = findHighestNote() / 12;
            int bassGap = findNextLowestNote() - bassNote; // Gap between bass and next note
            
            bool hasClearBassSeparation = false;
            
            // Strong bass separation indicators:
            // 1. Register difference (octave+ separation) - PRIMARY indicator
            // 2. Musical interval gap (3+ semitones = minor 3rd or larger) - SECONDARY
            // 3. Relaxed conditions for clear slash chord patterns
            bool hasRegisterSeparation = (upperRegister - bassRegister >= 1);
            bool hasPitchGap = (bassGap >= 3); // Relaxed from 4 to 3 semitones
            
            if (hasRegisterSeparation || hasPitchGap) { // Simplified: any pitch gap or register separation
                hasClearBassSeparation = true;
            }
            
            // Smart slash chord analysis - detect natural triads with bass separation
            bool isNaturalSlashChord = false;
            std::string naturalChordName = "";
            
            // Analyze if the full chord (bass + upper) forms a recognizable triad
            naturalChordName = analyzeNaturalSlashChord(mask, bassPitch, isNaturalSlashChord);
            
            if (isNaturalSlashChord) {
                // Create natural slash chord candidate
                DetailedChordCandidate slashCandidate;
                slashCandidate.name = naturalChordName + "/" + getNoteFromPitch(bassPitch);
                slashCandidate.mask = mask;
                slashCandidate.root = getNoteFromPitch(bassPitch); // Bass note becomes root for slash chords
                slashCandidate.isInversion = false;
                slashCandidate.inversionDegree = 0;
                slashCandidate.interpretationType = "natural_slash_chord";
                slashCandidate.matchScore = 1.0f;
                
                // Confidence based on bass separation quality and chord completeness
                // Count how many notes are in the chord to determine if it's a complete triad
                int totalNotes = __builtin_popcount(mask);
                bool isCompleteTriad = (totalNotes >= 3);
                
                // REVOLUTIONARY: Much higher base confidence for slash chords to compete with add9 chords
                if (hasClearBassSeparation) {
                    if (hasRegisterSeparation) {
                        if (isCompleteTriad) {
                            slashCandidate.confidence = 15.0f; // MASSIVE boost for excellent slash chords
                        } else {
                            slashCandidate.confidence = 12.0f; // Very high for good separation
                        }
                    } else {
                        if (isCompleteTriad) {
                            slashCandidate.confidence = 12.0f; // High for complete triad + pitch gap
                        } else {
                            slashCandidate.confidence = 8.0f; // Good for incomplete + pitch gap
                        }
                    }
                } else {
                    if (isCompleteTriad) {
                        slashCandidate.confidence = 10.0f; // High boost for complete triads
                    } else {
                        slashCandidate.confidence = 5.0f; // Moderate for incomplete
                    }
                }
                
                // Additional boosts for specific chord types
                if (naturalChordName.find("m") != std::string::npos) {
                    // Minor chord slash - boost to compete with m7b5 chords
                    slashCandidate.confidence += 3.0f; // Major boost for minor slash chords
                }
                
                // Massive boost for extended slash chords (add9, maj7, etc.)
                if (naturalChordName.find("add9") != std::string::npos || 
                    naturalChordName.find("maj9") != std::string::npos ||
                    naturalChordName.find("9") != std::string::npos) {
                    slashCandidate.confidence += 10.0f; // MASSIVE boost for 9th slash chords like Cmaj9/E
                }
                
                // KEY CONTEXT SPECIAL: Massive boost if upper chord is tonic function
                if (keyContext_.isSet()) {
                    std::string upperFunction = keyContext_.getChordFunction(naturalChordName);
                    if (upperFunction == "I" || upperFunction == "i") {
                        slashCandidate.confidence += 15.0f; // ENORMOUS boost for tonic slash chords like Cmaj9/E
                    } else if (upperFunction == "V" || upperFunction == "v") {
                        slashCandidate.confidence += 10.0f; // Large boost for dominant slash chords
                    } else if (upperFunction == "IV" || upperFunction == "iv") {
                        slashCandidate.confidence += 8.0f; // Good boost for subdominant slash chords  
                    }
                }
                
                // Analyze extensions for the upper chord structure
                slashCandidate.extensions = analyzeChordExtensions(mask, naturalChordName);
                
                detailedCandidates.push_back(slashCandidate);
            }
            
            // FALLBACK: Try original upper chord analysis for edge cases
            if (__builtin_popcount(upperMask) >= 2) {
                const EnhancedChordEntry* upperChord = findEnhancedChord(upperMask);
                if (upperChord) {
                    int upperRootPitch = -1;
                    for (int i = 0; i < 12; i++) {
                        if (upperMask & (1 << i)) {
                            upperRootPitch = i;
                            break;
                        }
                    }
                    
                    if (upperRootPitch >= 0 && bassPitch != upperRootPitch) {
                        auto slashCandidate = createSlashChordCandidate(upperChord, bassNote, upperChord->confidence);
                        
                        if (hasClearBassSeparation) {
                            slashCandidate.confidence *= 1.5f; // Boost for clear separation
                        } else {
                            slashCandidate.confidence *= 0.8f; // Slight demotion for unclear separation
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
    // Add key-aware analysis if context is available
    if (keyContext_.isSet()) {
        // printf("FIND-DETAILED: Adding key context analysis\n");
        
        // Add key-aware extended chord analysis
        auto extendedChords = detectExtendedChords(mask);
        for (const auto& extended : extendedChords) {
            // Check if we already have this chord to avoid duplicates
            bool exists = false;
            for (const auto& existing : detailedCandidates) {
                if (existing.name == extended.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                detailedCandidates.push_back(extended);
            }
        }
        
        // Add rootless chord analysis  
        auto rootlessChords = analyzeRootlessChords(mask);
        for (const auto& rootless : rootlessChords) {
            bool exists = false;
            for (const auto& existing : detailedCandidates) {
                if (existing.name == rootless.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                detailedCandidates.push_back(rootless);
            }
        }
        
        // Add polychord analysis
        auto polychords = analyzePolychords(mask);
        for (const auto& poly : polychords) {
            bool exists = false;
            for (const auto& existing : detailedCandidates) {
                if (existing.name == poly.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                detailedCandidates.push_back(poly);
            }
        }
    }
    
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
    
    // Apply key context boost to detailed candidates
    if (keyContext_.isSet() && !result.detailedCandidates.empty()) {
        // printf("DETAILED-PROCESS: Key set, %zu detailed candidates\n", result.detailedCandidates.size());
        
        for (auto& detailed : result.detailedCandidates) {
            std::string function = keyContext_.getChordFunction(detailed.name);
            bool isSlashChord = detailed.name.find("/") != std::string::npos;
            
            // Debug key function detection
            // if (detailed.name.find("Em") == 0 || detailed.name.find("C/E") == 0 || detailed.name.find("G/E") == 0) {
            //     printf("DETAILED-DEBUG: chord=%s function='%s' confidence=%f\n", 
            //            detailed.name.c_str(), function.c_str(), detailed.confidence);
            // }
            
            // Apply massive boost to primary functional harmony
            if ((function == "I" || function == "i") && !isSlashChord) {
                detailed.confidence *= 3.0f; // Massive tonic boost
            }
            else if ((function == "V" || function == "v") && !isSlashChord) {
                detailed.confidence *= 2.5f; // Strong dominant boost  
            }
            else if ((function == "IV" || function == "iv") && !isSlashChord) {
                detailed.confidence *= 2.0f; // Subdominant boost
            }
            
            // Additional 7th chord boost for functional harmony
            if (detailed.name.find("7") != std::string::npos && !isSlashChord) {
                if (function == "I" || function == "i") {
                    detailed.confidence *= 1.5f; // i7 gets extra boost
                }
                else if (function == "V" || function == "v") {
                    detailed.confidence *= 1.3f; // V7 gets extra boost
                }
            }
        }
        
        // Re-sort detailed candidates after boost
        std::sort(result.detailedCandidates.begin(), result.detailedCandidates.end(),
                  [](const DetailedChordCandidate& a, const DetailedChordCandidate& b) {
                      return a.confidence > b.confidence;
                  });
    }
    
    // Convert to simple candidates for backward compatibility
    for (const auto& detailed : result.detailedCandidates) {
        result.candidates.push_back({
            detailed.getDisplayName(), // Use enhanced display name with extensions
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

int EnhancedHashLookupEngine::findNextLowestNote() const {
    int lowestNote = findLowestNote();
    if (lowestNote < 0) return -1;
    
    // Find the next lowest note after the lowest
    for (int note = lowestNote + 1; note <= 127; note++) {
        if (noteStates_[note]) {
            return note;
        }
    }
    return -1; // No next note found
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

std::string EnhancedHashLookupEngine::analyzeNaturalSlashChord(uint16_t mask, int bassPitch, bool& isNaturalSlashChord) const {
    isNaturalSlashChord = false;
    
    // Check all 12 possible chord roots for major and minor triads
    for (int root = 0; root < 12; root++) {
        // Major triad: root + major third (4 semitones) + perfect fifth (7 semitones)
        uint16_t majorTriad = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 7) % 12));
        
        // Minor triad: root + minor third (3 semitones) + perfect fifth (7 semitones)  
        uint16_t minorTriad = (1 << root) | (1 << ((root + 3) % 12)) | (1 << ((root + 7) % 12));
        
        // Check if mask contains this major triad
        if ((mask & majorTriad) == majorTriad) {
            int third = (root + 4) % 12;
            int fifth = (root + 7) % 12;
            
            // Is bass note the 3rd or 5th of this triad (not the root)?
            if (bassPitch == third || bassPitch == fifth) {
                isNaturalSlashChord = true;
                return getNoteFromPitch(root); // Return chord name (e.g., "C", "F", "G")
            }
            
            // Also check for non-chord tone bass notes (extended slash chords)
            // If bass is not part of the triad but triad is complete, consider it a valid slash chord
            if (bassPitch != root && bassPitch != third && bassPitch != fifth) {
                isNaturalSlashChord = true;
                return getNoteFromPitch(root); // Extended slash chord with non-chord bass
            }
        }
        
        // Check if mask contains this minor triad
        if ((mask & minorTriad) == minorTriad) {
            int third = (root + 3) % 12;
            int fifth = (root + 7) % 12;
            
            // Is bass note the 3rd or 5th of this triad (not the root)?
            if (bassPitch == third || bassPitch == fifth) {
                isNaturalSlashChord = true;
                return getNoteFromPitch(root) + "m"; // Return minor chord name (e.g., "Cm", "Fm", "Gm")
            }
            
            // Also check for non-chord tone bass notes (extended slash chords)
            // If bass is not part of the triad but triad is complete, consider it a valid slash chord
            if (bassPitch != root && bassPitch != third && bassPitch != fifth) {
                isNaturalSlashChord = true;
                return getNoteFromPitch(root) + "m"; // Extended slash chord with non-chord bass
            }
        }
    }
    
    return ""; // No natural slash chord found
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::detect6thChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    // Check if this pattern could be a 6th chord (major triad + 6th)
    // 6th chords: C6 = C-E-G-A, F6 = F-A-C-D, etc.
    
    // Check all 12 possible roots for major triad + 6th patterns
    for (int root = 0; root < 12; root++) {
        // Major triad: root + major third (4 semitones) + perfect fifth (7 semitones)
        uint16_t majorTriad = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 7) % 12));
        
        // 6th note: root + major sixth (9 semitones)
        uint16_t sixth = (1 << ((root + 9) % 12));
        
        // Complete 6th chord pattern: major triad + 6th
        uint16_t sixthChordMask = majorTriad | sixth;
        
        // Check if the input mask exactly matches this 6th chord pattern
        if (mask == sixthChordMask) {
            // Get the lowest note to determine if this interpretation makes sense
            int bassNote = findLowestNote();
            if (bassNote >= 0) {
                int bassPitch = bassNote % 12;
                
                // Only prioritize 6th chord interpretation when root note is in bass
                // This helps distinguish C6 from Am7 while avoiding false positives
                float confidence = 0.650f; // Conservative base confidence
                
                if (bassPitch == root) {
                    // Root in bass - favor 6th chord interpretation
                    confidence = 1.450f; // Much higher confidence to outcompete slash chords
                } else {
                    // Non-root bass - significantly reduce confidence to avoid overriding 7th chords
                    confidence = 0.400f; // Low confidence to prevent false positives
                }
                
                EnhancedHashLookupEngine::DetailedChordCandidate candidate;
                candidate.name = getNoteFromPitch(root) + "6";
                candidate.mask = sixthChordMask;
                candidate.confidence = confidence;
                candidate.root = getNoteFromPitch(root);
                candidate.isInversion = (bassPitch != root);
                candidate.inversionDegree = 0;
                candidate.interpretationType = "sixth_chord";
                candidate.matchScore = 1.0f;
                
                candidates.push_back(candidate);
            }
        }
    }
    
    return candidates;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::detectSusChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    // Check if this pattern could be interpreted as sus2 instead of sus4
    // sus2: root + major 2nd (2 semitones) + perfect 5th (7 semitones)
    // sus4: root + perfect 4th (5 semitones) + perfect 5th (7 semitones)
    
    // Known conflicting patterns where both sus2 and sus4 are valid interpretations:
    struct SusPattern {
        uint16_t mask;
        int sus2Root; // Root for sus2 interpretation
        int sus4Root; // Root for sus4 interpretation
    };
    
    const SusPattern patterns[] = {
        {0x0085, 0, 7},   // C-D-G: Csus2 vs Gsus4
        {0x00a1, 5, 0},   // F-G-C: Fsus2 vs Csus4
        {0x0284, 7, 2}    // G-A-D: Gsus2 vs Dsus4
    };
    
    // Check if current mask matches any known sus pattern
    for (const auto& pattern : patterns) {
        if (mask == pattern.mask) {
            int bassNote = findLowestNote();
            if (bassNote >= 0) {
                int bassPitch = bassNote % 12;
                
                // Determine which interpretation (sus2 or sus4) fits better based on bass note
                if (bassPitch == pattern.sus2Root) {
                    // Bass matches sus2 root - create sus2 candidate with priority
                    EnhancedHashLookupEngine::DetailedChordCandidate sus2Candidate;
                    sus2Candidate.name = getNoteFromPitch(pattern.sus2Root) + "sus2";
                    sus2Candidate.mask = mask;
                    sus2Candidate.confidence = 1.800f; // Very high confidence when bass matches root
                    sus2Candidate.root = getNoteFromPitch(pattern.sus2Root);
                    sus2Candidate.isInversion = false;
                    sus2Candidate.inversionDegree = 0;
                    sus2Candidate.interpretationType = "sus_chord";
                    sus2Candidate.matchScore = 1.0f;
                    
                    candidates.push_back(sus2Candidate);
                } else if (bassPitch == pattern.sus4Root) {
                    // Bass matches sus4 root - the existing sus4 detection should handle this
                    // But we can still add sus2 as a lower-confidence alternative
                    EnhancedHashLookupEngine::DetailedChordCandidate sus2Candidate;
                    sus2Candidate.name = getNoteFromPitch(pattern.sus2Root) + "sus2";
                    sus2Candidate.mask = mask;
                    sus2Candidate.confidence = 0.600f; // Lower confidence when bass doesn't match
                    sus2Candidate.root = getNoteFromPitch(pattern.sus2Root);
                    sus2Candidate.isInversion = true;
                    sus2Candidate.inversionDegree = 1;
                    sus2Candidate.interpretationType = "sus_chord";
                    sus2Candidate.matchScore = 0.8f;
                    
                    candidates.push_back(sus2Candidate);
                } else {
                    // Bass is neither root - provide both interpretations with moderate confidence
                    // Add sus2 interpretation
                    EnhancedHashLookupEngine::DetailedChordCandidate sus2Candidate;
                    sus2Candidate.name = getNoteFromPitch(pattern.sus2Root) + "sus2";
                    sus2Candidate.mask = mask;
                    sus2Candidate.confidence = 0.700f; // Moderate confidence
                    sus2Candidate.root = getNoteFromPitch(pattern.sus2Root);
                    sus2Candidate.isInversion = true;
                    sus2Candidate.inversionDegree = 2;
                    sus2Candidate.interpretationType = "sus_chord";
                    sus2Candidate.matchScore = 0.85f;
                    
                    candidates.push_back(sus2Candidate);
                }
                
                break; // Found matching pattern, no need to check others
            }
        }
    }
    
    return candidates;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::detectAugmentedChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    // Check if this pattern could be an augmented chord with different root interpretation
    // Augmented chords: root + major 3rd (4 semitones) + augmented 5th (8 semitones)
    
    // Known augmented patterns where multiple roots are valid:
    struct AugmentedPattern {
        uint16_t mask;
        int roots[3]; // Three possible roots for the same augmented chord
        const char* note_names[3];
    };
    
    const AugmentedPattern patterns[] = {
        {0x0111, {0, 4, 8}, {"C", "E", "G#"}},     // Caug/Eaug/G#aug: C-E-G#
        {0x0222, {1, 5, 9}, {"C#", "F", "A"}},    // C#aug/Faug/Aaug: C#-F-A
        {0x0444, {2, 6, 10}, {"D", "F#", "A#"}},  // Daug/F#aug/A#aug: D-F#-A#
        {0x0888, {3, 7, 11}, {"D#", "G", "B"}}    // D#aug/Gaug/Baug: D#-G-B
    };
    
    // Check if current mask matches any known augmented pattern
    for (const auto& pattern : patterns) {
        if (mask == pattern.mask) {
            int bassNote = findLowestNote();
            if (bassNote >= 0) {
                int bassPitch = bassNote % 12;
                
                // Find which root interpretation best matches the bass note
                for (int i = 0; i < 3; i++) {
                    if (bassPitch == pattern.roots[i]) {
                        // Bass matches this augmented root - create high-confidence candidate
                        EnhancedHashLookupEngine::DetailedChordCandidate augCandidate;
                        augCandidate.name = std::string(pattern.note_names[i]) + "aug";
                        augCandidate.mask = mask;
                        augCandidate.confidence = 1.200f; // High confidence when bass matches root
                        augCandidate.root = pattern.note_names[i];
                        augCandidate.isInversion = false;
                        augCandidate.inversionDegree = 0;
                        augCandidate.interpretationType = "augmented_chord";
                        augCandidate.matchScore = 1.0f;
                        
                        candidates.push_back(augCandidate);
                        break; // Found the best interpretation
                    }
                }
                
                break; // Found matching pattern, no need to check others
            }
        }
    }
    
    return candidates;
}
std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::detectDiminishedSeventhChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    // Diminished 7th chords have a symmetric structure: root + minor 3rd + diminished 5th + diminished 7th
    // Each dim7 chord has 4 enharmonic equivalents (every minor 3rd interval)
    // Pattern: 0, 3, 6, 9 semitones
    
    // Known diminished 7th patterns:
    struct DiminishedSeventhPattern {
        uint16_t mask;
        int roots[4]; // Four possible roots for the same dim7 chord
        const char* note_names[4];
    };
    
    const DiminishedSeventhPattern patterns[] = {
        {0x0249, {0, 3, 6, 9}, {"C", "D#", "F#", "A"}},     // Cdim7/D#dim7/F#dim7/Adim7: C-D#-F#-A
        {0x0492, {1, 4, 7, 10}, {"C#", "E", "G", "A#"}},   // C#dim7/Edim7/Gdim7/A#dim7: C#-E-G-A#
        {0x0924, {2, 5, 8, 11}, {"D", "F", "G#", "B"}}     // Ddim7/Fdim7/G#dim7/Bdim7: D-F-G#-B
    };
    
    // Check if current mask matches any known dim7 pattern
    for (const auto& pattern : patterns) {
        if (mask == pattern.mask) {
            int bassNote = findLowestNote();
            if (bassNote >= 0) {
                int bassPitch = bassNote % 12;
                
                // Find which root interpretation best matches the bass note
                for (int i = 0; i < 4; i++) {
                    if (bassPitch == pattern.roots[i]) {
                        // Bass matches this dim7 root - create high-confidence candidate
                        EnhancedHashLookupEngine::DetailedChordCandidate dim7Candidate;
                        dim7Candidate.name = std::string(pattern.note_names[i]) + "dim7";
                        dim7Candidate.mask = mask;
                        dim7Candidate.confidence = 1.250f; // High confidence for exact dim7 matches
                        dim7Candidate.root = pattern.note_names[i];
                        dim7Candidate.isInversion = false;
                        dim7Candidate.inversionDegree = 0;
                        dim7Candidate.interpretationType = "diminished_seventh";
                        dim7Candidate.matchScore = 1.0f;
                        
                        candidates.push_back(dim7Candidate);
                        break; // Found the best interpretation
                    }
                }
                
                break; // Found matching pattern, no need to check others
            }
        }
    }
    
    return candidates;
}

ChordExtensions EnhancedHashLookupEngine::analyzeChordExtensions(uint16_t mask, const std::string& baseChordName) const {
    ChordExtensions extensions;
    
    // Determine the root note from base chord name
    int rootPitch = -1;
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    for (int i = 0; i < 12; i++) {
        if (baseChordName.find(noteNames[i]) == 0) {
            rootPitch = i;
            break;
        }
    }
    
    if (rootPitch < 0) return extensions; // Could not determine root
    
    // Determine if this is a major or minor chord
    bool isMinor = (baseChordName.find("m") != std::string::npos && 
                   baseChordName.find("maj") == std::string::npos);
    
    // Define basic triad patterns
    uint16_t majorTriad = (1 << rootPitch) | (1 << ((rootPitch + 4) % 12)) | (1 << ((rootPitch + 7) % 12));
    uint16_t minorTriad = (1 << rootPitch) | (1 << ((rootPitch + 3) % 12)) | (1 << ((rootPitch + 7) % 12));
    uint16_t baseTriad = isMinor ? minorTriad : majorTriad;
    
    // Check for add tones
    uint16_t extraMask = mask & (~baseTriad);
    
    // Check for common add tones
    if (extraMask & (1 << ((rootPitch + 2) % 12))) {
        extensions.addTones.push_back("add9");
    }
    if (extraMask & (1 << ((rootPitch + 5) % 12))) {
        extensions.addTones.push_back("add11");
    }
    if (extraMask & (1 << ((rootPitch + 9) % 12))) {
        // Could be add6 or part of a 7th chord
        if (baseChordName.find("7") == std::string::npos) {
            extensions.addTones.push_back("add6");
        }
    }
    if (extraMask & (1 << ((rootPitch + 10) % 12))) {
        // Minor 7th - only add if not already in base chord name
        if (baseChordName.find("7") == std::string::npos) {
            extensions.addTones.push_back("add7");
        }
    }
    if (extraMask & (1 << ((rootPitch + 11) % 12))) {
        // Major 7th - only add if not already in base chord name  
        if (baseChordName.find("maj7") == std::string::npos && baseChordName.find("M7") == std::string::npos) {
            extensions.addTones.push_back("addMaj7");
        }
    }
    
    // Check for alterations
    if (extraMask & (1 << ((rootPitch + 8) % 12))) {
        extensions.alterations.push_back("#5");
    }
    if (extraMask & (1 << ((rootPitch + 6) % 12))) {
        extensions.alterations.push_back("b5");
    }
    if (extraMask & (1 << ((rootPitch + 1) % 12))) {
        extensions.alterations.push_back("b9");
    }
    if (extraMask & (1 << ((rootPitch + 3) % 12))) {
        if (!isMinor) { // Only in major chords, otherwise it is the minor 3rd
            extensions.alterations.push_back("#9");
        }
    }
    
    // Check for sus chords
    bool hasThird = mask & (1 << ((rootPitch + (isMinor ? 3 : 4)) % 12));
    bool hasSecond = mask & (1 << ((rootPitch + 2) % 12));
    bool hasFourth = mask & (1 << ((rootPitch + 5) % 12));
    
    if (!hasThird) {
        if (hasSecond && !hasFourth) {
            extensions.suspensions.push_back("sus2");
        } else if (hasFourth && !hasSecond) {
            extensions.suspensions.push_back("sus4");
        } else if (hasSecond && hasFourth) {
            extensions.suspensions.push_back("sus2sus4");
        }
    }
    
    return extensions;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::detectAlteredSeventhChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    // Altered 7th chords contain: root + major 3rd + minor 7th + altered tensions
    // Common patterns: 7#5, 7b9, 7#9, 7b13, 7alt (multiple alterations)
    
    struct AlteredSeventhPattern {
        uint16_t mask;
        int root;
        const char* rootName;
        std::vector<std::string> alterations;
        std::string chordName;
    };
    
    // Generate altered patterns for all 12 roots
    std::vector<AlteredSeventhPattern> patterns;
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    for (int root = 0; root < 12; root++) {
        // Basic dominant 7th: root + major 3rd (4) + perfect 5th (7) + minor 7th (10)
        uint16_t basicDom7 = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 7) % 12)) | (1 << ((root + 10) % 12));
        
        // Common altered patterns:
        
        // 1. 7#5 (augmented 5th instead of perfect 5th)
        uint16_t aug5 = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 8) % 12)) | (1 << ((root + 10) % 12));
        if (mask == aug5) {
            patterns.push_back({aug5, root, noteNames[root], {"#5"}, noteNames[root] + std::string("7#5")});
        }
        
        // 2. 7b9 (flat 9th added)
        uint16_t flat9 = basicDom7 | (1 << ((root + 1) % 12));
        if (mask == flat9) {
            patterns.push_back({flat9, root, noteNames[root], {"b9"}, noteNames[root] + std::string("7b9")});
        }
        
        // 3. 7#9 (sharp 9th added)  
        uint16_t sharp9 = basicDom7 | (1 << ((root + 3) % 12));
        if (mask == sharp9) {
            patterns.push_back({sharp9, root, noteNames[root], {"#9"}, noteNames[root] + std::string("7#9")});
        }
        
        // 4. 7#5#9 (both #5 and #9)
        uint16_t aug5sharp9 = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 8) % 12)) | (1 << ((root + 10) % 12)) | (1 << ((root + 3) % 12));
        if (mask == aug5sharp9) {
            patterns.push_back({aug5sharp9, root, noteNames[root], {"#5", "#9"}, noteNames[root] + std::string("7#5#9")});
        }
        
        // 5. 7#5b9 (both #5 and b9)
        uint16_t aug5flat9 = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 8) % 12)) | (1 << ((root + 10) % 12)) | (1 << ((root + 1) % 12));
        if (mask == aug5flat9) {
            patterns.push_back({aug5flat9, root, noteNames[root], {"#5", "b9"}, noteNames[root] + std::string("7#5b9")});
        }
        
        // 6. 7alt (complex altered - without 5th, with multiple alterations)
        // Pattern: root + 3rd + 7th + b9 + #9 + #11 + b13
        uint16_t alt = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 10) % 12)) | 
                      (1 << ((root + 1) % 12)) | (1 << ((root + 3) % 12)) | (1 << ((root + 6) % 12)) | (1 << ((root + 8) % 12));
        // Check if mask contains most of these alterations (flexible matching)
        int altMatches = __builtin_popcount(mask & alt);
        int totalAltNotes = __builtin_popcount(alt);
        if (altMatches >= totalAltNotes - 1) { // Allow one missing note
            patterns.push_back({mask, root, noteNames[root], {"alt"}, noteNames[root] + std::string("7alt")});
        }
    }
    
    // Check if current mask matches any altered pattern
    for (const auto& pattern : patterns) {
        if (mask == pattern.mask || (pattern.alterations.size() == 1 && pattern.alterations[0] == "alt")) {
            int bassNote = findLowestNote();
            if (bassNote >= 0) {
                int bassPitch = bassNote % 12;
                
                // Prioritize when bass matches the root
                float confidence = 0.850f; // Base confidence for altered chords
                if (bassPitch == pattern.root) {
                    confidence = 1.200f; // Higher confidence when bass matches root
                }
                
                EnhancedHashLookupEngine::DetailedChordCandidate altCandidate;
                altCandidate.name = pattern.chordName;
                altCandidate.mask = mask;
                altCandidate.confidence = confidence;
                altCandidate.root = pattern.rootName;
                altCandidate.isInversion = (bassPitch != pattern.root);
                altCandidate.inversionDegree = 0;
                altCandidate.interpretationType = "altered_seventh";
                altCandidate.matchScore = 1.0f;
                
                // Set alterations in extensions
                for (const auto& alt : pattern.alterations) {
                    altCandidate.extensions.alterations.push_back(alt);
                }
                
                candidates.push_back(altCandidate);
                break; // Found a match, no need to check others
            }
        }
    }
    
    return candidates;
}

// Key context methods
void EnhancedHashLookupEngine::setKeyContext(const KeyContext& key) {
    keyContext_ = key;
}

void EnhancedHashLookupEngine::setKeyContext(int tonic, bool isMinor) {
    keyContext_ = KeyContext(tonic, isMinor);
}

void EnhancedHashLookupEngine::clearKeyContext() {
    keyContext_ = KeyContext();
}

const KeyContext& EnhancedHashLookupEngine::getKeyContext() const {
    return keyContext_;
}

float EnhancedHashLookupEngine::calculateKeyBoost(const std::string& chordName, uint16_t mask) const {
    if (!keyContext_.isSet()) return 1.0f;
    
    // Function-based analysis
    std::string function = keyContext_.getChordFunction(chordName);
    
    // Parse root from chord name to get degree in key
    int chordRoot = -1;
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < 12; i++) {
        if (chordName.find(noteNames[i]) == 0) {
            chordRoot = i;
            break;
        }
    }
    
    float boost = 1.0f;
    
    if (chordRoot >= 0) {
        // Calculate degree in key (0=I, 1=bII, etc.)
        int degree = (chordRoot - keyContext_.tonicPitch + 12) % 12;
        
        // Key-based degree scoring (Step 1)
        if (keyContext_.isMinor) {
            // Minor key priorities: i(0), iv(5), V(7), bIII(3), bVI(8), bVII(10), ii°(2)
            float degreeBonus[] = {0.3f, 0.0f, 0.1f, 0.2f, 0.0f, 0.2f, 0.0f, 0.25f, 0.2f, 0.0f, 0.15f, 0.0f};
            boost += degreeBonus[degree];
        } else {
            // Major key priorities: I(0), IV(5), V(7), vi(9), ii(2), iii(4), vii°(11)
            float degreeBonus[] = {0.3f, 0.0f, 0.2f, 0.0f, 0.1f, 0.2f, 0.0f, 0.25f, 0.0f, 0.2f, 0.0f, 0.1f};
            boost += degreeBonus[degree];
        }
    }
    
    // Scale relevance bonus
    float scaleRelevance = keyContext_.getScaleRelevance(mask);
    if (scaleRelevance >= 0.8f) {
        boost += 0.8f; // Very strong boost for fully diatonic chords
    } else if (scaleRelevance >= 0.6f) {
        boost += 0.4f * scaleRelevance;
    }
    
    // Function-based bonuses with absolute priority system
    // std::string function = keyContext_.getChordFunction(chordName); // Already defined above
    bool isTonicFunction = (function == "I" || function == "i");
    bool isDominantFunction = (function == "V" || function == "v");
    bool isSubdominantFunction = (function == "IV" || function == "iv");
    bool isPrimaryFunction = isTonicFunction || isDominantFunction || isSubdominantFunction;
    
    // Primary functional harmony gets substantial boost
    if (isPrimaryFunction) {
        boost += 1.2f; // Increased from 0.7f for stronger priority
        
        // Tonic function gets absolute priority
        if (isTonicFunction) {
            boost += 2.0f; // Massive tonic boost (total +3.2f) to ensure priority over slash chords
        }
        // Dominant function gets strong priority
        else if (isDominantFunction) {
            boost += 1.5f; // Strong dominant boost (total +2.7f)
        }
        // Subdominant function gets moderate priority
        else if (isSubdominantFunction) {
            boost += 1.0f; // Moderate subdominant boost (total +2.2f)
        }
    }
    
    if (function == "vi" || function == "VI" || function == "ii" || function == "iii" || function == "vii") {
        boost += 0.4f;
    }
    
    // 7th chord bonuses with enhanced functional priority
    if ((chordName.find("7") != std::string::npos || chordName.find("maj7") != std::string::npos)) {
        if (isDominantFunction) {
            boost += 1.8f; // Very strong dominant 7th boost
        } else if (isTonicFunction) {
            boost += 1.5f; // Very strong tonic 7th boost (total +4.7f for i7)
        } else if (isSubdominantFunction || function == "ii" || function == "vii") {
            boost += 1.2f; // Strong secondary function 7th boost
        } else if (function.find("V7/") == 0) {
            boost += 1.4f; // Strong secondary dominant boost
        }
    }
    
    // Slash chord scoring enhancement for better competition with triad+add9
    if (chordName.find("/") != std::string::npos) {
        // Revolutionary slash chord evaluation - bass notes get major boost
        float slashBonus = 1.0f; // Start neutral, then boost
        
        // Parse bass note from slash chord (e.g., "C/E" -> E)
        size_t slashPos = chordName.find("/");
        if (slashPos != std::string::npos && slashPos + 1 < chordName.length()) {
            std::string bassNote = chordName.substr(slashPos + 1);
            for (int i = 0; i < 12; i++) {
                if (bassNote == noteNames[i]) {
                    int bassDegree = (i - keyContext_.tonicPitch + 12) % 12;
                    
                    // Massive boost for key-relevant bass notes
                    if (keyContext_.isScaleTone(i)) {
                        // Key-tone bass notes get massive boost
                        slashBonus = 2.0f; // Was penalty 0.8, now major boost
                        
                        // Extra boost for important scale degrees
                        if (bassDegree == 0 || bassDegree == 5 || bassDegree == 7) { // I, IV, V
                            slashBonus = 2.5f; // Even bigger boost for primary functions
                        }
                        else if (bassDegree == 2 || bassDegree == 4 || bassDegree == 9) { // ii, iii, vi
                            slashBonus = 2.2f; // Strong boost for secondary functions
                        }
                    }
                    else {
                        // Non-diatonic bass still gets modest penalty
                        slashBonus = 0.7f; // Reduced from 0.5
                    }
                    
                    break;
                }
            }
        }
        
        // Apply the slash chord boost
        boost *= slashBonus;
        
        // Additional boost if the slash chord has extensions
        if (chordName.find("add") != std::string::npos || chordName.find("9") != std::string::npos) {
            boost *= 1.3f; // Extra boost for extended slash chords like Cmaj9/E
        }
    }
    
    return boost;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::detectExtendedChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    // printf("DETECT-EXTENDED: Called with mask=%04x, key set=%d\n", mask, keyContext_.isSet());
    
    if (!keyContext_.isSet()) return candidates;
    
    // Step 3: Enhanced tension preservation system with tonic priority
    int tonic = keyContext_.tonicPitch;
    // Put tonic first for priority, then other key roots
    std::vector<int> keyRoots = {tonic}; // Start with tonic for absolute priority
    
    // Add other key roots
    keyRoots.push_back((tonic + 5) % 12);  // IV
    keyRoots.push_back((tonic + 7) % 12);  // V  
    keyRoots.push_back((tonic + 9) % 12);  // vi (or VI in minor)
    
    for (int root : keyRoots) {
        // Check for basic triad patterns with extensions
        uint16_t majorTriad = (1 << root) | (1 << ((root + 4) % 12)) | (1 << ((root + 7) % 12));
        uint16_t minorTriad = (1 << root) | (1 << ((root + 3) % 12)) | (1 << ((root + 7) % 12));
        
        // Debug triad detection for G root
        // if (root == 7) { // G = 7
        //     printf("TRIAD-DEBUG: G root, mask=%04x, minorTriad=%04x, intersection=%04x, match=%d\n", 
        //            mask, minorTriad, (mask & minorTriad), (mask & minorTriad) == minorTriad);
        // }
        
        // Check if mask contains a triad + extensions (allow incomplete triads in key context)
        bool hasMajorTriad = (mask & majorTriad) == majorTriad;
        bool hasMinorTriad = (mask & minorTriad) == minorTriad;
        
        // Allow incomplete triads if root + third are present and we have extensions
        bool hasIncompleteMinor = false;
        bool hasIncompleteMajor = false;
        uint16_t minimalMinor = (1 << root) | (1 << ((root + 3) % 12)); // Root + minor third
        uint16_t minimalMajor = (1 << root) | (1 << ((root + 4) % 12)); // Root + major third
        
        if ((mask & minimalMinor) == minimalMinor && __builtin_popcount(mask) >= 3) {
            hasIncompleteMinor = true;
        }
        if ((mask & minimalMajor) == minimalMajor && __builtin_popcount(mask) >= 3) {
            hasIncompleteMajor = true;
        }
        
        if (hasMajorTriad || hasMinorTriad || hasIncompleteMinor || hasIncompleteMajor) {
            std::string baseName = getNoteFromPitch(root);
            if (hasMinorTriad || hasIncompleteMinor) baseName += "m";
            
            // Identify extensions (Step 3)
            std::vector<std::string> extensions;
            uint16_t baseTriad;
            if (hasMajorTriad) {
                baseTriad = majorTriad;
            } else if (hasMinorTriad) {
                baseTriad = minorTriad;
            } else if (hasIncompleteMinor) {
                baseTriad = minimalMinor;
            } else {
                baseTriad = minimalMajor;
            }
            uint16_t extensionMask = mask & (~baseTriad);
            
            // Enhanced tension detection with proper minor/major distinction
            if (extensionMask & (1 << ((root + 2) % 12))) { // 9th
                extensions.push_back("add9");
            }
            if (extensionMask & (1 << ((root + 5) % 12))) { // 11th (but watch for conflicts with 3rd)
                if (!hasMajorTriad) { // Only add11 in minor or sus contexts
                    extensions.push_back("add11");
                }
            }
            
            // CRITICAL: Proper 7th chord quality determination
            if (extensionMask & (1 << ((root + 10) % 12))) { // b7 (minor 7th)
                if (hasMinorTriad || hasIncompleteMinor) {
                    // Minor triad + b7 = m7, NOT just "7"
                    if (baseName.find("m") != std::string::npos) {
                        baseName = baseName.substr(0, baseName.find("m")) + "m7"; // Replace "m" with "m7"
                    } else {
                        baseName += "m7"; // Add m7 if somehow missed
                    }
                } else {
                    // Major triad + b7 = dominant 7th
                    baseName += "7";
                }
            }
            if (extensionMask & (1 << ((root + 11) % 12))) { // maj7
                if (hasMinorTriad || hasIncompleteMinor) {
                    baseName += "maj7"; // mmaj7 chord
                } else {
                    baseName += "maj7"; // major 7th
                }
            }
            
            // Step 5: Minor key ♭6 evaluation
            if (keyContext_.isMinor && extensionMask & (1 << ((root + 8) % 12))) { // ♭6 in minor
                // ♭6 is a natural tension in minor keys, not a conflict
                extensions.push_back("add♭6");
            }
            if (extensionMask & (1 << ((root + 9) % 12))) { // 6th/13th
                extensions.push_back("add6");
            }
            
            // Create candidate with proper extension labeling
            if (!extensions.empty() || baseName.find("7") != std::string::npos) {
                DetailedChordCandidate candidate;
                candidate.name = baseName;
                
                // Add extensions to name
                for (const auto& ext : extensions) {
                    candidate.name += "(" + ext + ")";
                }
                
                candidate.mask = mask;
                candidate.root = getNoteFromPitch(root);
                candidate.isInversion = false;
                candidate.inversionDegree = 0;
                candidate.interpretationType = "extended";
                
                // Calculate confidence based on key relevance
                int noteCount = __builtin_popcount(mask);
                int triadNotes = __builtin_popcount(baseTriad);
                float extensionBonus = 0.1f * extensions.size();
                
                candidate.confidence = 1.2f + extensionBonus; // Very high base confidence for key-aware extended chords
                candidate.matchScore = (float)triadNotes / (float)noteCount;
                
                // Apply key boost for functional harmony
                candidate.confidence *= calculateKeyBoost(baseName, mask);
                
                // Special boost for key-appropriate extensions
                if (root == tonic) {
                    candidate.confidence += 0.4f; // Very strong tonic extensions boost
                } else if (root == (tonic + 7) % 12) {
                    candidate.confidence += 0.3f; // Strong dominant extensions boost
                } else if (root == (tonic + 5) % 12) {
                    candidate.confidence += 0.25f; // Subdominant extensions boost
                }
                
                // Major priority boost when extensions match key function
                if (extensions.size() >= 2) {
                    candidate.confidence += 0.2f; // Multi-tension bonus
                }
                
                // Add extension details
                for (const auto& ext : extensions) {
                    if (ext == "add9") candidate.extensions.addTones.push_back("add9");
                    if (ext == "add11") candidate.extensions.addTones.push_back("add11");
                    if (ext == "add6") candidate.extensions.addTones.push_back("add6");
                }
                
                // Debug extended chord generation for Gm chords
                // if (candidate.name.find("Gm") == 0) {
                //     printf("EXTENDED-DEBUG: Generated %s confidence=%f\n", 
                //            candidate.name.c_str(), candidate.confidence);
                // }
                
                candidates.push_back(candidate);
            }
        }
    }
    
    return candidates;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::analyzeRootlessChords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    if (!keyContext_.isSet()) return candidates;
    
    // Enhanced rootless detection (Step 2)
    int tonic = keyContext_.tonicPitch;
    int dominant = (tonic + 7) % 12;
    int subdominant = (tonic + 5) % 12;
    int mediant = keyContext_.isMinor ? (tonic + 3) % 12 : (tonic + 4) % 12;
    int submediant = keyContext_.isMinor ? (tonic + 8) % 12 : (tonic + 9) % 12;
    
    // Expanded potential roots for better rootless detection
    std::vector<int> potentialRoots = {tonic, dominant, subdominant, mediant, submediant};
    
    for (int root : potentialRoots) {
        // Skip if root is already in the mask
        if (mask & (1 << root)) continue;
        
        // Try adding this root to complete a triad/7th chord
        uint16_t completedMask = mask | (1 << root);
        
        // Check various chord types with this root
        std::vector<std::string> chordTypes = {"", "m", "7", "maj7", "m7", "9", "maj9"};
        
        for (const std::string& type : chordTypes) {
            // Generate expected patterns for different chord types
            uint16_t expectedPattern = 0;
            std::string expectedName = getNoteFromPitch(root) + type;
            
            // Check if completed mask matches expected chord patterns
            const EnhancedChordEntry* entry = findEnhancedChord(completedMask);
            if (entry && std::string(entry->name).find(getNoteFromPitch(root)) == 0) {
                // Calculate confidence based on how well the mask fits
                int noteCount = __builtin_popcount(mask);
                int expectedNotes = __builtin_popcount(entry->mask);
                float rootlessBonus = 0.15f; // Base rootless allowance
                
                // Special bonus for key-appropriate rootless chords
                if (root == tonic && type.find("maj") != std::string::npos) {
                    rootlessBonus += 0.2f; // Tonic major rootless chords
                } else if (root == dominant && type.find("7") != std::string::npos) {
                    rootlessBonus += 0.25f; // Dominant 7th rootless chords
                }
                
                DetailedChordCandidate candidate;
                candidate.name = std::string(entry->name) + " (rootless)";
                candidate.mask = mask; // Keep original mask without root
                candidate.confidence = entry->confidence * 0.8f + rootlessBonus;
                candidate.root = getNoteFromPitch(root);
                candidate.isInversion = false;
                candidate.inversionDegree = 0;
                candidate.interpretationType = "rootless";
                candidate.matchScore = (float)(noteCount) / (float)(expectedNotes);
                
                // Apply key boost for functional harmony
                candidate.confidence *= calculateKeyBoost(entry->name, completedMask);
                
                candidates.push_back(candidate);
                break; // Found a match for this root
            }
        }
    }
    
    return candidates;
}

std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> EnhancedHashLookupEngine::analyzePolychords(uint16_t mask) const {
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> candidates;
    
    if (!keyContext_.isSet()) return candidates;
    
    int noteCount = __builtin_popcount(mask);
    if (noteCount < 4) return candidates; // Need at least 4 notes for polychord
    
    // Try to split the mask into two triads
    for (int split = 1; split < noteCount - 1; split++) {
        // Try different ways to split the notes
        std::vector<int> notes;
        for (int pitch = 0; pitch < 12; pitch++) {
            if (mask & (1 << pitch)) {
                notes.push_back(pitch);
            }
        }
        
        if (notes.size() >= 4) {
            // Try bass triad (first 3 notes) + upper structure
            uint16_t lowerMask = 0;
            uint16_t upperMask = 0;
            
            // Simple split: lower half vs upper half
            int mid = notes.size() / 2;
            for (int i = 0; i < mid; i++) {
                lowerMask |= (1 << notes[i]);
            }
            for (int i = mid; i < notes.size(); i++) {
                upperMask |= (1 << notes[i]);
            }
            
            // Check if both parts form valid chords
            const EnhancedChordEntry* lowerChord = findEnhancedChord(lowerMask);
            const EnhancedChordEntry* upperChord = findEnhancedChord(upperMask);
            
            if (lowerChord && upperChord) {
                DetailedChordCandidate candidate;
                candidate.name = std::string(upperChord->name) + "/" + std::string(lowerChord->name);
                candidate.mask = mask;
                candidate.confidence = (lowerChord->confidence + upperChord->confidence) / 2.0f * 0.9f;
                candidate.root = lowerChord->root;
                candidate.isInversion = false;
                candidate.inversionDegree = 0;
                candidate.interpretationType = "polychord";
                candidate.matchScore = 0.9f;
                
                // Apply key boost for polychord functionality
                candidate.confidence *= calculateKeyBoost(candidate.name, mask);
                
                candidates.push_back(candidate);
                break; // Found a valid polychord split
            }
        }
    }
    
    return candidates;
}
