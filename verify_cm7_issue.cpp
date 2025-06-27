#include "src/Chordlock.hpp"
#include <iostream>

int main() {
    Chordlock chordlock;
    
    std::cout << "=== Verifying CM7 Issue ===\n\n";
    
    // Test exact issue user reported
    std::cout << "1. Testing CM7:\n";
    std::string cm7_result = chordlock.chordNameToNotesJSON("CM7", 4);
    std::cout << "   " << cm7_result << "\n\n";
    
    std::cout << "2. Testing Cmaj7:\n";
    std::string cmaj7_result = chordlock.chordNameToNotesJSON("Cmaj7", 4);
    std::cout << "   " << cmaj7_result << "\n\n";
    
    // Extract chord names
    size_t pos1 = cm7_result.find("\"chord\":\"") + 9;
    size_t end1 = cm7_result.find("\"", pos1);
    std::string cm7_chord = cm7_result.substr(pos1, end1 - pos1);
    
    size_t pos2 = cmaj7_result.find("\"chord\":\"") + 9;
    size_t end2 = cmaj7_result.find("\"", pos2);
    std::string cmaj7_chord = cmaj7_result.substr(pos2, end2 - pos2);
    
    std::cout << "3. Extracted chord names:\n";
    std::cout << "   CM7 → \"" << cm7_chord << "\"\n";
    std::cout << "   Cmaj7 → \"" << cmaj7_chord << "\"\n\n";
    
    std::cout << "4. Are they the same? " << (cm7_chord == cmaj7_chord ? "YES ✅" : "NO ❌") << "\n";
    
    if (cm7_chord != cmaj7_chord) {
        std::cout << "\n❌ PROBLEM CONFIRMED: CM7 and Cmaj7 return different canonical names!\n";
        std::cout << "Expected: Both should return \"Cmaj7\"\n";
        return 1;
    } else {
        std::cout << "\n✅ Good: Both return the same canonical name\n";
        return 0;
    }
}