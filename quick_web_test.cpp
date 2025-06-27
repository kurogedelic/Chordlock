#include "src/Chordlock.hpp"
#include <iostream>

int main() {
    Chordlock chordlock;
    
    std::cout << "=== Quick Verification ===\n";
    
    // Test the exact cases user mentioned
    auto cm7_result = chordlock.chordNameToNotesJSON("CM7", 4);
    auto cmaj7_result = chordlock.chordNameToNotesJSON("Cmaj7", 4);
    
    std::cout << "CM7 result: " << cm7_result << "\n";
    std::cout << "Cmaj7 result: " << cmaj7_result << "\n";
    
    // Extract chord names for comparison
    size_t pos1 = cm7_result.find("\"chord\":\"") + 9;
    size_t end1 = cm7_result.find("\"", pos1);
    std::string cm7_chord = cm7_result.substr(pos1, end1 - pos1);
    
    size_t pos2 = cmaj7_result.find("\"chord\":\"") + 9;
    size_t end2 = cmaj7_result.find("\"", pos2);
    std::string cmaj7_chord = cmaj7_result.substr(pos2, end2 - pos2);
    
    std::cout << "\nExtracted chord names:\n";
    std::cout << "CM7 → " << cm7_chord << "\n";
    std::cout << "Cmaj7 → " << cmaj7_chord << "\n";
    
    if (cm7_chord == cmaj7_chord) {
        std::cout << "✅ CORRECT: Both return the same canonical name\n";
    } else {
        std::cout << "❌ PROBLEM: Different canonical names returned\n";
    }
    
    return 0;
}