#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Chordlock Error Handling and Suggestion Test\n";
    std::cout << "============================================\n\n";
    
    // Test valid chords first
    std::cout << "🎵 Testing valid chords:\n";
    std::vector<std::string> validChords = {"C", "Cmaj7", "F#m", "Gadd9"};
    
    for (const auto& chord : validChords) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  " << chord << ": " << jsonResult << "\n";
    }
    
    std::cout << "\n❌ Testing invalid/non-existent chords:\n";
    
    // Test invalid chords that should trigger error handling
    std::vector<std::string> invalidChords = {
        "Cxyz",        // Invalid quality
        "H7",          // Invalid root note  
        "Cmaj13b9#11", // Too complex/non-existent
        "F#xyz",       // Invalid quality with sharp root
        "Bbsupercomplex", // Made-up chord type
        "Zminor"       // Invalid root note
    };
    
    for (const auto& chord : invalidChords) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  " << chord << ": " << jsonResult << "\n";
    }
    
    std::cout << "\n🔍 Testing partial matches (should provide suggestions):\n";
    
    // Test chords that are close to valid ones
    std::vector<std::string> partialChords = {
        "Cmaj",    // Close to C or Cmaj7
        "F#min",   // Close to F#m
        "Ddom7",   // Close to D7
        "Amajor"   // Close to A or Amaj7
    };
    
    for (const auto& chord : partialChords) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        std::cout << "  " << chord << ": " << jsonResult << "\n";
    }
    
    std::cout << "\n✅ Error handling and suggestion test complete!\n";
    
    return 0;
}