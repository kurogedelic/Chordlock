/**
 * ChordLock CLI Example
 * Simple command-line interface for chord identification
 */

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>

#include "Core/ChordIdentifier.h"
#include "Core/ChordNameGenerator.h"
#include "Utils/NoteConverter.h"

using namespace ChordLock;

void printUsage() {
    std::cout << "🎵 ChordLock CLI - Ultra-fast chord identification\n\n";
    std::cout << "Usage:\n";
    std::cout << "  chordlock_cli <midi_notes>    # Identify chord from MIDI notes\n";
    std::cout << "  chordlock_cli --help          # Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  chordlock_cli 60,64,67        # C Major\n";
    std::cout << "  chordlock_cli 60,63,67        # C Minor\n";
    std::cout << "  chordlock_cli 60,64,67,70     # C7\n";
    std::cout << "  chordlock_cli 64,67,72        # C/E (first inversion)\n\n";
}

std::vector<int> parseNotes(const std::string& input) {
    std::vector<int> notes;
    std::stringstream ss(input);
    std::string note;
    
    while (std::getline(ss, note, ',')) {
        try {
            int midi_note = std::stoi(note);
            if (midi_note >= 0 && midi_note <= 127) {
                notes.push_back(midi_note);
            }
        } catch (const std::exception&) {
            // Skip invalid notes
        }
    }
    
    return notes;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return 1;
    }
    
    std::string input = argv[1];
    
    if (input == "--help" || input == "-h") {
        printUsage();
        return 0;
    }
    
    // Parse MIDI notes
    auto notes = parseNotes(input);
    if (notes.empty()) {
        std::cout << "❌ Error: No valid MIDI notes found\n";
        std::cout << "   Expected format: 60,64,67\n";
        return 1;
    }
    
    try {
        // Initialize ChordLock
        ChordIdentifier identifier(IdentificationMode::STANDARD);
        identifier.initialize("", "");  // Use compiled tables
        
        NoteConverter converter(AccidentalStyle::SHARPS);
        
        // Measure performance
        auto start = std::chrono::high_resolution_clock::now();
        auto result = identifier.identify(notes);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double microseconds = duration.count() / 1000.0;
        
        // Display results
        std::cout << "🎹 Input Notes: ";
        for (size_t i = 0; i < notes.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << converter.midiToNoteName(notes[i], AccidentalStyle::SHARPS, OctaveNotation::NO_OCTAVE);
            std::cout << "(" << notes[i] << ")";
        }
        std::cout << "\n\n";
        
        std::cout << "🎵 Chord: " << result.full_display_name << "\n";
        std::cout << "📊 Confidence: " << (result.confidence * 100) << "%\n";
        std::cout << "⚡ Processing Time: " << microseconds << "μs\n";
        
        if (result.is_slash_chord) {
            std::cout << "🔄 Type: Slash chord (inversion)\n";
        }
        
        if (!result.note_names.empty()) {
            std::cout << "🎼 Note Names: ";
            for (size_t i = 0; i < result.note_names.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << result.note_names[i];
            }
            std::cout << "\n";
        }
        
        if (!result.identified_intervals.empty()) {
            std::cout << "📐 Intervals: ";
            for (size_t i = 0; i < result.identified_intervals.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << result.identified_intervals[i];
            }
            std::cout << "\n";
        }
        
        std::cout << "\n✨ ChordLock v2.0.0 - Ultra-fast chord identification\n";
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}