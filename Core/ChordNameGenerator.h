#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "ChordDatabase.h"
#include "../Utils/NoteConverter.h"

namespace ChordLock {

enum class NamingStyle {
    JAZZ,       // C, Dm7, G7alt
    CLASSICAL,  // C, d, G7
    POPULAR,    // C, Dm, G7
    MINIMAL     // C, D-, G7
};

enum class KeyContext {
    C_MAJOR,     // Prefer naturals and sharps
    F_MAJOR,     // Prefer flats
    G_MAJOR,     // Prefer sharps
    AUTO_DETECT, // Analyze context automatically
    CHROMATIC    // No key preference
};

struct ChordNameResult {
    std::string chord_name;        // "Dm7"
    std::string root_note;         // "D"
    std::string chord_symbol;      // "m7"
    std::string bass_note;         // "F" (for slash chords)
    std::string full_name;         // "Dm7/F"
    bool is_slash_chord;
    int inversion_type;            // 0=root, 1=1st, 2=2nd, etc.
    float confidence;
    
    ChordNameResult() : is_slash_chord(false), inversion_type(0), confidence(0.0f) {}
};

class ChordNameGenerator {
private:
    std::unique_ptr<NoteConverter> note_converter;
    NamingStyle current_style;
    KeyContext current_key_context;
    
    // Chord symbol mapping tables
    static const std::unordered_map<std::string, std::string>& getJazzSymbols();
    static const std::unordered_map<std::string, std::string>& getClassicalSymbols();
    static const std::unordered_map<std::string, std::string>& getPopularSymbols();
    
    // Root detection algorithms
    int detectTheoreticalRoot(const std::vector<int>& intervals, const std::string& chord_type) const;
    int detectRootFromIntervalPattern(const std::vector<int>& intervals) const;
    int detectRootFromBassAndChordType(int bass_note, const std::string& chord_type, const std::vector<int>& intervals) const;
    
    // Key context analysis
    KeyContext analyzeKeyContext(const std::vector<int>& midi_notes) const;
    AccidentalStyle getAccidentalStyleForKey(KeyContext key) const;
    
    // Chord symbol generation
    std::string generateChordSymbol(const std::string& chord_type, NamingStyle style) const;
    std::string formatChordName(const std::string& root_note, const std::string& symbol, 
                               const std::string& bass_note = "") const;
    
    // Inversion analysis
    int analyzeInversion(const std::vector<int>& intervals, const std::string& chord_type) const;
    bool shouldUseSlashNotation(const std::string& chord_type, int inversion, int bass_interval) const;
    
    // Advanced root detection using music theory
    struct RootCandidate {
        int midi_note;
        float confidence;
        std::string reasoning;
    };
    
    std::vector<RootCandidate> findRootCandidates(const std::vector<int>& midi_notes, 
                                                 const std::vector<int>& intervals,
                                                 const std::string& chord_type) const;
    
public:
    ChordNameGenerator(NamingStyle style = NamingStyle::JAZZ, 
                      KeyContext key = KeyContext::AUTO_DETECT);
    ~ChordNameGenerator() = default;
    
    // Main API
    ChordNameResult generateChordName(const ChordMatch& match,
                                     const std::vector<int>& midi_notes,
                                     const std::vector<int>& intervals) const;
    
    ChordNameResult generateChordName(const std::string& chord_type,
                                     const std::vector<int>& midi_notes,
                                     const std::vector<int>& intervals,
                                     int bass_note = -1) const;
    
    // Configuration
    void setNamingStyle(NamingStyle style) { current_style = style; }
    NamingStyle getNamingStyle() const { return current_style; }
    
    void setKeyContext(KeyContext key) { current_key_context = key; }
    KeyContext getKeyContext() const { return current_key_context; }
    
    // Utility methods
    std::string getChordSymbol(const std::string& chord_type) const;
    std::vector<std::string> getAllPossibleNames(const ChordMatch& match,
                                                const std::vector<int>& midi_notes) const;
    
    // Analysis helpers
    bool isValidChordName(const std::string& chord_name) const;
    std::string normalizeChordName(const std::string& chord_name) const;
    
    // Performance methods
    void warmupCache() const;
    size_t getCacheSize() const;
};

} // namespace ChordLock