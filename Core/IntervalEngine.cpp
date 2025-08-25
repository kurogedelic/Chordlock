#include "IntervalEngine.h"
#include <algorithm>
#include <set>
#ifdef __x86_64__
#include <cpuid.h>
#elif defined(__arm64__) || defined(__aarch64__)
#include <sys/sysctl.h>
#endif

namespace ChordLock {

IntervalEngine::IntervalEngine() {
    // Initialize with default configuration
}

bool IntervalEngine::has_avx2_support() const {
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_max(0, nullptr) >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        return (ebx & (1 << 5)) != 0; // AVX2 bit
    }
    return false;
#elif defined(__arm64__) || defined(__aarch64__)
    // ARM NEON is always available on ARM64
    return true; 
#else
    return false;
#endif
}

bool IntervalEngine::has_sse4_support() const {
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return (ecx & (1 << 19)) != 0; // SSE4.1 bit
    }
    return false;
#elif defined(__arm64__) || defined(__aarch64__)
    // Use NEON instead of SSE on ARM
    return true;
#else
    return false;
#endif
}

IntervalResult IntervalEngine::calculateIntervals(const std::vector<int>& midi_notes) const {
    IntervalResult result;
    
    if (!validateInput(midi_notes)) {
        return result;
    }
    
    // Enhanced cache lookup with prefetching
    std::string* cached_result = interval_cache.get(midi_notes);
    if (cached_result && !cached_result->empty()) {
        // Parse cached result back to IntervalResult
        // For now, we'll skip cache for intervals and compute fresh
    }
    
    // Clean and sort input with reserve optimization
    std::vector<int> clean_notes;
    clean_notes.reserve(midi_notes.size());
    clean_notes = sortAndDeduplicate(std::vector<int>(midi_notes));
    
    if (clean_notes.empty()) {
        return result;
    }
    
    // Early exit for single note
    if (clean_notes.size() == 1) {
        result.bass_note = clean_notes[0];
        result.root_note = clean_notes[0];
        result.intervals = {0};
        result.has_inversion = false;
        return result;
    }
    
    // Detect bass note
    result.bass_note = detect_bass_note(clean_notes);
    
    // Calculate intervals relative to bass with optimized path selection
    result.intervals.reserve(clean_notes.size());
    
    // Use SIMD optimization based on note count and architecture
    if (has_avx2_support() && clean_notes.size() >= 4) {
        result.intervals = simd_engine.calculateIntervals_AVX2(clean_notes);
    } else if (clean_notes.size() >= 6) {
        // Use optimized scalar for medium-sized chords
        result.intervals = simd_engine.calculateIntervals_Scalar(clean_notes);
    } else {
        // Ultra-fast path for small chords (triads/7ths)
        const int bass = result.bass_note;
        for (int note : clean_notes) {
            result.intervals.push_back((note - bass + 12) % 12);
        }
    }
    
    // Normalize intervals (remove duplicates and sort)
    result.intervals = normalize_intervals(std::move(result.intervals));
    
    // Detect root note and inversion
    result.root_note = detect_root_note(clean_notes, result.intervals);
    result.has_inversion = is_inversion(result.intervals, 
                                      getIntervalClass(result.bass_note),
                                      getIntervalClass(result.root_note));
    
    return result;
}

IntervalResult IntervalEngine::calculateIntervals(const std::vector<int>& midi_notes, int specified_bass) const {
    IntervalResult result;
    
    if (!validateInput(midi_notes) || !isValidMidiNote(specified_bass)) {
        return result;
    }
    
    std::vector<int> clean_notes = sortAndDeduplicate(std::vector<int>(midi_notes));
    
    // Use specified bass
    result.bass_note = specified_bass;
    
    // Calculate intervals relative to specified bass
    result.intervals.reserve(clean_notes.size());
    
    for (int note : clean_notes) {
        int interval = (note - specified_bass) % OCTAVE;
        if (interval < 0) interval += OCTAVE;
        result.intervals.push_back(interval);
    }
    
    result.intervals = normalize_intervals(std::move(result.intervals));
    result.root_note = detect_root_note(clean_notes, result.intervals);
    result.has_inversion = is_inversion(result.intervals,
                                      getIntervalClass(result.bass_note),
                                      getIntervalClass(result.root_note));
    
    return result;
}

std::vector<int> IntervalEngine::normalize_intervals(std::vector<int> intervals) const {
    if (intervals.empty()) return intervals;
    
    // First normalize all intervals to 0-11 range (reduce to note classes)
    for (int& interval : intervals) {
        interval = interval % 12;
        if (interval < 0) interval += 12;
    }
    
    // Remove duplicates and sort
    std::sort(intervals.begin(), intervals.end());
    intervals.erase(std::unique(intervals.begin(), intervals.end()), intervals.end());
    
    // Ensure 0 is first (bass note)
    if (!intervals.empty() && intervals[0] != 0) {
        // Rotate so the lowest interval becomes 0
        int offset = intervals[0];
        for (int& interval : intervals) {
            interval = (interval - offset + 12) % 12;
        }
        std::sort(intervals.begin(), intervals.end());
    }
    
    return intervals;
}

std::vector<int> IntervalEngine::create_basic_intervals(const std::vector<int>& extended_intervals) const {
    std::vector<int> basic_intervals;
    std::set<int> unique_classes;
    
    for (int interval : extended_intervals) {
        // Reduce to basic interval class (0-11)
        int basic = interval % 12;
        unique_classes.insert(basic);
    }
    
    basic_intervals.assign(unique_classes.begin(), unique_classes.end());
    return basic_intervals;
}

int IntervalEngine::detect_bass_note(const std::vector<int>& notes) const {
    if (notes.empty()) return -1;
    
    // Enhanced bass detection with octave separation logic
    std::vector<int> sorted_notes = notes;
    std::sort(sorted_notes.begin(), sorted_notes.end());
    
    // If only 1-2 notes, use lowest
    if (sorted_notes.size() <= 2) {
        return sorted_notes[0];
    }
    
    // Detect potential left hand / right hand separation
    int lowest = sorted_notes[0];
    int second_lowest = sorted_notes[1];
    
    // If there's a significant gap (more than 1 octave) between lowest and next notes,
    // the lowest is likely a bass note played separately
    int gap = second_lowest - lowest;
    if (gap >= 12) { // 1 octave or more
        return lowest;
    }
    
    // If notes are clustered, analyze for musical bass patterns
    if (sorted_notes.size() >= 3) {
        int third_note = sorted_notes[2];
        
        // Check if lowest note forms a bass pattern with upper notes
        int bass_to_second = (second_lowest - lowest) % 12;
        int bass_to_third = (third_note - lowest) % 12;
        
        // Common bass intervals: root, fifth, octave
        if (bass_to_second == 7 || bass_to_third == 7 ||  // Perfect fifth
            bass_to_second == 0 || bass_to_third == 0) {  // Octave
            return lowest;
        }
        
        // If lowest note is isolated by more than a sixth, it's likely bass
        if (gap >= 9) { // Major sixth or more
            return lowest;
        }
    }
    
    return lowest; // Default to lowest note
}

int IntervalEngine::detect_root_note(const std::vector<int>& notes, const std::vector<int>& intervals) const {
    if (notes.empty() || intervals.empty()) return -1;
    
    // For now, assume bass is root (can be enhanced with chord theory)
    // In a more sophisticated version, we'd analyze the intervals to determine
    // if this is an inversion and calculate the theoretical root
    
    int bass_note = detect_bass_note(notes);
    
    // Simple heuristic: if intervals suggest inversion, try to find root
    if (intervals.size() >= 3) {
        // Check for common inversion patterns
        if (intervals.size() == 3) {
            // Check if this looks like first inversion (3rd in bass)
            // [0, 3, 8] -> first inversion of major triad
            // [0, 4, 9] -> first inversion of minor triad
            if ((intervals[1] == 3 && intervals[2] == 8) ||
                (intervals[1] == 4 && intervals[2] == 9)) {
                // This is likely first inversion, root is a major/minor third up
                return bass_note + intervals[1];
            }
            // Check for second inversion (5th in bass)
            // [0, 5, 9] -> second inversion of major triad
            // [0, 5, 8] -> second inversion of minor triad  
            else if ((intervals[1] == 5 && intervals[2] == 9) ||
                     (intervals[1] == 5 && intervals[2] == 8)) {
                // This is likely second inversion, root is a perfect fifth up
                return bass_note + 7; // Perfect fifth from the "5th"
            }
        }
    }
    
    return bass_note; // Default: bass is root
}

bool IntervalEngine::is_inversion(const std::vector<int>& intervals, int bass_class, int root_class) const {
    if (intervals.empty() || bass_class == root_class) return false;
    
    // If bass note is not 0 in the interval set, it might be an inversion
    return bass_class != root_class;
}

std::vector<int> IntervalEngine::getAllRotations(const std::vector<int>& intervals) const {
    if (intervals.size() <= 1) return intervals;
    
    std::vector<int> all_rotations;
    all_rotations.reserve(intervals.size() * intervals.size());
    
    for (size_t i = 0; i < intervals.size(); ++i) {
        std::vector<int> rotated = intervals;
        
        // Rotate by i positions
        std::rotate(rotated.begin(), rotated.begin() + i, rotated.end());
        
        // Normalize so first element is 0
        if (!rotated.empty() && rotated[0] != 0) {
            int offset = rotated[0];
            for (int& interval : rotated) {
                interval = (interval - offset + OCTAVE) % OCTAVE;
            }
        }
        
        // Add to result
        all_rotations.insert(all_rotations.end(), rotated.begin(), rotated.end());
    }
    
    return all_rotations;
}

std::vector<int> IntervalEngine::transposeIntervals(const std::vector<int>& intervals, int semitones) {
    std::vector<int> transposed;
    transposed.reserve(intervals.size());
    
    for (int interval : intervals) {
        int new_interval = (interval + semitones) % 12;
        if (new_interval < 0) new_interval += 12;
        transposed.push_back(new_interval);
    }
    
    std::sort(transposed.begin(), transposed.end());
    return transposed;
}

void IntervalEngine::warmupCache(const std::vector<std::vector<int>>& common_patterns) {
    for (const auto& pattern : common_patterns) {
        // Pre-calculate intervals for common patterns
        auto result = calculateIntervals(pattern);
        // Cache the result
        std::string cache_key = ""; // Simplified for now
        for (int note : pattern) {
            cache_key += std::to_string(note) + ",";
        }
        // interval_cache.put(pattern, cache_key); // Simplified caching
    }
}

std::vector<int> IntervalEngine::fast_modulo_12(const std::vector<int>& values) const {
    std::vector<int> result;
    result.reserve(values.size());
    
    for (int value : values) {
        // Fast modulo for positive values
        if (value >= 0) {
            result.push_back(value % 12);
        } else {
            int mod = value % 12;
            result.push_back(mod < 0 ? mod + 12 : mod);
        }
    }
    
    return result;
}

void IntervalEngine::apply_simd_optimization(std::vector<int>& intervals) const {
    // Additional SIMD optimizations could be applied here
    // For now, this is a placeholder for future enhancements
    (void)intervals; // Suppress unused parameter warning
}

} // namespace ChordLock