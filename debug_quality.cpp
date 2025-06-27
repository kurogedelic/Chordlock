#include "src/Chordlock.hpp"
#include <iostream>
#include <string>
#include <algorithm>

// Test the normalization and quality extraction logic
void testQualityNormalization(const std::string& input) {
    std::cout << "Input: '" << input << "'\n";
    
    // Simulate the parseChordName logic
    std::string normalized = input;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    std::string root, quality;
    if (normalized.length() >= 1) {
        if (normalized.length() >= 2 && (normalized[1] == '#' || normalized[1] == 'b')) {
            root = normalized.substr(0, 2);
            quality = normalized.substr(2);
        } else {
            root = normalized.substr(0, 1);
            quality = normalized.substr(1);
        }
    }
    
    std::cout << "  Normalized: '" << normalized << "'\n";
    std::cout << "  Root: '" << root << "'\n";
    std::cout << "  Quality: '" << quality << "'\n";
    
    // Test what intervals this would get
    std::cout << "  Expected intervals for '" << quality << "': ";
    if (quality == "maj7" || quality == "major7") {
        std::cout << "[0, 4, 7, 11] (C-E-G-B)";
    } else if (quality == "m7" || quality == "min7") {
        std::cout << "[0, 3, 7, 10] (C-Eb-G-Bb)";
    } else {
        std::cout << "Unknown quality";
    }
    std::cout << "\n\n";
}

int main() {
    std::cout << "Quality Normalization Debug\n";
    std::cout << "===========================\n\n";
    
    testQualityNormalization("Cmaj7");
    testQualityNormalization("CM7");
    testQualityNormalization("Cm7");
    testQualityNormalization("cmaj7");
    
    return 0;
}