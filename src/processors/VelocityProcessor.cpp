#include "VelocityProcessor.hpp"
#include "../utils/simd_utils.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

VelocityProcessor::VelocityProcessor(const Configuration& config) : config_(config) {
    if (!config_.isValid()) {
        throw std::invalid_argument("Invalid VelocityProcessor configuration");
    }
}

void VelocityProcessor::setConfiguration(const Configuration& config) {
    if (!config.isValid()) {
        throw std::invalid_argument("Invalid VelocityProcessor configuration");
    }
    config_ = config;
}

VelocityProcessor::VelocityWeights VelocityProcessor::processNotes(
    const bool noteStates[128], 
    const uint8_t velocities[128]) const {
    
    VelocityWeights result;
    
    if (!config_.velocitySensitive) {
        // Simple mode: all active notes with equal weight
        result.harmonicMask = buildSimpleMask(noteStates);
        result.melodicMask = 0;
        
        for (int i = 0; i < 128; i++) {
            if (noteStates[i]) {
                int pitchClass = i % 12;
                result.weights[pitchClass] = 1.0f;
                result.totalHarmonicWeight += 1.0f;
            }
        }
        return result;
    }
    
    // Advanced velocity-sensitive processing
    for (int i = 0; i < 128; i++) {
        if (!noteStates[i]) continue;
        
        int pitchClass = i % 12;
        uint8_t velocity = velocities[i];
        NoteRole role = classifyNote(velocity, i);
        
        // Calculate base weight from velocity
        float velocityWeight = velocity / 127.0f;
        
        // Apply role-specific processing
        switch (role) {
            case NoteRole::HARMONY:
                result.harmonicMask |= (1 << pitchClass);
                result.weights[pitchClass] += velocityWeight * config_.harmonyBoostFactor;
                result.totalHarmonicWeight += velocityWeight * config_.harmonyBoostFactor;
                break;
                
            case NoteRole::BASS:
                result.harmonicMask |= (1 << pitchClass);
                result.weights[pitchClass] += velocityWeight * config_.bassBoostFactor;
                result.totalHarmonicWeight += velocityWeight * config_.bassBoostFactor;
                break;
                
            case NoteRole::MIXED:
                // Include in harmony but with reduced weight
                if (config_.harmonyFilterEnabled) {
                    result.harmonicMask |= (1 << pitchClass);
                    result.weights[pitchClass] += velocityWeight * 0.7f;
                    result.totalHarmonicWeight += velocityWeight * 0.7f;
                } else {
                    result.harmonicMask |= (1 << pitchClass);
                    result.weights[pitchClass] += velocityWeight;
                    result.totalHarmonicWeight += velocityWeight;
                }
                break;
                
            case NoteRole::MELODY:
                result.melodicMask |= (1 << pitchClass);
                result.totalMelodicWeight += velocityWeight;
                
                // Only include melody in harmony if filtering is disabled
                if (!config_.harmonyFilterEnabled) {
                    result.harmonicMask |= (1 << pitchClass);
                    result.weights[pitchClass] += velocityWeight * 0.3f;  // Reduced weight
                }
                break;
        }
    }
    
    // Normalize weights if we have harmonic content
    if (result.totalHarmonicWeight > 0.0f) {
        for (int i = 0; i < 12; i++) {
            if (result.weights[i] > 0.0f) {
                result.weights[i] /= result.totalHarmonicWeight;
            }
        }
    }
    
    return result;
}

VelocityProcessor::NoteRole VelocityProcessor::classifyNote(uint8_t velocity, int midiNote) const {
    // Bass register classification (below C3)
    if (isInBassRegister(midiNote)) {
        return NoteRole::BASS;
    }
    
    // Velocity-based classification
    if (velocity >= config_.melodyThreshold) {
        return NoteRole::MELODY;
    } else if (velocity <= config_.padThreshold) {
        return NoteRole::HARMONY;
    } else {
        // Mixed classification can be influenced by register
        return isInMelodyRegister(midiNote) ? NoteRole::MELODY : NoteRole::MIXED;
    }
}

uint16_t VelocityProcessor::buildSimpleMask(const bool noteStates[128]) const {
    uint16_t mask = 0;
    for (int i = 0; i < 128; i++) {
        if (noteStates[i]) {
            mask |= (1 << (i % 12));
        }
    }
    return mask;
}

uint16_t VelocityProcessor::buildHarmonicMask(const bool noteStates[128], const uint8_t velocities[128]) const {
    return processNotes(noteStates, velocities).harmonicMask;
}

float VelocityProcessor::calculateBassWeight(int midiNote) const {
    if (!config_.bassWeightEnabled) return 1.0f;
    
    // Exponential decay from bass register
    float normalizedNote = midiNote / 127.0f;
    return 1.0f + (1.0f - normalizedNote) * (config_.bassBoostFactor - 1.0f);
}

float VelocityProcessor::calculateHarmonyWeight(uint8_t velocity) const {
    // Bell curve around moderate velocities (60-80)
    float normalizedVel = velocity / 127.0f;
    float target = 0.55f;  // Target around velocity 70
    float distance = std::abs(normalizedVel - target);
    return std::max(0.1f, 1.0f - distance / 0.4f) * config_.harmonyBoostFactor;
}

float VelocityProcessor::calculateMelodyPenalty(uint8_t velocity) const {
    if (velocity >= config_.melodyThreshold) {
        // Strong penalty for high velocity notes in harmony detection
        return 0.1f;
    }
    return 1.0f;
}

VelocityProcessor::AnalysisResult VelocityProcessor::analyzeVelocityDistribution(
    const bool noteStates[128], 
    const uint8_t velocities[128]) const {
    
    AnalysisResult result;
    result.harmonicNoteCount = 0;
    result.melodicNoteCount = 0;
    result.averageHarmonyVelocity = 0.0f;
    result.averageMelodyVelocity = 0.0f;
    result.lowestNote = 127;
    result.highestNote = 0;
    result.dynamicRange = 0.0f;
    
    float harmonyVelSum = 0.0f;
    float melodyVelSum = 0.0f;
    uint8_t minVel = 127, maxVel = 0;
    
    for (int i = 0; i < 128; i++) {
        if (!noteStates[i]) continue;
        
        uint8_t velocity = velocities[i];
        NoteRole role = classifyNote(velocity, i);
        
        // Update note range
        result.lowestNote = std::min(result.lowestNote, i);
        result.highestNote = std::max(result.highestNote, i);
        
        // Update velocity range
        minVel = std::min(minVel, velocity);
        maxVel = std::max(maxVel, velocity);
        
        // Classify and accumulate
        if (role == NoteRole::HARMONY || role == NoteRole::BASS || role == NoteRole::MIXED) {
            result.harmonicNoteCount++;
            harmonyVelSum += velocity;
        }
        
        if (role == NoteRole::MELODY) {
            result.melodicNoteCount++;
            melodyVelSum += velocity;
        }
    }
    
    // Calculate averages
    if (result.harmonicNoteCount > 0) {
        result.averageHarmonyVelocity = harmonyVelSum / result.harmonicNoteCount;
    }
    
    if (result.melodicNoteCount > 0) {
        result.averageMelodyVelocity = melodyVelSum / result.melodicNoteCount;
    }
    
    result.dynamicRange = maxVel - minVel;
    
    return result;
}

float VelocityProcessor::computeRegisterWeight(int midiNote) const {
    // Weight based on musical register importance
    if (midiNote < 48) {        // Below C3 - bass register
        return config_.bassBoostFactor;
    } else if (midiNote < 72) { // C3-C5 - harmony register
        return config_.harmonyBoostFactor;
    } else {                    // Above C5 - melody register
        return 0.8f;            // Slight reduction for very high notes
    }
}

bool VelocityProcessor::isInBassRegister(int midiNote) const {
    return midiNote < 48;  // Below C3
}

bool VelocityProcessor::isInMelodyRegister(int midiNote) const {
    return midiNote >= 72;  // C5 and above
}