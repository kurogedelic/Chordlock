#pragma once
#include <cstdint>
#include <cmath>
#include <string>

/**
 * SIMD-optimized utilities for high-performance bit operations
 * Provides hardware acceleration with software fallbacks
 */
namespace ChordlockSIMD {

// Hardware-accelerated bit counting
inline int fast_popcount(uint16_t mask) {
#if defined(__POPCNT__)
    return __builtin_popcountll(mask);
#else
    // Brian Kernighan's algorithm for software fallback
    int count = 0;
    while (mask) {
        mask &= (mask - 1);
        count++;
    }
    return count;
#endif
}

// Fast trailing zero count
inline int fast_trailing_zeros(uint16_t mask) {
#if defined(__BMI__) || defined(__LZCNT__)
    return mask ? __builtin_ctzll(mask) : 12;
#else
    if (mask == 0) return 12;
    int count = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        count++;
        if (count >= 12) break;  // Safety check
    }
    return count;
#endif
}

// Fast leading zero count for finding highest set bit
inline int fast_leading_zeros(uint16_t mask) {
#if defined(__LZCNT__)
    return mask ? __builtin_clzll(mask) - 48 : 16; // Adjust for 16-bit
#else
    if (mask == 0) return 16;
    int count = 0;
    uint16_t test = 0x8000;
    while ((mask & test) == 0) {
        test >>= 1;
        count++;
        if (count >= 16) break;  // Safety check
    }
    return count;
#endif
}

// Find lowest set bit position
inline int find_lowest_bit(uint16_t mask) {
    return fast_trailing_zeros(mask);
}

// Find highest set bit position  
inline int find_highest_bit(uint16_t mask) {
    return mask ? (15 - fast_leading_zeros(mask)) : -1;
}

// Parallel bit operations for mask manipulation
inline uint16_t rotate_mask_left(uint16_t mask, int positions) {
    positions %= 12;  // Only 12 pitch classes
    if (positions == 0) return mask;
    
    uint16_t result = 0;
    for (int i = 0; i < 12; i++) {
        if (mask & (1 << i)) {
            result |= (1 << ((i + positions) % 12));
        }
    }
    return result;
}

inline uint16_t rotate_mask_right(uint16_t mask, int positions) {
    positions %= 12;
    if (positions == 0) return mask;
    
    uint16_t result = 0;
    for (int i = 0; i < 12; i++) {
        if (mask & (1 << i)) {
            result |= (1 << ((i - positions + 12) % 12));
        }
    }
    return result;
}

// Optimized Jaccard similarity calculation
inline float jaccard_similarity(uint16_t mask1, uint16_t mask2) {
    int intersection = fast_popcount(mask1 & mask2);
    int union_size = fast_popcount(mask1 | mask2);
    return union_size > 0 ? static_cast<float>(intersection) / union_size : 0.0f;
}

// Weighted similarity calculation
inline float weighted_similarity(uint16_t mask1, uint16_t mask2, const float weights[12]) {
    float weighted_intersection = 0.0f;
    float weighted_union = 0.0f;
    
    for (int i = 0; i < 12; i++) {
        bool in_mask1 = (mask1 & (1 << i)) != 0;
        bool in_mask2 = (mask2 & (1 << i)) != 0;
        
        if (in_mask1 || in_mask2) {
            float weight = weights[i];
            weighted_union += weight;
            
            if (in_mask1 && in_mask2) {
                weighted_intersection += weight;
            }
        }
    }
    
    return weighted_union > 0.0f ? weighted_intersection / weighted_union : 0.0f;
}

// Batch processing utilities
struct MaskProcessor {
    static constexpr int BATCH_SIZE = 4;
    
    // Process multiple masks in parallel-friendly manner
    static void process_mask_batch(const uint16_t masks[], float results[], int count) {
        for (int i = 0; i < count; i += BATCH_SIZE) {
            int batch_end = std::min(i + BATCH_SIZE, count);
            
            // Unroll for better performance
            for (int j = i; j < batch_end; j++) {
                results[j] = static_cast<float>(fast_popcount(masks[j]));
            }
        }
    }
    
    // Calculate multiple Jaccard similarities
    static void jaccard_batch(uint16_t reference, const uint16_t masks[], float results[], int count) {
        for (int i = 0; i < count; i++) {
            results[i] = jaccard_similarity(reference, masks[i]);
        }
    }
};

// Confidence calculation optimization
struct ConfidenceCalculator {
    // Optimized confidence calculation with velocity weighting
    static float calculate_weighted_confidence(
        uint16_t detected_mask,
        uint16_t input_mask,
        const float weights[12],
        float base_confidence) {
        
        // Fast path for simple cases
        if (detected_mask == 0 || input_mask == 0) return 0.0f;
        
        // Use SIMD-optimized operations
        float jaccard = jaccard_similarity(detected_mask, input_mask);
        
        // Weight adjustment based on velocity importance
        float weight_factor = 1.0f;
        if (weights) {
            float weighted_score = 0.0f;
            float total_weight = 0.0f;
            
            for (int i = 0; i < 12; i++) {
                if (detected_mask & (1 << i)) {
                    weighted_score += weights[i];
                    total_weight += weights[i];
                }
            }
            
            weight_factor = total_weight > 0.0f ? (weighted_score / total_weight) : 1.0f;
        }
        
        return base_confidence * jaccard * weight_factor;
    }
    
    // Batch confidence calculation
    static void calculate_batch_confidence(
        const uint16_t detected_masks[],
        uint16_t input_mask,
        const float base_confidences[],
        float results[],
        int count,
        const float weights[12] = nullptr) {
        
        for (int i = 0; i < count; i++) {
            results[i] = calculate_weighted_confidence(
                detected_masks[i],
                input_mask, 
                weights,
                base_confidences[i]
            );
        }
    }
};

// CPU feature detection
struct CPUFeatures {
    static bool has_popcnt() {
#if defined(__POPCNT__)
        return true;
#else
        return false;
#endif
    }
    
    static bool has_bmi() {
#if defined(__BMI__)
        return true;
#else
        return false;
#endif
    }
    
    static std::string get_feature_string() {
        std::string features = "SIMD Features: ";
        if (has_popcnt()) features += "POPCNT ";
        if (has_bmi()) features += "BMI ";
        if (features.length() == 15) features += "None";
        return features;
    }
};

} // namespace ChordlockSIMD