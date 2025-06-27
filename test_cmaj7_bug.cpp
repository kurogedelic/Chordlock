#include "src/Chordlock.hpp"
#include <iostream>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Cmaj7 Bug Investigation\n";
    std::cout << "=======================\n\n";
    
    // Test Cmaj7 specifically
    std::cout << "Testing Cmaj7:\n";
    auto notes = chordlock.chordNameToNotes("Cmaj7", 4);
    std::cout << "  Notes: [";
    for (size_t i = 0; i < notes.size(); i++) {
        std::cout << notes[i];
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    // Convert MIDI numbers to note names for verification
    std::cout << "  Note names: [";
    for (size_t i = 0; i < notes.size(); i++) {
        int note = notes[i];
        int octave = note / 12;
        int semitone = note % 12;
        
        std::string noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        std::cout << noteNames[semitone] << octave;
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n\n";
    
    // Test JSON output
    std::string jsonResult = chordlock.chordNameToNotesJSON("Cmaj7", 4);
    std::cout << "JSON: " << jsonResult << "\n\n";
    
    // Test variations
    std::vector<std::string> variants = {"CM7", "Cmaj7", "Cmajor7", "CmajMaj7"};
    
    std::cout << "Testing Cmaj7 variations:\n";
    for (const auto& variant : variants) {
        auto variantNotes = chordlock.chordNameToNotes(variant, 4);
        std::cout << "  " << variant << ": [";
        for (size_t i = 0; i < variantNotes.size(); i++) {
            std::cout << variantNotes[i];
            if (i < variantNotes.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
    
    // Expected: Cmaj7 = C-E-G-B = [48, 52, 55, 59] in octave 4
    std::cout << "\nExpected Cmaj7 (C-E-G-B): [48, 52, 55, 59]\n";
    
    return 0;
}