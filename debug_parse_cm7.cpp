#include "src/Chordlock.hpp"
#include <iostream>

int main() {
    Chordlock chordlock;
    
    std::cout << "=== Debug parseChordName for CM7 ===\n\n";
    
    // We need to access private methods, so let's use the public interface and deduce the problem
    std::string testChord = "CM7";
    std::cout << "Testing: " << testChord << "\n\n";
    
    // Test with different variations to see which work
    std::vector<std::string> variations = {
        "CM7", "Cmaj7", "CMaj7", "Cmajor7",
        "C", "CM", "Cmaj", "Cmajor"
    };
    
    for (const auto& chord : variations) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(chord, 4);
        
        // Extract canonical name
        size_t chordPos = jsonResult.find("\"chord\":\"");
        std::string canonicalName = "NOT_FOUND";
        if (chordPos != std::string::npos) {
            size_t start = chordPos + 9;
            size_t end = jsonResult.find("\"", start);
            if (end != std::string::npos) {
                canonicalName = jsonResult.substr(start, end - start);
            }
        }
        
        std::cout << chord << " → canonical: \"" << canonicalName << "\"";
        if (canonicalName == chord) {
            std::cout << " ❌ (returning input!)";
        } else {
            std::cout << " ✅";
        }
        std::cout << "\n";
    }
    
    return 0;
}