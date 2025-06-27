#include "src/Chordlock.hpp"
#include <iostream>

int main() {
    Chordlock chordlock;
    
    std::cout << "=== Test Canonical Name Generation ===\n\n";
    
    // Test canonical name generation with known intervals
    std::vector<int> cmaj7Intervals = {0, 4, 7, 11};
    std::string root = "C";
    
    std::cout << "Testing getCanonicalChordName(\"" << root << "\", [0, 4, 7, 11]):\n";
    
    // This should return "Cmaj7"
    std::string canonical = chordlock.getCanonicalChordName(root, cmaj7Intervals);
    std::cout << "Result: \"" << canonical << "\"\n";
    
    if (canonical == "Cmaj7") {
        std::cout << "✅ Correct! getCanonicalChordName works\n";
    } else if (canonical == "C") {
        std::cout << "❌ Returned just root - intervals were considered empty or no match found\n";
    } else {
        std::cout << "❌ Unexpected result: \"" << canonical << "\"\n";
    }
    
    return 0;
}