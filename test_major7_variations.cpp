#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Major 7th Chord Variations Test\n";
    std::cout << "===============================\n\n";
    
    // Test all major 7th variations
    std::vector<std::string> major7Chords = {
        "CM7", "Cmaj7", "CMaj7", "Cmajor7", "CMajor7",
        "DM7", "Dmaj7", "EM7", "Emaj7", "FM7", "Fmaj7",
        "GM7", "Gmaj7", "AM7", "Amaj7", "BM7", "Bmaj7"
    };
    
    std::cout << "Testing major 7th chords (should all be [root, major3rd, 5th, major7th]):\n";
    
    for (const auto& chord : major7Chords) {
        auto notes = chordlock.chordNameToNotes(chord, 4);
        std::cout << "  " << chord << ": ";
        
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "✅ [";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
            
            // Verify it's actually a major 7th chord
            if (notes.size() == 4) {
                int intervals[3] = {notes[1] - notes[0], notes[2] - notes[1], notes[3] - notes[2]};
                bool isCorrect = (intervals[0] == 4 && intervals[1] == 3 && intervals[2] == 4);
                std::cout << (isCorrect ? " ✓" : " ❌ Wrong intervals");
            }
        }
        std::cout << "\n";
    }
    
    // Test minor 7th to ensure we didn't break it
    std::cout << "\nTesting minor 7th chords (should be [root, minor3rd, 5th, minor7th]):\n";
    std::vector<std::string> minor7Chords = {"Cm7", "Dm7", "Em7", "Fm7", "Gm7", "Am7", "Bm7"};
    
    for (const auto& chord : minor7Chords) {
        auto notes = chordlock.chordNameToNotes(chord, 4);
        std::cout << "  " << chord << ": ";
        
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "✅ [";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
            
            // Verify it's actually a minor 7th chord
            if (notes.size() == 4) {
                int intervals[3] = {notes[1] - notes[0], notes[2] - notes[1], notes[3] - notes[2]};
                bool isCorrect = (intervals[0] == 3 && intervals[1] == 4 && intervals[2] == 3);
                std::cout << (isCorrect ? " ✓" : " ❌ Wrong intervals");
            }
        }
        std::cout << "\n";
    }
    
    return 0;
}