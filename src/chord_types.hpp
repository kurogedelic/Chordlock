#pragma once
#include <string>
#include <cstdint>

/**
 * Basic chord candidate structure
 */
struct ChordCandidate {
    std::string name;
    uint16_t mask;
    float confidence;
    
    ChordCandidate() : mask(0), confidence(0.0f) {}
    ChordCandidate(const std::string& n, uint16_t m, float c) 
        : name(n), mask(m), confidence(c) {}
    
    bool operator<(const ChordCandidate& other) const {
        return confidence > other.confidence; // Higher confidence first
    }
};