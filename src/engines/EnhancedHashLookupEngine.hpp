#pragma once

#include "../interfaces/IChordDetectionEngine.hpp"
#include "../processors/VelocityProcessor.hpp"
#include "../enhanced_chord_hash_table.hpp"
#include "../chord_types.hpp"
#include <vector>
#include <string>
#include <algorithm>

/**
 * @brief Enhanced chord detection engine using the improved hash table
 * 
 * This engine uses the enhanced hash table generated from the fixed corpus,
 * which includes properly repaired Dominant 11th chords and extended harmonies.
 */
class EnhancedHashLookupEngine : public IChordDetectionEngine {
public:
    struct DetailedChordCandidate {
        std::string name;
        uint16_t mask;
        float confidence;
        std::string root;
        bool isInversion;
        int inversionDegree;      // 0=root position, 1=1st inversion, etc.
        std::vector<int> missingNotes;  // Expected notes not present
        std::vector<int> extraNotes;    // Present notes not expected
        std::string interpretationType; // "exact", "subset", "superset", "inversion", "transposition"
        float matchScore;         // Percentage of note matching
    };

    struct EnhancedLookupResult {
        std::vector<ChordCandidate> candidates;
        std::vector<DetailedChordCandidate> detailedCandidates;
        float averageConfidence;
        bool hasResults() const { return !candidates.empty(); }
        const ChordCandidate& getBest() const { return candidates[0]; }
        const DetailedChordCandidate& getBestDetailed() const { return detailedCandidates[0]; }
    };

private:
    VelocityProcessor velocityProcessor_;
    bool noteStates_[128] = {false};
    uint8_t velocities_[128] = {0};
    bool velocitySensitive_ = true;
    bool slashChordDetection_ = true;
    int currentKey_ = 0;
    
public:
    EnhancedHashLookupEngine();
    ~EnhancedHashLookupEngine() = default;
    
    // IChordDetectionEngine interface
    ChordCandidate detectBest(uint32_t timestamp = 0) override;
    std::vector<ChordCandidate> detectAlternatives(int maxResults = 3, uint32_t timestamp = 0) override;
    
    void noteOn(uint8_t note, uint32_t timestamp = 0) override;
    void noteOff(uint8_t note) override;
    void setVelocity(uint8_t note, uint8_t velocity) override;
    void reset() override;
    
    void setVelocitySensitivity(bool enabled) override;
    void setSlashChordDetection(bool enabled) override;
    void setKey(int key) override;
    
    bool getVelocitySensitivity() const override;
    bool getSlashChordDetection() const override;
    int getKey() const override;
    
    std::string getEngineName() const override;
    std::string getEngineVersion() const override;
    
    // Enhanced specific methods
    EnhancedLookupResult lookupChordEnhanced(const VelocityProcessor::VelocityWeights& weights) const;
    EnhancedLookupResult lookupChordDirect(uint16_t mask) const;
    EnhancedLookupResult lookupChordWithDetailedAnalysis(uint16_t mask, int maxResults = 5) const;
    
    // Helper methods for testing
    void setChordFromMIDI(const std::vector<int>& midiNotes, uint8_t baseVelocity = 80);
    uint16_t getCurrentMask() const;
    
private:
    uint16_t calculateMask(const VelocityProcessor::VelocityWeights& weights) const;
    uint16_t calculateDirectMask() const;
    std::vector<ChordCandidate> findAlternativeChords(uint16_t mask, int maxResults) const;
    std::vector<DetailedChordCandidate> findDetailedAlternatives(uint16_t mask, int maxResults) const;
    
    // Advanced analysis methods
    DetailedChordCandidate analyzeInversions(uint16_t mask, const EnhancedChordEntry* entry) const;
    std::vector<DetailedChordCandidate> findEnharmonicEquivalents(uint16_t mask) const;
    float calculateMatchScore(uint16_t inputMask, uint16_t chordMask) const;
    int detectInversionDegree(uint16_t mask, const std::string& chordName) const;
    std::vector<int> findMissingNotes(uint16_t inputMask, uint16_t expectedMask) const;
    std::vector<int> findExtraNotes(uint16_t inputMask, uint16_t expectedMask) const;
    
    float calculateConfidenceBoost(const EnhancedChordEntry* entry, const VelocityProcessor::VelocityWeights& weights) const;
    std::string getNoteFromPitch(int pitch) const;
    
    // Enhanced bass-aware analysis
    int findLowestNote() const;
    int findHighestNote() const;
    std::string formatNoteName(int midiNote) const;
    uint16_t removeLowestNoteFromMask(uint16_t mask, int lowestNote) const;
    DetailedChordCandidate createSlashChordCandidate(const EnhancedChordEntry* upperChord, int bassNote, float baseConfidence) const;
};