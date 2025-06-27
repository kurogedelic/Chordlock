#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Chordlock Reverse Lookup Test\n";
    std::cout << "=============================\n\n";
    
    // Test chords including theoretical calculation ones
    std::vector<std::string> testChords = {
        "C", "Cmaj", "Cm", "C7", "Cmaj7", 
        "Csus4", "Csus2", "C5", "Caug", "Cdim",
        "Gadd9", "F#m", "C#7", "F#aug", "Dbmaj7"
    };
    
    for (const auto& chordName : testChords) {
        std::cout << "Testing: " << chordName << "\n";
        
        // Test JSON output function
        std::string jsonResult = chordlock.chordNameToNotesJSON(chordName, 4);
        std::cout << "  JSON: " << jsonResult << "\n";
        
        // Test direct notes function  
        auto notes = chordlock.chordNameToNotes(chordName, 4);
        std::cout << "  Notes: [";
        for (size_t i = 0; i < notes.size(); i++) {
            std::cout << notes[i];
            if (i < notes.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n\n";
    }
    
    // Test similar chord names function
    std::cout << "Testing similar chord names for 'F#':\n";
    auto similar = chordlock.findSimilarChordNames("F#");
    std::cout << "  Similar: [";
    for (size_t i = 0; i < similar.size(); i++) {
        std::cout << similar[i];
        if (i < similar.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n\n";
    
    return 0;
}