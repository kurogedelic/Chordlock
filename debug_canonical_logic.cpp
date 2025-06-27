#include "src/Chordlock.hpp"
#include <iostream>

// Test the canonical naming logic step by step
int main() {
    Chordlock chordlock;
    
    std::cout << "=== Debug Canonical Logic ===\n\n";
    
    // Test CM7 specifically
    std::string testChord = "CM7";
    std::cout << "Testing: " << testChord << "\n";
    
    // Get the notes first
    auto notes = chordlock.chordNameToNotes(testChord, 4);
    std::cout << "Notes found: " << (notes.empty() ? "NO" : "YES") << "\n";
    if (!notes.empty()) {
        std::cout << "Notes: [";
        for (size_t i = 0; i < notes.size(); i++) {
            std::cout << notes[i];
            if (i < notes.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
    
    // Test the JSON result
    std::string jsonResult = chordlock.chordNameToNotesJSON(testChord, 4);
    std::cout << "JSON result: " << jsonResult << "\n\n";
    
    // Extract the chord field
    size_t chordPos = jsonResult.find("\"chord\":\"");
    if (chordPos != std::string::npos) {
        size_t start = chordPos + 9;
        size_t end = jsonResult.find("\"", start);
        if (end != std::string::npos) {
            std::string chordValue = jsonResult.substr(start, end - start);
            std::cout << "Extracted chord field: \"" << chordValue << "\"\n";
            
            if (chordValue == testChord) {
                std::cout << "❌ PROBLEM: Chord field contains original input, not canonical name!\n";
            } else {
                std::cout << "✅ Good: Chord field contains canonical name\n";
            }
        }
    }
    
    return 0;
}