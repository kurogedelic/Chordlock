#include "Chordlock.hpp"
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    Chordlock chordlock;
    
    std::cout << "Chordlock Test Program" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << std::endl;
    
    // Test chord definitions
    struct TestChord {
        std::string name;
        std::vector<int> notes;
    };
    
    std::vector<TestChord> testChords = {
        {"C Major", {60, 64, 67}},          // C E G
        {"C Minor", {60, 63, 67}},          // C Eb G
        {"C7", {60, 64, 67, 70}},           // C E G Bb
        {"CM7", {60, 64, 67, 71}},          // C E G B
        {"Cm7", {60, 63, 67, 70}},          // C Eb G Bb
        {"C/E", {52, 60, 67}},              // E C G (first inversion)
        {"Dm7", {62, 65, 69, 72}},          // D F A C
        {"G7", {55, 59, 62, 65}},           // G B D F
        {"Am", {57, 60, 64}},               // A C E
        {"F", {53, 57, 60}},                // F A C
        {"C9", {60, 64, 67, 70, 74}},       // C E G Bb D
        {"Cadd9", {60, 62, 64, 67}},        // C D E G
        {"Csus4", {60, 65, 67}},            // C F G
        {"C6", {60, 64, 67, 69}},           // C E G A
        {"Cdim", {60, 63, 66}},             // C Eb Gb
        {"Caug", {60, 64, 68}},             // C E G#
    };
    
    // Get current time
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    for (const auto& testChord : testChords) {
        // Clear previous notes
        chordlock.reset();
        
        // Play the chord
        for (int note : testChord.notes) {
            chordlock.noteOn(note, ms);
            chordlock.setVelocity(note, 100);
        }
        
        // Detect the chord
        auto detected = chordlock.detect(ms + 10);
        
        std::cout << "Input: " << testChord.name << " (";
        for (size_t i = 0; i < testChord.notes.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << testChord.notes[i];
        }
        std::cout << ")" << std::endl;
        
        std::cout << "Detected: " << detected.name;
        std::cout << " (confidence: " << detected.confidence << ")" << std::endl;
        
        // Show alternatives
        auto alternatives = chordlock.detectAlternatives(ms + 10, 3);
        if (alternatives.size() > 1) {
            std::cout << "Alternatives: ";
            for (size_t i = 1; i < alternatives.size(); ++i) {
                if (i > 1) std::cout << ", ";
                std::cout << alternatives[i].name << " (" << alternatives[i].confidence << ")";
            }
            std::cout << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "Test with slash chord detection disabled:" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
    
    chordlock.setOnChordDetection(false);
    chordlock.reset();
    
    // Test C/E without slash chord detection
    chordlock.noteOn(52, ms);  // E
    chordlock.noteOn(60, ms);  // C
    chordlock.noteOn(67, ms);  // G
    
    auto detected = chordlock.detect(ms + 10);
    std::cout << "C/E with slash detection OFF: " << detected.name << std::endl;
    
    return 0;
}