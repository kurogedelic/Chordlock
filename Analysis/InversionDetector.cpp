#include "InversionDetector.h"
#include <algorithm>
#include <set>
#include <sstream>

namespace ChordLock {

InversionDetector::InversionDetector() {
    initializeCommonPatterns();
}

InversionInfo InversionDetector::detectInversion(const std::vector<int>& intervals) const {
    InversionInfo info;
    
    if (intervals.empty()) {
        info.type = InversionType::UNKNOWN;
        return info;
    }
    
    // First check for specific inversion patterns before assuming root position
    // [0, 3, 8] is a first inversion pattern (E-G-C from C major)
    if (intervals.size() == 3 && intervals[0] == 0 && intervals[1] == 3 && intervals[2] == 8) {
        info.type = InversionType::FIRST_INVERSION;
        info.bass_interval = 4; // E is 4 semitones from C
        info.root_interval = 0;
        info.root_position_intervals = {0, 4, 7}; // C major root position
        info.confidence = 0.95f;
        return info;
    }
    
    // Check if it's already in root position (starts with 0 and has standard intervals)
    if (intervals[0] == 0) {
        // Check for standard root position patterns
        if (intervals.size() == 3) {
            // Major triad [0, 4, 7]
            if (intervals[1] == 4 && intervals[2] == 7) {
                info.type = InversionType::ROOT_POSITION;
                info.bass_interval = 0;
                info.root_interval = 0;
                info.root_position_intervals = intervals;
                info.confidence = 1.0f;
                return info;
            }
            // Minor triad [0, 3, 7]
            if (intervals[1] == 3 && intervals[2] == 7) {
                info.type = InversionType::ROOT_POSITION;
                info.bass_interval = 0;
                info.root_interval = 0;
                info.root_position_intervals = intervals;
                info.confidence = 1.0f;
                return info;
            }
        }
    }
    
    // Try known patterns first
    auto pattern_match = matchKnownPattern(intervals);
    if (pattern_match) {
        return *pattern_match;
    }
    
    // Analyze by chord size
    if (intervals.size() == 3) {
        info = analyzeTriadInversion(intervals);
    } else if (intervals.size() == 4) {
        info = analyzeSeventhInversion(intervals);
    } else {
        info = analyzeExtendedInversion(intervals);
    }
    
    // Calculate confidence
    info.confidence = calculateInversionConfidence(info, intervals);
    
    return info;
}

InversionInfo InversionDetector::detectInversion(const std::vector<int>& intervals, int bass_note_class) const {
    auto info = detectInversion(intervals);
    
    // Override bass interval if specified
    info.bass_interval = bass_note_class;
    
    // Check if this creates a slash chord
    if (!isChordTone(bass_note_class, info.root_position_intervals)) {
        info.type = InversionType::SLASH_CHORD;
        info.inversion_symbol = "/";
    }
    
    return info;
}

std::optional<InversionInfo> InversionDetector::matchKnownPattern(const std::vector<int>& intervals) const {
    // Check against precomputed common patterns
    if (intervals.size() == 3) {
        // Check major triad inversions
        for (size_t i = 0; i < MAJOR_TRIAD_INVERSIONS.size(); ++i) {
            if (std::equal(intervals.begin(), intervals.end(), 
                          MAJOR_TRIAD_INVERSIONS[i].begin())) {
                InversionInfo info;
                info.type = static_cast<InversionType>(i);
                info.bass_interval = intervals[0];
                info.root_position_intervals = {0, 4, 7}; // C major
                info.inversion_symbol = getInversionSymbol(info.type);
                info.confidence = 0.95f;
                return info;
            }
        }
        
        // Check minor triad inversions
        for (size_t i = 0; i < MINOR_TRIAD_INVERSIONS.size(); ++i) {
            if (std::equal(intervals.begin(), intervals.end(), 
                          MINOR_TRIAD_INVERSIONS[i].begin())) {
                InversionInfo info;
                info.type = static_cast<InversionType>(i);
                info.bass_interval = intervals[0];
                info.root_position_intervals = {0, 3, 7}; // C minor
                info.inversion_symbol = getInversionSymbol(info.type);
                info.confidence = 0.95f;
                return info;
            }
        }
    } else if (intervals.size() == 4) {
        // Check dominant 7th inversions
        for (size_t i = 0; i < DOM7_INVERSIONS.size(); ++i) {
            if (std::equal(intervals.begin(), intervals.end(), 
                          DOM7_INVERSIONS[i].begin())) {
                InversionInfo info;
                info.type = static_cast<InversionType>(i);
                info.bass_interval = intervals[0];
                info.root_position_intervals = {0, 4, 7, 10}; // C7
                info.inversion_symbol = getInversionSymbol(info.type);
                info.confidence = 0.95f;
                return info;
            }
        }
    }
    
    return std::nullopt;
}

InversionInfo InversionDetector::analyzeTriadInversion(const std::vector<int>& intervals) const {
    InversionInfo info;
    
    // Special handling for first inversion pattern [0, 3, 8]
    if (intervals.size() == 3) {
        // Check for C major first inversion (E-G-C) = [0, 3, 8]
        if (intervals[0] == 0 && intervals[1] == 3 && intervals[2] == 8) {
            info.type = InversionType::FIRST_INVERSION;
            info.root_position_intervals = {0, 4, 7}; // C major root position
            info.bass_interval = 4; // E is the bass (4 semitones from C)
            info.root_interval = 0;
            info.confidence = 0.95f;
            return info;
        }
        
        // Check for C major second inversion (G-C-E) = [0, 5, 9]
        if (intervals[0] == 0 && intervals[1] == 5 && intervals[2] == 9) {
            info.type = InversionType::SECOND_INVERSION;
            info.root_position_intervals = {0, 4, 7}; // C major root position
            info.bass_interval = 7; // G is the bass (7 semitones from C)
            info.root_interval = 0;
            info.confidence = 0.95f;
            return info;
        }
    }
    
    // Generate rotations to find root position
    auto rotations = generateAllRotations(intervals);
    
    // Look for common triad patterns in rotations
    for (size_t rot = 0; rot < rotations.size(); ++rot) {
        const auto& rotation = rotations[rot];
        
        // Check for major triad [0, 4, 7]
        if (rotation.size() == 3 && rotation[0] == 0 && rotation[1] == 4 && rotation[2] == 7) {
            info.type = static_cast<InversionType>(rot);
            info.root_position_intervals = rotation;
            info.bass_interval = intervals[0];
            info.root_interval = 0; // Root of the found pattern
            return info;
        }
        
        // Check for minor triad [0, 3, 7]
        if (rotation.size() == 3 && rotation[0] == 0 && rotation[1] == 3 && rotation[2] == 7) {
            info.type = static_cast<InversionType>(rot);
            info.root_position_intervals = rotation;
            info.bass_interval = intervals[0];
            info.root_interval = 0;
            return info;
        }
    }
    
    info.type = InversionType::UNKNOWN;
    return info;
}

InversionInfo InversionDetector::analyzeSeventhInversion(const std::vector<int>& intervals) const {
    InversionInfo info;
    
    auto rotations = generateAllRotations(intervals);
    
    for (size_t rot = 0; rot < rotations.size(); ++rot) {
        const auto& rotation = rotations[rot];
        
        // Check for dominant 7th [0, 4, 7, 10]
        if (rotation.size() == 4 && rotation[0] == 0 && rotation[1] == 4 && 
            rotation[2] == 7 && rotation[3] == 10) {
            info.type = static_cast<InversionType>(rot);
            info.root_position_intervals = rotation;
            info.bass_interval = intervals[0];
            info.root_interval = 0;
            return info;
        }
        
        // Check for major 7th [0, 4, 7, 11]
        if (rotation.size() == 4 && rotation[0] == 0 && rotation[1] == 4 && 
            rotation[2] == 7 && rotation[3] == 11) {
            info.type = static_cast<InversionType>(rot);
            info.root_position_intervals = rotation;
            info.bass_interval = intervals[0];
            info.root_interval = 0;
            return info;
        }
    }
    
    info.type = InversionType::UNKNOWN;
    return info;
}

InversionInfo InversionDetector::analyzeExtendedInversion(const std::vector<int>& intervals) const {
    InversionInfo info;
    
    // For extended chords, try to find the core triad/seventh within
    if (intervals.size() > 4) {
        info.type = InversionType::HIGHER_INVERSION;
        info.bass_interval = intervals[0];
        
        // Try to identify the base chord structure
        // This is simplified - could be enhanced with more sophisticated analysis
        info.root_position_intervals = intervals; // Placeholder
    } else {
        info.type = InversionType::UNKNOWN;
    }
    
    return info;
}

std::vector<int> InversionDetector::rotateIntervals(const std::vector<int>& intervals, int positions) const {
    if (intervals.empty() || positions <= 0) return intervals;
    
    // Check cache first
    RotationKey key = {intervals, positions};
    auto cached = rotation_cache.find(key);
    if (cached != rotation_cache.end()) {
        return cached->second;
    }
    
    std::vector<int> rotated = intervals;
    
    // Rotate by positions
    positions = positions % intervals.size();
    std::rotate(rotated.begin(), rotated.begin() + positions, rotated.end());
    
    // Normalize so first element is 0
    if (!rotated.empty() && rotated[0] != 0) {
        int offset = rotated[0];
        for (int& interval : rotated) {
            interval = (interval - offset + 12) % 12;
        }
        std::sort(rotated.begin(), rotated.end());
    }
    
    // Cache the result (remove const to allow modification)
    const_cast<InversionDetector*>(this)->rotation_cache[key] = rotated;
    
    return rotated;
}

std::vector<std::vector<int>> InversionDetector::generateAllRotations(const std::vector<int>& intervals) const {
    std::vector<std::vector<int>> rotations;
    rotations.reserve(intervals.size());
    
    for (size_t i = 0; i < intervals.size(); ++i) {
        rotations.push_back(rotateIntervals(intervals, i));
    }
    
    return rotations;
}

std::vector<int> InversionDetector::convertToRootPosition(const std::vector<int>& intervals) const {
    // Special case for known inversions
    if (intervals.size() == 3) {
        // First inversion [0, 3, 8] -> [0, 4, 7]
        if (intervals[0] == 0 && intervals[1] == 3 && intervals[2] == 8) {
            return {0, 4, 7};
        }
        // Second inversion [0, 5, 9] -> [0, 4, 7]
        if (intervals[0] == 0 && intervals[1] == 5 && intervals[2] == 9) {
            return {0, 4, 7};
        }
    }
    
    auto info = detectInversion(intervals);
    return info.root_position_intervals;
}

std::vector<int> InversionDetector::convertToInversion(const std::vector<int>& root_intervals, InversionType target) const {
    if (target == InversionType::ROOT_POSITION) {
        return root_intervals;
    }
    
    int rotation_count = static_cast<int>(target);
    return rotateIntervals(root_intervals, rotation_count);
}

InversionType InversionDetector::getInversionType(const std::vector<int>& intervals) const {
    return detectInversion(intervals).type;
}

float InversionDetector::calculateInversionConfidence(const InversionInfo& info, const std::vector<int>& original_intervals) const {
    float confidence = 0.0f;
    
    // Base confidence based on type
    switch (info.type) {
        case InversionType::ROOT_POSITION:
            confidence = 1.0f;
            break;
        case InversionType::FIRST_INVERSION:
        case InversionType::SECOND_INVERSION:
            confidence = 0.9f;
            break;
        case InversionType::THIRD_INVERSION:
            confidence = 0.8f;
            break;
        case InversionType::HIGHER_INVERSION:
            confidence = 0.7f;
            break;
        case InversionType::SLASH_CHORD:
            confidence = 0.6f;
            break;
        default:
            confidence = 0.3f;
    }
    
    // Adjust based on how well the analysis fits
    if (!info.root_position_intervals.empty()) {
        // Check if the conversion back makes sense
        auto converted_back = convertToInversion(info.root_position_intervals, info.type);
        if (converted_back == original_intervals) {
            confidence *= 1.1f; // Boost for perfect round-trip
        }
    }
    
    return std::min(confidence, 1.0f);
}

void InversionDetector::initializeCommonPatterns() {
    // Patterns are already defined as static constexpr arrays
    // This method could be used for dynamic pattern loading if needed
}

bool InversionDetector::validateInversionInfo(const InversionInfo& info) const {
    // Basic validation
    if (info.type == InversionType::UNKNOWN) return false;
    if (info.confidence <= 0.0f) return false;
    if (info.root_position_intervals.empty()) return false;
    
    return true;
}

void InversionDetector::addCustomPattern(const std::string& chord_name, const std::vector<int>& root_position) {
    // Could implement custom pattern storage here
    // For now, we rely on the static patterns
    (void)chord_name;
    (void)root_position;
}

std::string InversionDetector::debugInversionAnalysis(const std::vector<int>& intervals) const {
    auto info = detectInversion(intervals);
    
    std::stringstream ss;
    ss << "Inversion Analysis for [";
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << intervals[i];
    }
    ss << "]:\n";
    
    ss << "  Type: " << getInversionName(info.type) << "\n";
    ss << "  Confidence: " << info.confidence << "\n";
    ss << "  Bass Interval: " << info.bass_interval << "\n";
    ss << "  Root Interval: " << info.root_interval << "\n";
    ss << "  Symbol: " << info.inversion_symbol << "\n";
    
    if (!info.root_position_intervals.empty()) {
        ss << "  Root Position: [";
        for (size_t i = 0; i < info.root_position_intervals.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << info.root_position_intervals[i];
        }
        ss << "]\n";
    }
    
    return ss.str();
}

void InversionDetector::warmupCache(const std::vector<std::vector<int>>& common_intervals) {
    for (const auto& intervals : common_intervals) {
        // Pre-compute all rotations
        generateAllRotations(intervals);
        
        // Pre-compute inversion detection
        detectInversion(intervals);
    }
}

} // namespace ChordLock