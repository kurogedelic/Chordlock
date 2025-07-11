#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include "PerformanceStrategy.h"
#ifdef USE_COMPILED_TABLES
#include "../Data/CompiledTables.h"
#endif

namespace ChordLock {

struct ChordInfo {
    std::string name;
    std::string category;
    std::vector<std::string> aliases;
    std::vector<int> intervals;
    float quality_score;  // 0.0-1.0, how "common" this chord is
    bool requires_bass;   // Does this chord require specific bass note?
    
    ChordInfo() : quality_score(0.5f), requires_bass(false) {}
    
    ChordInfo(const std::string& chord_name, const std::vector<int>& chord_intervals)
        : name(chord_name), intervals(chord_intervals), quality_score(0.5f), requires_bass(false) {}
};

struct ChordMatch {
    ChordInfo chord_info;
    float confidence;     // 0.0-1.0, how confident we are in this match
    bool is_inversion;    // Is this an inversion of the base chord?
    int bass_interval;    // Bass note interval (-1 if no specific bass)
    std::vector<int> missing_notes;  // Expected notes that are missing
    std::vector<int> extra_notes;    // Notes that don't belong to the chord
    
    // Enhanced slash chord support
    int bass_note_midi;   // Actual MIDI note of bass (-1 if no bass)
    int root_note_midi;   // Actual MIDI note of theoretical root (-1 if unknown)
    bool is_slash_chord;  // True if bass != root (requires /bass notation)
    std::string bass_note_name;  // Bass note name for slash notation (e.g., "E" for C/E)
    int inversion_type;   // 0=root position, 1=1st inversion, 2=2nd inversion, etc.
    
    ChordMatch() : confidence(0.0f), is_inversion(false), bass_interval(-1), 
                   bass_note_midi(-1), root_note_midi(-1), is_slash_chord(false), 
                   inversion_type(0) {}
    
    bool operator>(const ChordMatch& other) const {
        return confidence > other.confidence;
    }
};

class ChordDatabase {
private:
    // Primary storage
    std::unordered_map<std::vector<int>, std::string, ChordHashMap::IntervalHash> main_chord_map;
    std::unordered_map<std::string, ChordInfo> chord_info_map;
    
    // Alias resolution
    std::unordered_map<std::string, std::string> alias_to_canonical;
    std::unordered_map<std::string, std::vector<std::string>> canonical_to_aliases;
    
    // Performance optimizations
    BloomFilter known_intervals_filter;
    LRUChordCache<1024> lookup_cache;
    
    // Inversion and rotation tables
    std::unordered_map<std::vector<int>, std::vector<std::vector<int>>, ChordHashMap::IntervalHash> inversion_table;
    std::unordered_map<std::vector<int>, std::string, ChordHashMap::IntervalHash> inversion_to_root;
    
    // Statistical data for quality scoring
    std::unordered_map<std::string, float> chord_frequency_map;
    
    // Internal methods
    void buildInversionTables();
    void buildAliasResolution();
    void calculateQualityScores();
    void populateBloomFilter();
    
    // YAML parsing helpers
    bool parseYamlFile(const std::string& filepath);
    ChordInfo parseChordEntry(const std::string& intervals_str, const std::string& chord_name);
    std::vector<int> parseIntervalString(const std::string& intervals_str);
    
    // Fuzzy matching helpers
    float calculateSimilarity(const std::vector<int>& a, const std::vector<int>& b) const;
    std::vector<ChordMatch> findFuzzyMatches(const std::vector<int>& intervals, float min_confidence = 0.3f) const;
    
    // Wide voicing helpers
    std::vector<int> createBasicIntervals(const std::vector<int>& extended_intervals) const;
    std::optional<ChordMatch> findExactMatchInternal(const std::vector<int>& intervals) const;
    
    // Priority calculation
    float calculateChordPriority(const std::string& chord_name, size_t interval_count) const;
    
public:
    ChordDatabase();
    ~ChordDatabase() = default;
    
    // Initialization
    bool loadFromYaml(const std::string& filepath);
    bool loadFromYaml(const std::string& chord_dict_path, const std::string& aliases_path);
    void addChord(const std::string& name, const std::vector<int>& intervals);
    void addAlias(const std::string& canonical_name, const std::string& alias);
    
    // Primary lookup methods
    std::optional<ChordMatch> findExactMatch(const std::vector<int>& intervals) const;
    std::vector<ChordMatch> findMatches(const std::vector<int>& intervals, bool include_inversions = true) const;
    std::vector<ChordMatch> findBestMatches(const std::vector<int>& intervals, size_t max_results = 5) const;
    
    // Advanced lookup methods
    std::optional<ChordMatch> findWithInversion(const std::vector<int>& intervals) const;
    std::vector<ChordMatch> findWithTensions(const std::vector<int>& intervals) const;
    std::vector<ChordMatch> findWithOmissions(const std::vector<int>& intervals) const;
    
    // Alias resolution
    std::string resolveAlias(const std::string& chord_name) const;
    std::vector<std::string> getAliases(const std::string& canonical_name) const;
    std::string getPreferredAlias(const std::string& canonical_name, const std::string& style = "standard") const;
    
    // Utility methods
    bool hasChord(const std::vector<int>& intervals) const;
    bool hasChord(const std::string& chord_name) const;
    size_t getChordCount() const { return chord_info_map.size(); }
    std::vector<std::string> getAllChordNames() const;
    std::vector<std::string> getChordsByCategory(const std::string& category) const;
    
    // Performance monitoring
    void warmupCache(const std::vector<std::vector<int>>& common_patterns);
    void clearCaches();
    size_t getCacheHitRate() const;
    
    // Quality scoring
    float getChordQuality(const std::string& chord_name) const;
    float getChordFrequency(const std::string& chord_name) const;
    void updateFrequencyData(const std::string& chord_name);
    
    // Debug and validation
    bool validateDatabase() const;
    void printStatistics() const;
    std::vector<std::string> findConflicts() const;
    
private:
#ifdef USE_COMPILED_TABLES
    void loadCompiledChords();
#endif
    
private:
    // Static data for common chords (compile-time optimization)
    static constexpr std::array<std::pair<std::array<int, 3>, const char*>, 16> COMMON_TRIADS = {{
        {{{0, 4, 7}}, "major-triad"},
        {{{0, 3, 7}}, "minor-triad"},
        {{{0, 3, 6}}, "diminished-triad"},
        {{{0, 4, 8}}, "augmented-triad"},
        {{{0, 5, 7}}, "sus4-triad"},
        {{{0, 2, 7}}, "sus2-triad"},
        // ... more common triads
    }};
    
    static constexpr std::array<std::pair<std::array<int, 4>, const char*>, 16> COMMON_SEVENTHS = {{
        {{{0, 4, 7, 10}}, "dominant-seventh"},
        {{{0, 4, 7, 11}}, "major-seventh"},
        {{{0, 3, 7, 10}}, "minor-seventh"},
        {{{0, 3, 6, 9}}, "diminished-seventh"},
        {{{0, 3, 6, 10}}, "half-diminished-seventh"},
        {{{0, 3, 7, 11}}, "minor-major-seventh"},
        // ... more common sevenths
    }};
    
    // Fast lookup for common patterns
    std::optional<std::string> fastCommonLookup(const std::vector<int>& intervals) const;
};

// Inline implementations for performance-critical methods
inline bool ChordDatabase::hasChord(const std::vector<int>& intervals) const {
    // Quick bloom filter check first
    if (!known_intervals_filter.mayContain(intervals)) {
        return false;
    }
    
    // Check cache
    if (auto* cached = lookup_cache.get(intervals)) {
        return !cached->empty();
    }
    
    return main_chord_map.find(intervals) != main_chord_map.end();
}

inline std::optional<std::string> ChordDatabase::fastCommonLookup(const std::vector<int>& intervals) const {
    // Ultra-fast lookup for most common chords
    if (intervals.size() == 3) {
        for (const auto& [pattern, name] : COMMON_TRIADS) {
            if (std::equal(intervals.begin(), intervals.end(), pattern.begin())) {
                return std::string(name);
            }
        }
    } else if (intervals.size() == 4) {
        for (const auto& [pattern, name] : COMMON_SEVENTHS) {
            if (std::equal(intervals.begin(), intervals.end(), pattern.begin())) {
                return std::string(name);
            }
        }
    }
    
    return std::nullopt;
}

} // namespace ChordLock