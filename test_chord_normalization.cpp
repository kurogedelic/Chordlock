#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Chord Name Normalization Test\n";
    std::cout << "=============================\n\n";
    
    std::cout << "Testing various input formats that should return the same canonical name:\n\n";
    
    // Test augmented chord variations
    std::cout << "🔸 Augmented Chord Variations (should all return 'Caug'):\n";
    std::vector<std::string> augVariations = {"C+", "Caug", "Caugmented"};
    
    for (const auto& chord : augVariations) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  Input: " << chord << " → " << jsonResult << "\n";
    }
    
    // Test diminished chord variations  
    std::cout << "\n🔸 Diminished Chord Variations (should return 'Cdim' for triad, 'Cdim7' for 7th):\n";
    std::vector<std::string> dimVariations = {"C°", "Cdim", "Cdiminished"};
    
    for (const auto& chord : dimVariations) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  Input: " << chord << " → " << jsonResult << "\n";
    }
    
    // Test major chord variations
    std::cout << "\n🔸 Major Chord Variations (should all return 'C'):\n";
    std::vector<std::string> majorVariations = {"C", "CM", "Cmaj", "Cmajor"};
    
    for (const auto& chord : majorVariations) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  Input: " << chord << " → " << jsonResult << "\n";
    }
    
    // Test major 7th variations
    std::cout << "\n🔸 Major 7th Variations (should all return 'Cmaj7'):\n";
    std::vector<std::string> maj7Variations = {"CM7", "Cmaj7", "CMaj7", "Cmajor7"};
    
    for (const auto& chord : maj7Variations) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  Input: " << chord << " → " << jsonResult << "\n";
    }
    
    // Test suspended chord variations
    std::cout << "\n🔸 Suspended Chord Variations:\n";
    std::vector<std::string> susVariations = {"Csus", "Csus4"};
    
    for (const auto& chord : susVariations) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  Input: " << chord << " → " << jsonResult << "\n";
    }
    
    return 0;
}