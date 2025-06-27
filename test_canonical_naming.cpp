#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Canonical Chord Naming Test\n";
    std::cout << "===========================\n\n";
    
    // Test various input formats that should return canonical names
    std::vector<std::pair<std::string, std::string>> tests = {
        {"C+", "Caug"},
        {"Caug", "Caug"},  
        {"C°", "Cdim"},
        {"Cdim", "Cdim"},
        {"CM", "C"},
        {"Cmaj", "C"},
        {"CM7", "Cmaj7"},
        {"Cmaj7", "Cmaj7"},
        {"Csus", "Csus4"},
        {"Csus4", "Csus4"},
        {"C5", "C5"}
    };
    
    for (const auto& test : tests) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(test.first, 4);
        std::cout << "Input: " << test.first << " (expect: " << test.second << ")\n";
        std::cout << "  Result: " << jsonResult << "\n\n";
    }
    
    return 0;
}