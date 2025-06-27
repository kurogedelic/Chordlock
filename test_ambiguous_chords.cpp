#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "Ambiguous Chord Notation Test\n";
    std::cout << "=============================\n\n";
    
    // Test potentially ambiguous major chord notations
    std::cout << "1️⃣ Major vs Minor confusion:\n";
    std::vector<std::pair<std::string, std::string>> majorMinorTests = {
        {"CM7", "Should be C major 7th [C,E,G,B]"},
        {"Cm7", "Should be C minor 7th [C,Eb,G,Bb]"},
        {"CM", "Should be C major [C,E,G]"},
        {"Cm", "Should be C minor [C,Eb,G]"},
        {"CM9", "Should be C major 9th"},
        {"Cm9", "Should be C minor 9th"},
        {"CM6", "Should be C major 6th"},
        {"Cm6", "Should be C minor 6th"}
    };
    
    for (const auto& test : majorMinorTests) {
        auto notes = chordlock.chordNameToNotes(test.first, 4);
        std::cout << "  " << test.first << ": ";
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "[";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << " - " << test.second << "\n";
    }
    
    // Test add vs sus confusion
    std::cout << "\n2️⃣ Add vs Sus confusion:\n";
    std::vector<std::pair<std::string, std::string>> addSusTests = {
        {"Cadd2", "Should add 2nd: [C,D,E,G]"},
        {"Csus2", "Should suspend 3rd with 2nd: [C,D,G]"},
        {"Cadd4", "Should add 4th: [C,E,F,G]"},
        {"Csus4", "Should suspend 3rd with 4th: [C,F,G]"},
        {"Cadd9", "Should add 9th: [C,E,G,D+octave]"},
        {"Csus9", "Should be sus with 9th (if exists)"}
    };
    
    for (const auto& test : addSusTests) {
        auto notes = chordlock.chordNameToNotes(test.first, 4);
        std::cout << "  " << test.first << ": ";
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "[";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << " - " << test.second << "\n";
    }
    
    // Test diminished variations
    std::cout << "\n3️⃣ Diminished chord variations:\n";
    std::vector<std::pair<std::string, std::string>> dimTests = {
        {"Cdim", "Diminished triad [C,Eb,Gb]"},
        {"Cdim7", "Diminished 7th [C,Eb,Gb,A]"},
        {"Cm7b5", "Half-diminished [C,Eb,Gb,Bb]"},
        {"C°", "Diminished symbol"},
        {"C°7", "Diminished 7th symbol"},
        {"Cø", "Half-diminished symbol"}
    };
    
    for (const auto& test : dimTests) {
        auto notes = chordlock.chordNameToNotes(test.first, 4);
        std::cout << "  " << test.first << ": ";
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "[";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << " - " << test.second << "\n";
    }
    
    // Test augmented variations
    std::cout << "\n4️⃣ Augmented chord variations:\n";
    std::vector<std::pair<std::string, std::string>> augTests = {
        {"Caug", "Augmented triad [C,E,G#]"},
        {"C+", "Augmented symbol"},
        {"C+7", "Augmented 7th"},
        {"C7#5", "Dominant 7th sharp 5"},
        {"Cmaj7#5", "Major 7th sharp 5"}
    };
    
    for (const auto& test : augTests) {
        auto notes = chordlock.chordNameToNotes(test.first, 4);
        std::cout << "  " << test.first << ": ";
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "[";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << " - " << test.second << "\n";
    }
    
    // Test power chord variations
    std::cout << "\n5️⃣ Power chord variations:\n";
    std::vector<std::pair<std::string, std::string>> powerTests = {
        {"C5", "Power chord [C,G]"},
        {"Cno3", "No 3rd (if exists)"},
        {"C(omit3)", "Omit 3rd"},
        {"Csus", "Generic sus (should default to sus4?)"}
    };
    
    for (const auto& test : powerTests) {
        auto notes = chordlock.chordNameToNotes(test.first, 4);
        std::cout << "  " << test.first << ": ";
        if (notes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "[";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << " - " << test.second << "\n";
    }
    
    return 0;
}