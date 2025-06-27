#include "src/Chordlock.hpp"
#include <iostream>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "F#aug Specific Test\n";
    std::cout << "==================\n\n";
    
    // Test F#aug directly
    std::cout << "Testing F#aug:\n";
    auto notes = chordlock.chordNameToNotes("F#aug", 4);
    std::cout << "  Direct notes: [";
    for (size_t i = 0; i < notes.size(); i++) {
        std::cout << notes[i];
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    // Test JSON output
    std::string jsonResult = chordlock.chordNameToNotesJSON("F#aug", 4);
    std::cout << "  JSON: " << jsonResult << "\n\n";
    
    // Test variations
    std::vector<std::string> variants = {"F#aug", "F# aug", "F#Aug", "Gbaug"};
    
    std::cout << "Testing F#aug variations:\n";
    for (const auto& variant : variants) {
        auto variantNotes = chordlock.chordNameToNotes(variant, 4);
        std::cout << "  " << variant << ": ";
        if (variantNotes.empty()) {
            std::cout << "❌ Not found";
        } else {
            std::cout << "✅ [";
            for (size_t i = 0; i < variantNotes.size(); i++) {
                std::cout << variantNotes[i];
                if (i < variantNotes.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << "\n";
    }
    
    return 0;
}