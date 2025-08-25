#pragma once

#include <vector>
#include <string>
#include <optional>
#include <map>
#include <cmath>
#include <algorithm>
#include "ChordDatabase.h"

namespace ChordLock {

/**
 * Advanced Chord Recognition System
 * Implements sophisticated algorithms for accurate chord identification
 * Including jazz extensions, altered chords, polychords, and microtonal harmonies
 */
class AdvancedChordRecognition {
public:
    enum class RecognitionMode {
        STRICT,           // Exact match only
        JAZZ,            // Allow common jazz omissions (5th, root)
        CLASSICAL,       // Classical voice leading rules
        CONTEMPORARY,    // Modern/experimental harmony
        WORLD,          // Non-western scales and tunings
        ADAPTIVE        // AI-powered adaptive recognition
    };
    
    struct ExtendedChordInfo {
        std::string primary_chord;
        std::string secondary_chord;  // For polychords
        std::vector<std::string> extensions;
        std::vector<std::string> alterations;
        std::vector<std::string> omissions;
        std::string bass_note;
        std::string modal_context;
        float confidence;
        RecognitionMode mode_used;
        
        // Advanced properties
        bool is_polychord;
        bool is_quartal;
        bool is_cluster;
        bool has_altered_extensions;
        bool is_rootless_voicing;
        bool is_upper_structure;
        float tonal_ambiguity;  // 0=clear tonal center, 1=atonal
        
        std::string getFullName() const;
        std::string getSymbol() const;
    };
    
private:
    std::unique_ptr<ChordDatabase> database_;
    RecognitionMode current_mode_;
    
    // Advanced recognition parameters
    float jazz_omission_threshold_ = 0.7f;
    float voice_leading_weight_ = 0.3f;
    float harmonic_context_weight_ = 0.2f;
    bool allow_enharmonic_equivalence_ = true;
    bool detect_upper_structures_ = true;
    
    // Microtonal support
    float pitch_tolerance_cents_ = 50.0f;  // ±50 cents for microtonal
    bool enable_just_intonation_ = false;
    
    // Statistical learning
    std::map<std::vector<int>, std::string> learned_patterns_;
    std::map<std::string, float> chord_probability_;
    
public:
    AdvancedChordRecognition();
    
    /**
     * Primary recognition method with advanced algorithms
     */
    ExtendedChordInfo recognize(const std::vector<int>& midi_notes, 
                                RecognitionMode mode = RecognitionMode::ADAPTIVE);
    
    /**
     * Recognize with harmonic context (previous and next chords)
     */
    ExtendedChordInfo recognizeInContext(
        const std::vector<int>& midi_notes,
        const std::optional<std::vector<int>>& previous_chord,
        const std::optional<std::vector<int>>& next_chord);
    
    /**
     * Polychord detection (two or more simultaneous chords)
     */
    std::optional<std::pair<ExtendedChordInfo, ExtendedChordInfo>> 
        detectPolychord(const std::vector<int>& midi_notes);
    
    /**
     * Quartal harmony detection
     */
    ExtendedChordInfo detectQuartalHarmony(const std::vector<int>& midi_notes);
    
    /**
     * Modal interchange detection
     */
    std::string detectModalInterchange(const std::vector<int>& midi_notes,
                                      const std::string& key_context);
    
    /**
     * Upper structure triad detection for jazz voicings
     */
    std::optional<std::pair<std::string, std::string>> 
        detectUpperStructure(const std::vector<int>& midi_notes);
    
    /**
     * Microtonal chord recognition (non-12TET)
     */
    ExtendedChordInfo recognizeMicrotonal(const std::vector<float>& frequencies);
    
    /**
     * AI-based fuzzy matching using pattern learning
     */
    ExtendedChordInfo recognizeWithAI(const std::vector<int>& midi_notes);
    
    /**
     * Learn from user corrections
     */
    void learnPattern(const std::vector<int>& midi_notes, 
                      const std::string& correct_chord);
    
    /**
     * Voice leading analysis
     */
    float analyzeVoiceLeading(const std::vector<int>& chord1,
                              const std::vector<int>& chord2);
    
    /**
     * Harmonic function detection
     */
    std::string detectHarmonicFunction(const std::vector<int>& midi_notes,
                                       const std::string& key);
    
    /**
     * Configuration methods
     */
    void setMode(RecognitionMode mode) { current_mode_ = mode; }
    void setJazzOmissionThreshold(float threshold) { jazz_omission_threshold_ = threshold; }
    void setMicrotonalTolerance(float cents) { pitch_tolerance_cents_ = cents; }
    void enableJustIntonation(bool enable) { enable_just_intonation_ = enable; }
    
private:
    // Jazz-specific algorithms
    ExtendedChordInfo recognizeJazzChord(const std::vector<int>& midi_notes);
    std::vector<std::string> detectJazzExtensions(const std::vector<int>& intervals);
    std::vector<std::string> detectAlterations(const std::vector<int>& intervals);
    bool isRootlessVoicing(const std::vector<int>& intervals);
    
    // Classical harmony algorithms
    ExtendedChordInfo recognizeClassicalChord(const std::vector<int>& midi_notes);
    std::string detectCadenceType(const std::vector<int>& chord1,
                                  const std::vector<int>& chord2);
    
    // Contemporary/experimental algorithms
    ExtendedChordInfo recognizeContemporaryChord(const std::vector<int>& midi_notes);
    bool isClusterChord(const std::vector<int>& intervals);
    float calculateTonalAmbiguity(const std::vector<int>& intervals);
    
    // Helper functions
    std::vector<int> extractBassLine(const std::vector<int>& midi_notes);
    std::vector<int> extractUpperVoices(const std::vector<int>& midi_notes);
    std::vector<int> normalizeToRoot(const std::vector<int>& intervals);
    float calculateIntervalTension(int interval);
    
    // Pattern matching with tolerance
    bool fuzzyMatch(const std::vector<int>& pattern1,
                   const std::vector<int>& pattern2,
                   float tolerance);
    
    // Statistical methods
    float calculateChordProbability(const std::string& chord_name,
                                   const std::string& context);
    void updateStatistics(const std::string& chord_name);
};

/**
 * Chord Progression Analyzer
 * Analyzes sequences of chords for musical context
 */
class ChordProgressionAnalyzer {
private:
    std::vector<std::vector<int>> progression_;
    std::string detected_key_;
    std::string detected_mode_;
    
public:
    struct ProgressionAnalysis {
        std::string key;
        std::string mode;
        std::vector<std::string> roman_numerals;
        std::vector<std::string> functions;  // Tonic, Subdominant, Dominant
        std::string progression_type;  // ii-V-I, I-vi-IV-V, etc.
        float tonal_stability;
        bool has_modulation;
        std::vector<std::string> modulation_points;
    };
    
    void addChord(const std::vector<int>& midi_notes);
    void clearProgression();
    
    ProgressionAnalysis analyze();
    std::string detectKey();
    std::vector<std::string> getRomanNumerals();
    std::string identifyProgressionType();
    
    // Advanced analysis
    std::vector<std::string> detectModalInterchanges();
    std::vector<std::string> detectSecondaryDominants();
    std::vector<std::string> detectChromaticMediants();
    float calculateHarmonicRhythm();
    
    // Jazz-specific
    bool isGiantSteps();
    bool isColtrangeChanges();
    bool isRhythmChanges();
    
    // Classical forms
    bool isSonataForm();
    bool isRondo();
    bool isFugue();
};

/**
 * Real-time Chord Tracker
 * For live performance and MIDI input
 */
class RealtimeChordTracker {
private:
    std::vector<int> current_notes_;
    std::chrono::steady_clock::time_point last_change_;
    float stability_threshold_ms_ = 50.0f;
    
    AdvancedChordRecognition recognizer_;
    ChordProgressionAnalyzer analyzer_;
    
public:
    struct ChordEvent {
        std::chrono::steady_clock::time_point timestamp;
        std::vector<int> notes;
        std::string chord_name;
        float confidence;
    };
    
    void noteOn(int midi_note);
    void noteOff(int midi_note);
    void sustainPedal(bool on);
    
    std::optional<ChordEvent> getCurrentChord();
    std::vector<ChordEvent> getChordHistory();
    
    // Live analysis
    std::string getCurrentKey();
    std::string predictNextChord();
    std::vector<std::string> suggestVoicings();
    
    // Performance metrics
    float getTimingAccuracy();
    float getHarmonicComplexity();
};

} // namespace ChordLock