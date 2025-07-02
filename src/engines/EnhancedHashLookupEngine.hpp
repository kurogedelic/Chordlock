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
// Forward declaration for nested class
class EnhancedHashLookupEngine;

// ROOT ESTIMATION: Smart root detection for symmetric chords
class RootEstimator {
public:
    struct RootCandidate {
        int root;
        float confidence;
        std::string reason;
    };
    
    // Main estimation method
    int estimateRoot(uint16_t mask, int bassNote) const;
    
    // Get all root candidates with confidence scores
    std::vector<RootCandidate> getAllRootCandidates(uint16_t mask, int bassNote) const;
    
private:
    // Music theory based estimation
    int estimateByLowestNote(int bassNote) const;
    int estimateByIntervalStructure(uint16_t mask) const;
    int estimateByStatisticalFrequency(uint16_t mask) const;
    
    // Check for strong root indicators
    bool hasStrongRootIndicators(uint16_t mask, int root) const;
    bool hasPerfectFifth(uint16_t mask, int root) const;
    bool hasMajorThird(uint16_t mask, int root) const;
    bool hasMinorThird(uint16_t mask, int root) const;
};

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
        ChordExtensions extensions; // Add/altered/sus tones
        
        std::string getDisplayName() const {
            if (extensions.hasExtensions()) {
                return name + extensions.formatExtensions();
            }
            return name;
        }
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
    KeyContext keyContext_; // Key-aware analysis
    RootEstimator rootEstimator_; // Smart root detection
    
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
    
    // Key context methods
    void setKeyContext(const KeyContext& key);
    void setKeyContext(int tonic, bool isMinor = false);
    void clearKeyContext();
    const KeyContext& getKeyContext() const;
    
    bool getVelocitySensitivity() const override;
    bool getSlashChordDetection() const override;
    int getKey() const override;
    
    std::string getEngineName() const override;
    std::string getEngineVersion() const override;
    
    // Enhanced specific methods
    EnhancedLookupResult lookupChordEnhanced(const VelocityProcessor::VelocityWeights& weights) const;
    EnhancedLookupResult lookupChordDirect(uint16_t mask) const;
    EnhancedLookupResult lookupChordWithDetailedAnalysis(uint16_t mask, int maxResults = 5) const;
    
    // Ambiguous set handling
    struct AmbiguousSet {
        uint16_t mask;
        std::vector<std::string> equivalentChords;
        std::string getLabel(int bassPitchClass) const;
    };
    
    std::vector<AmbiguousSet> getAmbiguousSets() const;
    bool isAmbiguousSet(uint16_t mask, AmbiguousSet& result) const;
    
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
    int findLowestNoteWithVelocity(uint8_t minVelocity = 20) const;
    int findNextLowestNote() const;
    int findHighestNote() const;
    int getLowestNotePitch() const; // New: get lowest note pitch class
    std::string formatNoteName(int midiNote) const;
    uint16_t removeLowestNoteFromMask(uint16_t mask, int lowestNote) const;
    DetailedChordCandidate createSlashChordCandidate(const EnhancedChordEntry* upperChord, int bassNote, float baseConfidence) const;
    std::string analyzeNaturalSlashChord(uint16_t mask, int bassPitch, bool& isNaturalSlashChord) const;
    
    // PRIORITY DETECTION: Critical chord patterns detected first
    ChordCandidate detectHalfDiminished(uint16_t mask) const;
    
    // Extended chord template generation
    ChordCandidate generateExtendedChordTemplate(uint16_t mask, const std::string& chordName) const;
    std::vector<ChordCandidate> generateAllExtendedTemplates(uint16_t mask) const;
    
    // Score normalization utilities
    void normalizeCandidateScores(std::vector<ChordCandidate>& candidates) const;
    void normalizeDetailedCandidateScores(std::vector<DetailedChordCandidate>& candidates) const;
    
    // Universal slash chord scoring (scale-independent)
    float calculateUniversalSlashScore(uint16_t mask, int rootPitch, int bassPitch) const;
    
    // Comprehensive slash candidate generation
    std::vector<ChordCandidate> generateAllSlashCandidates(uint16_t mask) const;
    
    // 6th chord detection
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> detect6thChords(uint16_t mask) const;
    
    // sus2/sus4 chord detection
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> detectSusChords(uint16_t mask) const;
    
    // augmented chord detection
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> detectAugmentedChords(uint16_t mask) const;
    
    // diminished 7th chord detection
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> detectDiminishedSeventhChords(uint16_t mask) const;
    
    // chord extension analysis
    ChordExtensions analyzeChordExtensions(uint16_t mask, const std::string& baseChordName) const;
    
    // altered 7th chord detection
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> detectAlteredSeventhChords(uint16_t mask) const;
    
    // key-aware analysis
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> detectExtendedChords(uint16_t mask) const;
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> analyzeRootlessChords(uint16_t mask) const;
    std::vector<EnhancedHashLookupEngine::DetailedChordCandidate> analyzePolychords(uint16_t mask) const;
    float calculateKeyBoost(const std::string& chordName, uint16_t mask) const;
};