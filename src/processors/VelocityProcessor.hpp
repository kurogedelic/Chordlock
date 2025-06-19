#pragma once
#include <cstdint>
#include <array>

/**
 * Handles velocity-based note analysis and filtering
 * Separates melody from harmony based on velocity patterns
 */
class VelocityProcessor {
public:
    enum class NoteRole {
        HARMONY,    // Low velocity, likely chord tone
        MELODY,     // High velocity, likely melody note  
        MIXED,      // Medium velocity, could be either
        BASS        // Low register, likely bass note
    };
    
    struct VelocityWeights {
        uint16_t harmonicMask;      // Notes considered harmonic
        uint16_t melodicMask;       // Notes considered melodic
        float weights[12];          // Weight for each pitch class
        float totalHarmonicWeight;  // Sum of harmonic weights
        float totalMelodicWeight;   // Sum of melodic weights
        
        VelocityWeights() : harmonicMask(0), melodicMask(0), totalHarmonicWeight(0.0f), totalMelodicWeight(0.0f) {
            for (int i = 0; i < 12; i++) weights[i] = 0.0f;
        }
    };
    
    struct Configuration {
        bool velocitySensitive;
        bool harmonyFilterEnabled;
        bool bassWeightEnabled;
        uint8_t melodyThreshold;
        uint8_t padThreshold;
        float bassBoostFactor;
        float harmonyBoostFactor;
        
        Configuration() : velocitySensitive(false), harmonyFilterEnabled(true),
                         bassWeightEnabled(true), melodyThreshold(100),
                         padThreshold(70), bassBoostFactor(1.5f),
                         harmonyBoostFactor(1.2f) {}
        
        // Validate configuration
        bool isValid() const {
            return melodyThreshold > padThreshold && 
                   bassBoostFactor >= 1.0f && 
                   harmonyBoostFactor >= 1.0f;
        }
    };
    
public:
    explicit VelocityProcessor(const Configuration& config = Configuration());
    
    // Configuration management
    void setConfiguration(const Configuration& config);
    const Configuration& getConfiguration() const { return config_; }
    
    // Core processing methods
    VelocityWeights processNotes(const bool noteStates[128], const uint8_t velocities[128]) const;
    NoteRole classifyNote(uint8_t velocity, int midiNote) const;
    
    // Utility methods
    uint16_t buildSimpleMask(const bool noteStates[128]) const;
    uint16_t buildHarmonicMask(const bool noteStates[128], const uint8_t velocities[128]) const;
    
    // Weight calculation strategies
    float calculateBassWeight(int midiNote) const;
    float calculateHarmonyWeight(uint8_t velocity) const;
    float calculateMelodyPenalty(uint8_t velocity) const;
    
    // Analysis methods
    struct AnalysisResult {
        int harmonicNoteCount;
        int melodicNoteCount;
        float averageHarmonyVelocity;
        float averageMelodyVelocity;
        int lowestNote;
        int highestNote;
        float dynamicRange;
    };
    
    AnalysisResult analyzeVelocityDistribution(const bool noteStates[128], const uint8_t velocities[128]) const;
    
private:
    Configuration config_;
    
    // Helper methods
    float computeRegisterWeight(int midiNote) const;
    bool isInBassRegister(int midiNote) const;
    bool isInMelodyRegister(int midiNote) const;
};