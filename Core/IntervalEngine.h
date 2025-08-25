#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include "PerformanceStrategy.h"

namespace ChordLock {

struct IntervalResult {
    std::vector<int> intervals;
    int bass_note;
    int root_note;
    bool has_inversion;
    
    IntervalResult() : bass_note(-1), root_note(-1), has_inversion(false) {}
    
    bool operator==(const IntervalResult& other) const {
        return intervals == other.intervals && 
               bass_note == other.bass_note && 
               root_note == other.root_note;
    }
};

class IntervalEngine {
private:
    static constexpr int OCTAVE = 12;
    static constexpr int MAX_MIDI_NOTE = 127;
    
    // Performance optimizations
    SIMDIntervalEngine simd_engine;
    LRUChordCache<512> interval_cache;
    
    // CPU feature detection for SIMD
    bool has_avx2_support() const;
    bool has_sse4_support() const;
    
    // Core calculation methods
    std::vector<int> normalize_intervals(std::vector<int> intervals) const;
    std::vector<int> create_basic_intervals(const std::vector<int>& extended_intervals) const;
    int detect_bass_note(const std::vector<int>& notes) const;
    int detect_root_note(const std::vector<int>& notes, const std::vector<int>& intervals) const;
    bool is_inversion(const std::vector<int>& intervals, int bass_class, int root_class) const;
    
public:
    IntervalEngine();
    ~IntervalEngine() = default;
    
    // Main API methods
    IntervalResult calculateIntervals(const std::vector<int>& midi_notes) const;
    IntervalResult calculateIntervals(const std::vector<int>& midi_notes, int specified_bass) const;
    
    // Utility methods
    std::vector<int> normalizeToOctave(const std::vector<int>& midi_notes) const;
    std::vector<int> sortAndDeduplicate(std::vector<int> notes) const;
    std::vector<int> getAllRotations(const std::vector<int>& intervals) const;
    
    // Analysis methods
    bool isValidInterval(int interval) const { return interval >= 0 && interval < OCTAVE; }
    bool isValidMidiNote(int note) const { return note >= 0 && note <= MAX_MIDI_NOTE; }
    size_t getChordSpan(const std::vector<int>& midi_notes) const;
    
    // Performance methods
    void warmupCache(const std::vector<std::vector<int>>& common_patterns);
    void clearCache() { interval_cache = LRUChordCache<512>{}; }
    
    // Static utility methods
    static std::vector<int> transposeIntervals(const std::vector<int>& intervals, int semitones);
    static int getIntervalClass(int midi_note) { return midi_note % OCTAVE; }
    static int getOctave(int midi_note) { return (midi_note / OCTAVE) - 1; }
    
    // Validation
    bool validateInput(const std::vector<int>& midi_notes) const;
    
private:
    // Internal optimization methods
    std::vector<int> fast_modulo_12(const std::vector<int>& values) const;
    void apply_simd_optimization(std::vector<int>& intervals) const;
};

// Inline implementations for hot path methods
inline std::vector<int> IntervalEngine::normalizeToOctave(const std::vector<int>& midi_notes) const {
    std::vector<int> normalized;
    normalized.reserve(midi_notes.size());
    
    for (int note : midi_notes) {
        if (isValidMidiNote(note)) {
            normalized.push_back(getIntervalClass(note));
        }
    }
    
    return normalized;
}

inline std::vector<int> IntervalEngine::sortAndDeduplicate(std::vector<int> notes) const {
    if (notes.empty()) return notes;
    
    std::sort(notes.begin(), notes.end());
    notes.erase(std::unique(notes.begin(), notes.end()), notes.end());
    
    return notes;
}

inline bool IntervalEngine::validateInput(const std::vector<int>& midi_notes) const {
    if (midi_notes.empty() || midi_notes.size() > 16) return false;
    
    return std::all_of(midi_notes.begin(), midi_notes.end(), 
                      [this](int note) { return isValidMidiNote(note); });
}

inline size_t IntervalEngine::getChordSpan(const std::vector<int>& midi_notes) const {
    if (midi_notes.size() < 2) return 0;
    
    auto [min_it, max_it] = std::minmax_element(midi_notes.begin(), midi_notes.end());
    return *max_it - *min_it;
}

} // namespace ChordLock