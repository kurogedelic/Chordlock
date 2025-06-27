#include "src/Chordlock.hpp"
#include <iostream>

int main() {
    Chordlock chordlock;
    
    std::cout << "=== Debug Intervals Calculation ===\n\n";
    
    // Test Cmaj7: notes should be [48, 52, 55, 59] = C, E, G, B
    auto notes = chordlock.chordNameToNotes("Cmaj7", 4);
    std::cout << "Cmaj7 notes: [";
    for (size_t i = 0; i < notes.size(); i++) {
        std::cout << notes[i];
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    // Calculate intervals manually
    if (!notes.empty()) {
        int rootNote = notes[0];
        std::vector<int> intervals;
        for (int note : notes) {
            intervals.push_back(note - rootNote);
        }
        
        std::cout << "Calculated intervals: [";
        for (size_t i = 0; i < intervals.size(); i++) {
            std::cout << intervals[i];
            if (i < intervals.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        
        // These should be [0, 4, 7, 11] for Cmaj7
        if (intervals == std::vector<int>{0, 4, 7, 11}) {
            std::cout << "✅ Intervals match Cmaj7 pattern [0, 4, 7, 11]\n";
        } else {
            std::cout << "❌ Intervals do NOT match expected [0, 4, 7, 11]\n";
        }
    }
    
    return 0;
}