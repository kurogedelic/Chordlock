#include "src/Chordlock.hpp"
#include <iostream>

int main() {
    Chordlock chordlock;
    
    std::cout << "=== Debug Cmaj7 Specific Issue ===\n\n";
    
    // Test the problematic case: Cmaj7
    std::string problemChord = "Cmaj7";
    std::cout << "Testing: " << problemChord << "\n";
    
    // Get notes
    auto notes = chordlock.chordNameToNotes(problemChord, 4);
    std::cout << "Notes: [";
    for (size_t i = 0; i < notes.size(); i++) {
        std::cout << notes[i];
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    // Get JSON
    std::string jsonResult = chordlock.chordNameToNotesJSON(problemChord, 4);
    std::cout << "JSON: " << jsonResult << "\n\n";
    
    // The issue is that for some chords, the canonical naming might not be working
    // Let's test a few more problematic cases
    std::vector<std::string> problematicChords = {
        "Cmaj7", "Cmajor7", "C", "Cmajor"
    };
    
    for (const auto& chord : problematicChords) {
        std::string json = chordlock.chordNameToNotesJSON(chord, 4);
        
        // Extract canonical name
        size_t pos = json.find("\"chord\":\"") + 9;
        size_t end = json.find("\"", pos);
        std::string canonical = json.substr(pos, end - pos);
        
        std::cout << chord << " → " << canonical;
        if (canonical == chord) {
            std::cout << " ❌ (same as input - possible fallback)";
        } else {
            std::cout << " ✅ (converted)";
        }
        std::cout << "\n";
    }
    
    return 0;
}