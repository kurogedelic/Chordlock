#pragma once

#include <unordered_map>
#include <vector>
#include <string>

namespace ChordLock {

// Compiled chord dictionary from interval_dict.yaml
// This eliminates runtime YAML parsing for maximum performance

struct ChordEntry {
    std::vector<int> intervals;
    std::string name;
    std::string category;
};

class CompiledChordDatabase {
public:
    static const std::unordered_map<std::vector<int>, std::string>& getChordMap();
    static const std::vector<ChordEntry>& getAllChords();
    
private:
    static void initializeChordMap();
    static std::unordered_map<std::vector<int>, std::string> chord_map;
    static std::vector<ChordEntry> all_chords;
    static bool initialized;
};

// Hash function for vector<int> keys
struct VectorHash {
    std::size_t operator()(const std::vector<int>& v) const {
        std::size_t seed = v.size();
        for (auto& i : v) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

// Static chord definitions compiled from YAML
const std::pair<std::vector<int>, const char*> COMPILED_CHORDS[] = {
    // Two-note structures
    {{0,2}, "sus2-dyad"},
    {{0,3}, "minor-third"},
    {{0,4}, "major-third"},
    {{0,5}, "perfect-fourth"},
    {{0,6}, "tritone"},
    {{0,7}, "power-chord"},
    {{0,8}, "minor-sixth"},
    {{0,9}, "major-sixth"},
    {{0,10}, "minor-seventh"},
    {{0,11}, "major-seventh"},

    // Three-note structures
    {{0,1,4}, "phrygian-trichord"},
    {{0,1,5}, "hijaz-fragment"},
    {{0,1,2}, "chromatic-3-cluster"},
    {{0,2,3}, "chromatic-cluster"},
    {{0,2,4}, "whole-tone-cluster"},
    {{0,2,5}, "pentatonic-cluster"},
    {{0,2,6}, "lydian-trichord"},
    {{0,2,7}, "sus2-triad"},
    {{0,3,5}, "minor-cluster"},
    {{0,3,6}, "diminished-triad"},
    {{0,3,7}, "minor-triad"},
    {{0,3,8}, "minor-sharp5"},
    {{0,4,6}, "major-flat5"},
    {{0,4,7}, "major-triad"},
    {{0,4,8}, "augmented-triad"},
    {{0,4,10}, "italian-sixth"},
    {{0,5,7}, "sus4-triad"},
    {{0,5,9}, "quartal-sus"},
    {{0,5,10}, "quartal-triad"},
    {{0,7,12}, "power-chord-octave"},

    // Modern contemporary harmony
    {{0,1,5}, "microtonal-cluster"},
    {{0,2,6}, "quartal-modern"},
    {{0,6,11}, "tritone-major7"},

    // Four-note structures
    {{0,1,4,5}, "hijaz-tetrachord"},
    {{0,1,5,8}, "phrygian-b2-chord"},
    {{0,2,3,5}, "rast-tetrachord"},
    {{0,2,5,7}, "pentatonic-seventh"},
    {{0,2,6,9}, "major-add9-slash"},
    {{0,2,6,10}, "french-sixth"},
    {{0,2,7,10}, "7sus2"},

    // Contemporary four-note harmonies
    {{0,1,6,7}, "semitone-tritone-cluster"},
    {{0,2,5,8}, "minor-major-quartal"},
    {{0,3,6,11}, "diminished-major7"},
    {{0,1,7,8}, "chromatic-fifth-cluster"},
    {{0,6,7,11}, "tritone-fifth-major7"},
    
    {{0,3,5,7}, "minor-pentatonic"},
    {{0,3,6,9}, "diminished-seventh"},
    {{0,3,6,10}, "half-diminished-seventh"},
    {{0,3,7,10}, "minor-seventh"},
    {{0,3,7,11}, "minor-major-seventh"},
    {{0,2,3,7}, "minor-add9"},
    {{0,3,8,10}, "minor-seventh-sharp5"},
    {{0,4,6,10}, "dominant-seventh-flat5"},
    {{0,4,6,11}, "major-seventh-flat5"},
    {{0,4,7,9}, "major-sixth"},
    {{0,4,7,10}, "dominant-seventh"},
    {{0,4,7,11}, "major-seventh"},
    {{0,2,4,7}, "add9"},
    {{0,4,5,7}, "add11"},
    {{0,4,8,10}, "dominant-seventh-sharp5"},
    {{0,4,8,11}, "major-seventh-sharp5"},
    {{0,5,6,10}, "quartal-augmented"},
    {{0,5,7,10}, "7sus4"},
    {{0,5,9,12}, "quartal-ninth"},
    {{0,5,10,15}, "quartal-fourth"},

    // Five-note structures
    {{0,1,4,5,7}, "hijaz-pentachord"},
    {{0,2,5,7,9}, "pentatonic-scale"},
    {{0,3,5,6,7}, "blues-pentachord"},
    {{0,3,5,7,10}, "minor-pentatonic-seventh"},
    {{0,3,6,7,10}, "blues-scale-fragment"},
    {{0,3,6,10,14}, "minor-ninth-flat5"},
    {{0,2,3,7,10}, "minor-ninth"},
    {{0,4,6,10,1}, "dominant-seventh-flat5-flat9"},
    {{0,4,7,9,14}, "six-nine"},
    {{0,1,4,7,10}, "dominant-seventh-flat9"},
    {{0,2,4,7,10}, "dominant-ninth"},
    {{0,3,4,7,10}, "dominant-seventh-sharp9"},
    {{0,4,6,7,10}, "dominant-seventh-sharp11"},
    {{0,4,7,8,10}, "dominant-seventh-flat13"},
    {{0,2,4,7,11}, "major-ninth"},
    {{0,4,7,11,18}, "major-seventh-sharp11"},
    {{0,3,4,8,10}, "dominant-seventh-sharp5-sharp9"},
    {{0,5,7,10,14}, "9sus4"},
    {{0,5,10,15,19}, "so-what-chord"},
    {{0,5,10,15,20}, "quartal-fifth"},

    // Six-note structures  
    {{0,2,4,6,8,10}, "whole-tone-scale"},
    {{0,3,5,6,7,10}, "blues-scale"},
    {{0,3,6,10,14,17}, "minor-eleventh-flat5"},
    {{0,2,3,5,7,10}, "minor-eleventh"},
    {{0,2,3,7,9,10}, "minor-thirteenth"},
    {{0,1,4,6,7,10}, "dominant-seventh-flat9-sharp11"},
    {{0,2,4,5,7,10}, "dominant-eleventh"},
    {{0,4,5,7,10}, "dominant-eleventh-standard"},
    {{0,2,4,7,9,10}, "dominant-thirteenth"},
    {{0,3,4,7,8,10}, "dominant-seventh-sharp9-flat13"},
    {{0,2,4,5,7,11}, "major-eleventh"},
    {{0,2,4,7,9,11}, "major-thirteenth"},
    {{0,2,5,7,9,10}, "13sus4"},

    // Advanced contemporary six-note structures
    {{0,1,5,6,7,11}, "super-locrian-hexachord"},
    {{0,2,4,6,9,11}, "lydian-augmented-sixth"},
    {{0,1,3,5,8,10}, "octatonic-fragment"},

    // Seven-note structures
    {{0,1,3,4,6,8,10}, "locrian-scale"},
    {{0,1,3,5,6,8,10}, "phrygian-scale"},
    {{0,1,3,5,7,8,10}, "pelog-approximation"},
    {{0,1,4,5,7,8,11}, "hijaz-scale"},
    {{0,2,3,5,7,8,10}, "dorian-scale"},
    {{0,2,3,5,7,9,10}, "natural-minor-scale"},
    {{0,2,4,5,7,9,10}, "mixolydian-scale"},
    {{0,2,4,5,7,9,11}, "major-scale"},
    {{0,2,4,6,7,9,11}, "lydian-scale"},

    // Extended and complex structures
    {{0,1,2,3,4,5,6,7,8,9,10,11}, "chromatic-scale"},
    {{0,2,4,6,8,10,12,14,16,18,20,22}, "whole-tone-extended"},
    {{0,3,5,6,7,9,10,12,15}, "bebop-scale"},
    {{0,3,5,7,10,12,15,17,19}, "extended-blues"},
    {{0,4,7,10,13,16,18,20,23}, "altered-scale"},
    
    // Missing challenging chord types
    {{0,1,2,3}, "chromatic-cluster"},
    {{0,1,2,3,4}, "chromatic-cluster"}, 
    {{0,2,4,6,8}, "whole-tone-chord"},
    {{0,4,8,12}, "augmented-triad-extended"},
    {{0,6,12}, "tritone-or-flat5"},
    {{0,4,5,7}, "major-slash-non-chord-tone"},
    {{0,7,14,19}, "quartal-displaced"},
    {{0,4,11,16}, "quintal-structure"},
    {{0,5,9,14,19}, "sus4-add9-add13"},
    {{0,2,7,11,16}, "sus2-major7-add11"},
    
    // Extended eleventh chords with proper voicings
    {{0,7,10,14,18}, "dominant-eleventh"},
    {{0,7,11,14,18}, "major-eleventh"}, 
    {{0,6,10,14,17}, "minor-eleventh-flat5"},
    
    // Thirteenth chords with complete voicings
    {{0,4,7,9,10,14}, "dominant-thirteenth"},        // C7(9,13) - C,E,G,A,Bb,D
    {{0,7,9,10,14,18}, "dominant-thirteenth"},       // C13 full voicing
    {{0,3,7,9,10,14}, "minor-thirteenth"},           // Cm13 voicing
    {{0,4,7,9,11,14}, "major-thirteenth"},           // CM13 voicing
    {{0,10,14,18,21}, "dominant-seventh-with-upper-structure"},
    {{0,11,15,19,22}, "major-seventh-with-upper-structure"},
    
    // Complex modern jazz voicings
    {{0,4,7,11,14,18}, "major-ninth-sharp11"},
    {{0,3,7,10,14,17}, "minor-ninth-flat13"},
    {{0,4,7,10,13,17,20}, "dominant-altered-scale"},
    
    // Additional extended chord voicings
    {{0,4,7,10,14,17}, "dominant-eleventh-sharp11"},  // C7(9,#11)
    {{0,4,7,9,10,17}, "dominant-sharp11-thirteenth"}, // C7(#11,13)
    {{0,4,7,9,10,13}, "dominant-flat9-sharp13"},     // C7(b9,#13)
    {{0,3,7,9,10,13}, "minor-flat9-sharp13"},        // Cm7(b9,#13)
    {{0,3,7,10,13,17}, "minor-flat9-sharp11"},        // Cm7(b9,#11)
    {{0,4,7,11,14,17}, "major-ninth-add11"},          // CM9(add11)
    {{0,4,9,10,14}, "dominant-ninth-omit5-add13"},   // C7(9,13,omit5)
    {{0,4,7,10,17}, "dominant-sharp11"},              // C7#11
    {{0,4,7,9,10}, "dominant-thirteenth-omit9"},     // C7(13,omit9)
    
    // Critical altered chord patterns for Section 6 improvement
    {{0,1,4,6,7,10}, "dominant-seventh-flat9-sharp11"},   // C7(b9,#11)
    {{0,1,4,7,8,10}, "dominant-seventh-flat9-sharp13"},   // C7(b9,#13)  
    {{0,1,3,4,7,10}, "dominant-seventh-flat9-sharp9"},    // C7(b9,#9)
    {{0,3,4,6,7,10}, "dominant-seventh-sharp9-sharp11"},  // C7(#9,#11)
    {{0,3,4,7,8,10}, "dominant-seventh-sharp9-sharp13"},  // C7(#9,#13)
    {{0,1,4,6,10}, "dominant-seventh-flat5-flat9-sharp11"}, // C7(b5,b9,#11)
    {{0,1,4,8,10}, "dominant-seventh-sharp5-flat9-sharp11"}, // C7(#5,b9,#11)
    {{0,3,4,6,10}, "dominant-seventh-flat5-sharp9-sharp11"}, // C7(b5,#9,#11)
    
    // Missing chord patterns found in testing
    {{0,4,8,10}, "dominant-seventh-sharp5"},              // C7#5
    {{0,4,5,7}, "add11"},                                 // Cadd11
    {{0,4,6,7,10}, "dominant-seventh-sharp11-clean"},     // C7#11
    {{0,4,7,10,18}, "dominant-seventh-sharp11"},         // C7#11 (5-note)
    {{0}, "single-note"},                                 // Single note
    {{0,12}, "octave"},                                   // Octave interval
    
    // Sus4 and sixth chord variations
    {{0,5,7,14}, "sus4-add9"},                        // Csus4add9 - C,F,G,D
    {{0,4,7,9,14}, "six-nine"},                       // C6/9 proper - C,E,G,A,D
    {{0,3,7,9,14}, "minor-six-nine"},                 // Cm6/9 - C,Eb,G,A,D
    
    // Wide voicing major triads (normalized)
    {{0,4,7}, "major-triad"}, // Will catch wide voicings after normalization
    
    // Polychord approximations
    {{0,7,11,14}, "polychord"},
    {{0,1,5,8}, "polychord"}, 
    {{0,5,9,12}, "polychord"},
    
    // Advanced jazz harmony extensions  
    {{0,2,4,5,7,9,11}, "major-thirteenth-add11"},
    {{0,2,3,5,7,9,10}, "minor-thirteenth-add11"},
    {{0,1,3,4,6,7,10}, "altered-dominant-scale-fragment"},
    {{0,1,4,6,8,10}, "tritone-substitution-chord"},
    {{0,2,4,6,9,10}, "lydian-dominant-fragment"},
    {{0,1,3,6,8,10}, "diminished-whole-tone"},
    {{0,3,6,8,10}, "diminished-dominant"},
    {{0,2,5,8,10}, "quartal-dominant"},
    {{0,3,5,8,11}, "minor-major-ninth-sharp11"},
    
    // Bebop harmony
    {{0,1,4,5,8,10}, "bebop-dominant-fragment"},
    {{0,2,3,6,8,10}, "bebop-minor-fragment"},
    {{0,2,4,5,8,11}, "bebop-major-fragment"},
    {{0,1,3,5,6,9,10}, "bebop-blues-scale"},
    {{0,2,3,5,6,8,10}, "harmonic-minor-bebop"},
    {{0,1,4,6,7,9,10}, "altered-bebop-scale"},
    
    // Modern jazz voicings
    {{0,4,6,10}, "tritone-sub-shell"},
    {{0,3,6,9}, "symmetric-diminished"},
    {{0,2,6,8}, "spread-triad"},
    {{0,5,10}, "so-what-voicing"},
    {{0,7,10}, "upper-structure"},
    {{0,2,7}, "quartal-voicing-basic"},
    {{0,5,10,15}, "quartal-stack"},
    {{0,3,7,10,14}, "minor-eleventh-no-five"},
    
    // Contemporary clusters
    {{0,1,5,6}, "minor-second-cluster"},
    {{0,6,7}, "tritone-cluster"},
    {{0,1,11}, "chromatic-edge-cluster"},
    {{0,2,3,4}, "chromatic-tetrachord"},
    {{0,1,6,11}, "symmetric-cluster"},
    
    // Microtonal and quarter-tone approximations (12-TET approximations)
    {{0,1,3}, "quarter-tone-triad"},
    {{0,2,5}, "quarter-tone-minor"},
    {{0,3,5}, "quarter-tone-neutral"},
    {{0,1,6}, "quarter-tone-augmented"},
    {{0,1,7}, "quarter-tone-seventh"},
    {{0,2,8}, "quarter-tone-wide-third"},
    {{0,1,4,8}, "quarter-tone-tetrachord"},
    {{0,1,5,9}, "quarter-tone-spread"},
    {{0,2,3,6}, "microtonal-tetrachord"},
    {{0,1,2,5}, "microtonal-cluster-wide"},
    
    // Extended microtonal structures
    {{0,1,4,5,9}, "microtonal-pentachord"},
    {{0,1,3,5,7}, "microtonal-scale-fragment"},
    {{0,2,3,5,8}, "microtonal-pentatonic"},
    {{0,1,2,4,6,8}, "microtonal-hexachord"}
};

const size_t COMPILED_CHORDS_COUNT = sizeof(COMPILED_CHORDS) / sizeof(COMPILED_CHORDS[0]);

// Chord aliases
const std::pair<const char*, const char*> CHORD_ALIASES[] = {
    // Major triads
    {"major-triad", "M"},
    {"major-triad", "maj"},
    {"major-triad", ""},
    
    // Minor triads  
    {"minor-triad", "m"},
    {"minor-triad", "min"},
    {"minor-triad", "-"},
    
    // Sevenths
    {"dominant-seventh", "7"},
    {"major-seventh", "M7"},
    {"major-seventh", "maj7"},
    {"major-seventh", "Δ7"},
    {"minor-seventh", "m7"},
    {"minor-seventh", "min7"},
    {"minor-seventh", "-7"},
    
    // Diminished/Augmented
    {"diminished-triad", "dim"},
    {"diminished-triad", "°"},
    {"augmented-triad", "aug"},
    {"augmented-triad", "+"},
    
    // Suspended
    {"sus4-triad", "sus4"},
    {"sus4-triad", "sus"},
    {"sus2-triad", "sus2"},
    
    // Extended
    {"dominant-ninth", "9"},
    {"major-ninth", "M9"},
    {"minor-ninth", "m9"},
    {"dominant-eleventh", "11"},
    {"dominant-thirteenth", "13"}
};

const size_t CHORD_ALIASES_COUNT = sizeof(CHORD_ALIASES) / sizeof(CHORD_ALIASES[0]);

} // namespace ChordLock