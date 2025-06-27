#include "src/Chordlock.hpp"
#include <iostream>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Debug CM vs Cm Bug\n";
    std::cout << "==================\n\n";
    
    // Test the basic major chord notations
    std::vector<std::string> majorTests = {"C", "CM", "Cmaj", "Cmajor"};
    std::vector<std::string> minorTests = {"Cm", "Cmin", "Cminor"};
    
    std::cout << "Major chord tests (should be [48,52,55] = C-E-G):\n";
    for (const auto& chord : majorTests) {
        auto notes = chordlock.chordNameToNotes(chord, 4);
        std::cout << "  " << chord << ": [";
        for (size_t i = 0; i < notes.size(); i++) {
            std::cout << notes[i];
            if (i < notes.size() - 1) std::cout << ", ";
        }
        std::cout << "]";
        
        // Check if it's actually major (4 semitones between root and 3rd)
        if (notes.size() >= 2) {
            int interval = notes[1] - notes[0];
            std::cout << " - " << (interval == 4 ? "✅ Major" : "❌ Minor");
        }
        std::cout << "\n";
    }
    
    std::cout << "\nMinor chord tests (should be [48,51,55] = C-Eb-G):\n";
    for (const auto& chord : minorTests) {
        auto notes = chordlock.chordNameToNotes(chord, 4);
        std::cout << "  " << chord << ": [";
        for (size_t i = 0; i < notes.size(); i++) {
            std::cout << notes[i];
            if (i < notes.size() - 1) std::cout << ", ";
        }
        std::cout << "]";
        
        // Check if it's actually minor (3 semitones between root and 3rd)
        if (notes.size() >= 2) {
            int interval = notes[1] - notes[0];
            std::cout << " - " << (interval == 3 ? "✅ Minor" : "❌ Major");
        }
        std::cout << "\n";
    }
    
    return 0;
}