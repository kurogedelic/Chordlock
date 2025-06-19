#pragma once
#include "../chord_types.hpp"
#include <vector>
#include <memory>

/**
 * Abstract interface for chord detection engines
 * Enables swapping between different detection algorithms
 */
class IChordDetectionEngine {
public:
    virtual ~IChordDetectionEngine() = default;
    
    // Core detection methods
    virtual ChordCandidate detectBest(uint32_t timestamp = 0) = 0;
    virtual std::vector<ChordCandidate> detectAlternatives(int maxResults = 3, uint32_t timestamp = 0) = 0;
    
    // Note input methods
    virtual void noteOn(uint8_t note, uint32_t timestamp = 0) = 0;
    virtual void noteOff(uint8_t note) = 0;
    virtual void setVelocity(uint8_t note, uint8_t velocity) = 0;
    virtual void reset() = 0;
    
    // Configuration
    virtual void setVelocitySensitivity(bool enabled) = 0;
    virtual void setSlashChordDetection(bool enabled) = 0;
    virtual void setKey(int key) = 0;
    
    // Getters
    virtual bool getVelocitySensitivity() const = 0;
    virtual bool getSlashChordDetection() const = 0;
    virtual int getKey() const = 0;
    
    // Utility
    virtual std::string getEngineName() const = 0;
    virtual std::string getEngineVersion() const = 0;
};

/**
 * Extended interface for advanced detection engines with probabilistic output
 */
class IAdvancedChordDetectionEngine : public IChordDetectionEngine {
public:
    struct ProbabilisticResult {
        struct WeightedCandidate {
            std::string name;
            uint16_t mask;
            float probability;
            float confidence;
            int root;
            int bass;
            
            WeightedCandidate() : mask(0), probability(0.0f), confidence(0.0f), root(-1), bass(-1) {}
            WeightedCandidate(const std::string& n, uint16_t m, float p, float c, int r, int b = -1)
                : name(n), mask(m), probability(p), confidence(c), root(r), bass(b) {}
        };
        
        std::vector<WeightedCandidate> candidates;
        float ambiguityIndex;
        bool isAmbiguous;
        
        ProbabilisticResult() : ambiguityIndex(0.0f), isAmbiguous(false) {}
        
        WeightedCandidate getBest() const {
            return candidates.empty() ? WeightedCandidate() : candidates[0];
        }
        
        std::vector<WeightedCandidate> getTopN(int n, float threshold = 0.1f) const;
    };
    
    // Advanced detection methods
    virtual ProbabilisticResult detectProbabilistic(uint32_t timestamp = 0) = 0;
    
    // Advanced configuration
    virtual void setMelodyVelocityThreshold(uint8_t threshold) = 0;
    virtual void setPadVelocityThreshold(uint8_t threshold) = 0;
    virtual void setHarmonyFilterEnabled(bool enabled) = 0;
    virtual void setBassWeightEnabled(bool enabled) = 0;
    
    // Performance metrics
    virtual uint64_t getTotalDetections() const = 0;
    virtual double getAverageDetectionTimeMs() const = 0;
    virtual void resetPerformanceCounters() = 0;
};