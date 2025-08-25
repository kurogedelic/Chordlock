#pragma once

#include <vector>
#include <unordered_map>
#include <array>
#include <cstdint>
#ifdef __x86_64__
#include <immintrin.h>
#elif defined(__arm64__) || defined(__aarch64__)
#include <arm_neon.h>
#endif

namespace ChordLock {

// ========== PERFORMANCE OPTIMIZATION STRATEGIES ==========

// 1. COMPILE-TIME LOOKUP TABLES
template<size_t MaxNotes = 16>
struct IntervalLookup {
    static constexpr std::array<uint8_t, 128> note_to_class = []() {
        std::array<uint8_t, 128> arr{};
        for (int i = 0; i < 128; ++i) {
            arr[i] = i % 12;
        }
        return arr;
    }();
    
    static constexpr std::array<uint8_t, 12> class_to_semitone = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
    };
};

// 2. SIMD-OPTIMIZED INTERVAL CALCULATION  
struct SIMDIntervalEngine {
#ifdef __x86_64__
    // Vectorized interval calculation using AVX2
    static inline std::vector<int> calculateIntervals_AVX2(const std::vector<int>& notes) {
        if (notes.empty()) return {};
        
        const int bass = *std::min_element(notes.begin(), notes.end());
        std::vector<int> intervals;
        intervals.reserve(notes.size());
        
        // SIMD batch processing for 8 notes at once
        size_t i = 0;
        const size_t simd_end = (notes.size() / 8) * 8;
        
        for (; i < simd_end; i += 8) {
            __m256i note_vec = _mm256_loadu_si256((__m256i*)&notes[i]);
            __m256i bass_vec = _mm256_set1_epi32(bass);
            __m256i diff_vec = _mm256_sub_epi32(note_vec, bass_vec);
            
            // Manual modulo 12 since _mm256_rem_epu32 doesn't exist
            int temp[8];
            _mm256_storeu_si256((__m256i*)temp, diff_vec);
            for (int j = 0; j < 8; ++j) {
                intervals.push_back((temp[j] + 12) % 12);
            }
        }
        
        // Handle remaining notes
        for (; i < notes.size(); ++i) {
            intervals.push_back((notes[i] - bass + 12) % 12);
        }
        
        return intervals;
    }
#elif defined(__arm64__) || defined(__aarch64__)
    // NEON-optimized interval calculation for ARM
    static inline std::vector<int> calculateIntervals_NEON(const std::vector<int>& notes) {
        if (notes.empty()) return {};
        
        const int bass = *std::min_element(notes.begin(), notes.end());
        std::vector<int> intervals;
        intervals.reserve(notes.size());
        
        // NEON batch processing for 4 notes at once
        size_t i = 0;
        const size_t simd_end = (notes.size() / 4) * 4;
        int32x4_t bass_vec = vdupq_n_s32(bass);
        
        for (; i < simd_end; i += 4) {
            int32x4_t note_vec = vld1q_s32(&notes[i]);
            int32x4_t diff_vec = vsubq_s32(note_vec, bass_vec);
            
            // Store and preserve extended intervals
            int temp[4];
            vst1q_s32(temp, diff_vec);
            for (int j = 0; j < 4; ++j) {
                int raw_interval = temp[j];
                if (raw_interval >= 24) {
                    intervals.push_back(raw_interval % 12 + 12);
                } else if (raw_interval >= 12) {
                    intervals.push_back(raw_interval);
                } else {
                    intervals.push_back(raw_interval < 0 ? raw_interval + 12 : raw_interval);
                }
            }
        }
        
        // Handle remaining notes with same logic
        for (; i < notes.size(); ++i) {
            int raw_interval = notes[i] - bass;
            if (raw_interval >= 24) {
                intervals.push_back(raw_interval % 12 + 12);
            } else if (raw_interval >= 12) {
                intervals.push_back(raw_interval);
            } else {
                intervals.push_back(raw_interval < 0 ? raw_interval + 12 : raw_interval);
            }
        }
        
        return intervals;
    }
    
    // Alias for consistency
    static inline std::vector<int> calculateIntervals_AVX2(const std::vector<int>& notes) {
        return calculateIntervals_NEON(notes);
    }
#else
    // Fallback for other architectures
    static inline std::vector<int> calculateIntervals_AVX2(const std::vector<int>& notes) {
        return calculateIntervals_Scalar(notes);
    }
#endif
    
    // Fallback scalar implementation
    static inline std::vector<int> calculateIntervals_Scalar(const std::vector<int>& notes) {
        if (notes.empty()) return {};
        
        const int bass = *std::min_element(notes.begin(), notes.end());
        std::vector<int> intervals;
        intervals.reserve(notes.size());
        
        for (int note : notes) {
            int raw_interval = note - bass;
            // Preserve extended intervals for 9ths, 11ths, 13ths
            if (raw_interval >= 24) {
                // Reduce by octaves but keep extension character
                intervals.push_back(raw_interval % 12 + 12);
            } else if (raw_interval >= 12) {
                // Keep extended intervals (9th=14, 11th=17, 13th=21)
                intervals.push_back(raw_interval);
            } else {
                intervals.push_back(raw_interval < 0 ? raw_interval + 12 : raw_interval);
            }
        }
        
        return intervals;
    }
};

// 3. HIGH-PERFORMANCE HASH MAP FOR CHORD LOOKUP
struct ChordHashMap {
    using IntervalKey = std::vector<int>;
    using ChordName = std::string;
    
    // Custom hash function optimized for interval vectors
    struct IntervalHash {
        std::size_t operator()(const IntervalKey& key) const noexcept {
            // Fast hash using FNV-1a algorithm (with proper size_t conversion)
            std::size_t hash = static_cast<std::size_t>(14695981039346656037ULL);
            for (int interval : key) {
                hash ^= static_cast<std::size_t>(interval);
                hash *= static_cast<std::size_t>(1099511628211ULL);
            }
            return hash;
        }
    };
    
    std::unordered_map<IntervalKey, ChordName, IntervalHash> chord_map;
    
    // Perfect hash lookup for common chords (compile-time)
    static constexpr std::array<std::pair<uint32_t, const char*>, 32> perfect_hash_table = {{
        {0x047, "major-triad"},      // [0,4,7]
        {0x037, "minor-triad"},      // [0,3,7] 
        {0x036, "diminished-triad"}, // [0,3,6]
        {0x048, "augmented-triad"},  // [0,4,8]
        // ... more common chords
    }};
    
    // Ultra-fast lookup for common chords
    static inline const char* perfectLookup(const IntervalKey& intervals) {
        if (intervals.size() != 3) return nullptr;
        
        uint32_t hash = (intervals[0] << 16) | (intervals[1] << 8) | intervals[2];
        for (const auto& [key, value] : perfect_hash_table) {
            if (key == hash) return value;
        }
        return nullptr;
    }
};

// 4. LRU CACHE FOR FREQUENT PATTERNS
template<size_t Capacity = 1024>
class LRUChordCache {
private:
    struct CacheNode {
        std::vector<int> intervals;
        std::string chord_name;
        CacheNode* prev = nullptr;
        CacheNode* next = nullptr;
    };
    
    mutable CacheNode* head;
    mutable CacheNode* tail;
    mutable std::unordered_map<std::vector<int>, CacheNode*, ChordHashMap::IntervalHash> cache_map;
    
    void moveToFront(CacheNode* node) const {
        if (node == head) return;
        
        // Remove from current position
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (node == tail) tail = node->prev;
        
        // Move to front
        node->prev = nullptr;
        node->next = head;
        if (head) head->prev = node;
        head = node;
        if (!tail) tail = node;
    }
    
public:
    LRUChordCache() : head(nullptr), tail(nullptr) {}
    
    std::string* get(const std::vector<int>& intervals) const {
        auto it = cache_map.find(intervals);
        if (it == cache_map.end()) return nullptr;
        
        moveToFront(it->second);
        return &(it->second->chord_name);
    }
    
    void put(const std::vector<int>& intervals, const std::string& chord_name) const {
        auto it = cache_map.find(intervals);
        if (it != cache_map.end()) {
            it->second->chord_name = chord_name;
            moveToFront(it->second);
            return;
        }
        
        // Evict if at capacity
        if (cache_map.size() >= Capacity && tail) {
            cache_map.erase(tail->intervals);
            CacheNode* old_tail = tail;
            tail = tail->prev;
            if (tail) tail->next = nullptr;
            delete old_tail;
        }
        
        // Add new node
        CacheNode* new_node = new CacheNode{intervals, chord_name};
        cache_map[intervals] = new_node;
        moveToFront(new_node);
    }
};

// 5. BLOOM FILTER FOR NEGATIVE LOOKUPS
class BloomFilter {
private:
    static constexpr size_t FILTER_SIZE = 8192;
    static constexpr size_t NUM_HASHES = 3;
    std::array<uint64_t, FILTER_SIZE / 64> bits{};
    
    std::array<size_t, NUM_HASHES> hash(const std::vector<int>& intervals) const {
        std::array<size_t, NUM_HASHES> hashes;
        std::size_t h1 = 0;
        for (int val : intervals) {
            h1 ^= std::hash<int>{}(val) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        }
        size_t h2 = h1 >> 16;
        
        for (size_t i = 0; i < NUM_HASHES; ++i) {
            hashes[i] = (h1 + i * h2) % FILTER_SIZE;
        }
        return hashes;
    }
    
public:
    void add(const std::vector<int>& intervals) {
        auto hashes = hash(intervals);
        for (size_t h : hashes) {
            bits[h / 64] |= (1ULL << (h % 64));
        }
    }
    
    bool mayContain(const std::vector<int>& intervals) const {
        auto hashes = hash(intervals);
        for (size_t h : hashes) {
            if (!(bits[h / 64] & (1ULL << (h % 64)))) {
                return false;
            }
        }
        return true;
    }
};

// 6. MEMORY POOL FOR ZERO-ALLOCATION PROCESSING
template<typename T, size_t PoolSize = 1024>
class MemoryPool {
private:
    alignas(T) char pool[sizeof(T) * PoolSize];
    std::array<bool, PoolSize> used{};
    size_t next_free = 0;
    
public:
    T* allocate() {
        for (size_t i = next_free; i < PoolSize; ++i) {
            if (!used[i]) {
                used[i] = true;
                next_free = i + 1;
                return reinterpret_cast<T*>(&pool[i * sizeof(T)]);
            }
        }
        
        // Wrap around
        for (size_t i = 0; i < next_free; ++i) {
            if (!used[i]) {
                used[i] = true;
                next_free = i + 1;
                return reinterpret_cast<T*>(&pool[i * sizeof(T)]);
            }
        }
        
        return nullptr; // Pool exhausted
    }
    
    void deallocate(T* ptr) {
        if (!ptr) return;
        size_t index = (reinterpret_cast<char*>(ptr) - pool) / sizeof(T);
        if (index < PoolSize) {
            used[index] = false;
            if (index < next_free) next_free = index;
        }
    }
};

} // namespace ChordLock