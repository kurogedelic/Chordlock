#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <optional>

namespace ChordLock {

enum class InversionType {
    ROOT_POSITION,      // 0th inversion (root in bass)
    FIRST_INVERSION,    // 1st inversion (3rd in bass)  
    SECOND_INVERSION,   // 2nd inversion (5th in bass)
    THIRD_INVERSION,    // 3rd inversion (7th in bass)
    HIGHER_INVERSION,   // 4th+ inversion (9th, 11th, 13th in bass)
    SLASH_CHORD,        // Non-chord tone in bass
    UNKNOWN
};

struct InversionInfo {
    InversionType type;
    int bass_interval;          // Interval of bass note from root (0-11)
    int root_interval;          // What should be the root interval
    std::vector<int> root_position_intervals;  // Intervals in root position
    std::string inversion_symbol;  // "/" for slash, "b", "c", "d" for inversions
    float confidence;           // How confident we are (0.0-1.0)
    
    InversionInfo() : type(InversionType::UNKNOWN), bass_interval(-1), 
                     root_interval(-1), confidence(0.0f) {}
};

class InversionDetector {
private:
    // Precomputed inversion patterns for common chord types
    struct InversionPattern {
        std::vector<int> root_position;
        std::array<std::vector<int>, 4> inversions;  // Up to 4 inversions
        std::array<std::string, 4> symbols;
    };
    
    std::unordered_map<std::string, InversionPattern> inversion_patterns;
    
    // Performance optimization: precomputed rotation tables
    using IntervalVector = std::vector<int>;
    using RotationKey = std::pair<IntervalVector, int>;  // (intervals, rotation)
    
    struct RotationKeyHash {
        std::size_t operator()(const RotationKey& key) const {
            // Custom hash for vector<int>
            std::size_t h1 = 0;
            for (int val : key.first) {
                h1 ^= std::hash<int>{}(val) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
            }
            std::size_t h2 = std::hash<int>{}(key.second);
            return h1 ^ (h2 << 1);
        }
    };
    
    std::unordered_map<RotationKey, IntervalVector, RotationKeyHash> rotation_cache;
    
    // Core analysis methods
    std::vector<int> rotateIntervals(const std::vector<int>& intervals, int positions) const;
    std::vector<std::vector<int>> generateAllRotations(const std::vector<int>& intervals) const;
    
    // Pattern matching
    std::optional<InversionInfo> matchKnownPattern(const std::vector<int>& intervals) const;
    InversionInfo analyzeTriadInversion(const std::vector<int>& intervals) const;
    InversionInfo analyzeSeventhInversion(const std::vector<int>& intervals) const;
    InversionInfo analyzeExtendedInversion(const std::vector<int>& intervals) const;
    
    // Bass note analysis
    bool isChordTone(int bass_interval, const std::vector<int>& chord_intervals) const;
    int findBestRootCandidate(const std::vector<int>& intervals) const;
    
    // Confidence calculation
    float calculateInversionConfidence(const InversionInfo& info, const std::vector<int>& original_intervals) const;
    
    // Static pattern definitions
    void initializeCommonPatterns();
    
public:
    InversionDetector();
    ~InversionDetector() = default;
    
    // Primary detection methods
    InversionInfo detectInversion(const std::vector<int>& intervals) const;
    InversionInfo detectInversion(const std::vector<int>& intervals, int bass_note_class) const;
    
    // Multiple candidate analysis
    std::vector<InversionInfo> findAllPossibleInversions(const std::vector<int>& intervals) const;
    std::vector<InversionInfo> findBestInversions(const std::vector<int>& intervals, size_t max_results = 3) const;
    
    // Conversion methods
    std::vector<int> convertToRootPosition(const std::vector<int>& intervals) const;
    std::vector<int> convertToInversion(const std::vector<int>& root_intervals, InversionType target) const;
    std::vector<int> convertToSpecificInversion(const std::vector<int>& root_intervals, int bass_interval) const;
    
    // Utility methods
    bool isInversion(const std::vector<int>& intervals) const;
    bool isSlashChord(const std::vector<int>& intervals, int bass_note_class) const;
    InversionType getInversionType(const std::vector<int>& intervals) const;
    std::string getInversionSymbol(InversionType type) const;
    std::string getInversionName(InversionType type) const;
    
    // Pattern management
    void addCustomPattern(const std::string& chord_name, const std::vector<int>& root_position);
    bool hasPattern(const std::string& chord_name) const;
    void clearCustomPatterns();
    
    // Performance optimization
    void warmupCache(const std::vector<std::vector<int>>& common_intervals);
    void clearCache() { rotation_cache.clear(); }
    size_t getCacheSize() const { return rotation_cache.size(); }
    
    // Validation and debugging
    bool validateInversionInfo(const InversionInfo& info) const;
    std::string debugInversionAnalysis(const std::vector<int>& intervals) const;
    
private:
    // Static data for common chord inversions
    static constexpr std::array<std::array<int, 3>, 3> MAJOR_TRIAD_INVERSIONS = {{
        {{0, 4, 7}},    // Root position
        {{0, 3, 8}},    // First inversion (C/E: E-G-C)
        {{0, 5, 9}}     // Second inversion (C/G: G-C-E)
    }};
    
    static constexpr std::array<std::array<int, 3>, 3> MINOR_TRIAD_INVERSIONS = {{
        {{0, 3, 7}},    // Root position
        {{0, 4, 9}},    // First inversion (Cm/Eb: Eb-G-C)
        {{0, 5, 8}}     // Second inversion (Cm/G: G-C-Eb)
    }};
    
    static constexpr std::array<std::array<int, 4>, 4> DOM7_INVERSIONS = {{
        {{0, 4, 7, 10}}, // Root position
        {{0, 3, 6, 8}},  // First inversion (/3)
        {{0, 3, 5, 9}},  // Second inversion (/5)
        {{0, 2, 6, 9}}   // Third inversion (/b7)
    }};
    
    // Quick lookup for inversion symbols
    static constexpr std::array<const char*, 8> INVERSION_SYMBOLS = {
        "",      // ROOT_POSITION
        "b",     // FIRST_INVERSION  
        "c",     // SECOND_INVERSION
        "d",     // THIRD_INVERSION
        "e",     // HIGHER_INVERSION (simplified)
        "/",     // SLASH_CHORD
        "?",     // UNKNOWN
        ""
    };
    
    static constexpr std::array<const char*, 8> INVERSION_NAMES = {
        "root position",
        "first inversion",
        "second inversion", 
        "third inversion",
        "higher inversion",
        "slash chord",
        "unknown",
        ""
    };
};

// Inline implementations for performance
inline bool InversionDetector::isInversion(const std::vector<int>& intervals) const {
    if (intervals.empty() || intervals[0] == 0) return false;
    return detectInversion(intervals).type != InversionType::ROOT_POSITION;
}

inline std::string InversionDetector::getInversionSymbol(InversionType type) const {
    return INVERSION_SYMBOLS[static_cast<int>(type)];
}

inline std::string InversionDetector::getInversionName(InversionType type) const {
    return INVERSION_NAMES[static_cast<int>(type)];
}

inline bool InversionDetector::isChordTone(int bass_interval, const std::vector<int>& chord_intervals) const {
    return std::find(chord_intervals.begin(), chord_intervals.end(), bass_interval) != chord_intervals.end();
}

inline bool InversionDetector::isSlashChord(const std::vector<int>& intervals, int bass_note_class) const {
    return !isChordTone(bass_note_class, intervals);
}

} // namespace ChordLock