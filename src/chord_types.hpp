#pragma once
#include <string>
#include <cstdint>
#include <vector>

/**
 * Chord extensions and alterations
 */
struct ChordExtensions {
    std::vector<std::string> addTones;      // ["add9", "add11", "add13"]
    std::vector<std::string> alterations;   // ["#5", "b9", "#9", "b13"]
    std::vector<std::string> suspensions;   // ["sus2", "sus4"]
    
    bool hasExtensions() const {
        return !addTones.empty() || !alterations.empty() || !suspensions.empty();
    }
    
    std::string formatExtensions() const {
        std::string result;
        for (const auto& sus : suspensions) {
            result += sus;
        }
        for (const auto& add : addTones) {
            result += "(" + add + ")";
        }
        for (const auto& alt : alterations) {
            result += "(" + alt + ")";
        }
        return result;
    }
};

/**
 * Basic chord candidate structure
 */
struct ChordCandidate {
    std::string name;
    uint16_t mask;
    float confidence;
    ChordExtensions extensions;
    
    ChordCandidate() : mask(0), confidence(0.0f) {}
    ChordCandidate(const std::string& n, uint16_t m, float c) 
        : name(n), mask(m), confidence(c) {}
    ChordCandidate(const std::string& n, uint16_t m, float c, const ChordExtensions& ext) 
        : name(n), mask(m), confidence(c), extensions(ext) {}
    
    std::string getDisplayName() const {
        if (extensions.hasExtensions()) {
            return name + extensions.formatExtensions();
        }
        return name;
    }
    
    bool operator<(const ChordCandidate& other) const {
        return confidence > other.confidence; // Higher confidence first
    }
};

/**
 * Key context for enhanced chord analysis
 */
struct KeyContext {
    int tonicPitch;          // Tonic note (0-11, -1 = no key)
    bool isMinor;           // Major/minor key
    std::vector<int> scale; // Scale tones
    
    KeyContext() : tonicPitch(-1), isMinor(false) {}
    
    KeyContext(int tonic, bool minor = false) : tonicPitch(tonic), isMinor(minor) {
        generateScale();
    }
    
    bool isSet() const { return tonicPitch >= 0; }
    
    bool isScaleTone(int pitch) const {
        if (!isSet()) return false;
        for (int scaleTone : scale) {
            if (pitch % 12 == scaleTone) return true;
        }
        return false;
    }
    
    float getScaleRelevance(uint16_t mask) const {
        if (!isSet()) return 0.0f;
        int scaleNotes = 0;
        int totalNotes = __builtin_popcount(mask);
        
        for (int pitch = 0; pitch < 12; pitch++) {
            if (mask & (1 << pitch) && isScaleTone(pitch)) {
                scaleNotes++;
            }
        }
        
        return totalNotes > 0 ? (float)scaleNotes / (float)totalNotes : 0.0f;
    }
    
    std::string getChordFunction(const std::string& chordName) const {
        if (!isSet()) return "";
        
        // Parse root from chord name
        int chordRoot = parseChordRoot(chordName);
        if (chordRoot < 0) return "";
        
        // Calculate interval from tonic
        int interval = (chordRoot - tonicPitch + 12) % 12;
        
        // Check for secondary dominants (V7/x)
        if (chordName.find("7") != std::string::npos && !isMinor) {
            // In major keys, check for common secondary dominants
            if (interval == 0) return "V7/IV";  // C7 in C major = V7/IV
            if (interval == 2) return "V7/V";   // D7 in C major = V7/V  
            if (interval == 4) return "V7/vi";  // E7 in C major = V7/vi
            if (interval == 9) return "V7/ii";  // A7 in C major = V7/ii
            if (interval == 11) return "V7/iii"; // B7 in C major = V7/iii
        }
        
        // Roman numeral analysis
        const char* majorFunctions[] = {"I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII"};
        const char* minorFunctions[] = {"i", "bII", "ii", "bIII", "III", "iv", "bV", "v", "bVI", "VI", "bVII", "VII"};
        
        if (isMinor) {
            return minorFunctions[interval];
        } else {
            return majorFunctions[interval];
        }
    }
    
private:
    void generateScale() {
        scale.clear();
        if (tonicPitch < 0) return;
        
        if (isMinor) {
            // Natural minor scale intervals: 0, 2, 3, 5, 7, 8, 10
            int intervals[] = {0, 2, 3, 5, 7, 8, 10};
            for (int interval : intervals) {
                scale.push_back((tonicPitch + interval) % 12);
            }
        } else {
            // Major scale intervals: 0, 2, 4, 5, 7, 9, 11
            int intervals[] = {0, 2, 4, 5, 7, 9, 11};
            for (int interval : intervals) {
                scale.push_back((tonicPitch + interval) % 12);
            }
        }
    }
    
    int parseChordRoot(const std::string& chordName) const {
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        const char* altNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
        
        for (int i = 0; i < 12; i++) {
            if (chordName.find(noteNames[i]) == 0 || chordName.find(altNames[i]) == 0) {
                return i;
            }
        }
        return -1;
    }
};