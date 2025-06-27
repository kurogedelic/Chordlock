#include "src/Chordlock.hpp"
#include <iostream>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Debug Cmaj7 Issue\n";
    std::cout << "=================\n\n";
    
    // Test parsing
    std::cout << "1. Parsing 'Cmaj7':\n";
    // We can't access parseChordName directly, so let's test the flow
    
    std::cout << "2. Testing theoretical calculation:\n";
    auto theoreticalNotes = chordlock.chordNameToNotes("Cmaj7", 4);
    std::cout << "   Theoretical result: [";
    for (size_t i = 0; i < theoreticalNotes.size(); i++) {
        std::cout << theoreticalNotes[i];
        if (i < theoreticalNotes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    // Test what happens if we manually force theoretical calculation
    std::cout << "\n3. Testing known chord types:\n";
    std::vector<std::string> testChords = {"maj7", "Maj7", "major7", "Major7"};
    
    // We can't directly test isKnownChordType, but we can see what happens
    for (const auto& quality : testChords) {
        auto notes = chordlock.chordNameToNotes("C" + quality, 4);
        std::cout << "   C" << quality << ": [";
        for (size_t i = 0; i < notes.size(); i++) {
            std::cout << notes[i];
            if (i < notes.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
    
    std::cout << "\n4. Testing with different case:\n";
    std::vector<std::string> caseTests = {"cmaj7", "CMAJ7", "CMaj7", "Cmaj7"};
    for (const auto& chord : caseTests) {
        auto notes = chordlock.chordNameToNotes(chord, 4);
        std::cout << "   " << chord << ": [";
        for (size_t i = 0; i < notes.size(); i++) {
            std::cout << notes[i];
            if (i < notes.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
    
    std::cout << "\nExpected: Cmaj7 should be [48, 52, 55, 59] (C-E-G-B)\n";
    std::cout << "Actual:   Getting [48, 51, 55, 58] (C-Eb-G-Bb) which is Cm7!\n";
    
    return 0;
}