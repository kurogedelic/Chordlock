#include "Chordlock.hpp"
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <vector>

/**
 * @brief WebAssembly bindings for Chordlock
 * 
 * Provides JavaScript-compatible interface for the enhanced chord detection engine
 */

// Global Chordlock instance for WebAssembly
static std::unique_ptr<Chordlock> g_chordlock = nullptr;

extern "C" {

// Core functionality
EMSCRIPTEN_KEEPALIVE
void chordlock_init() {
    Chordlock::EngineConfiguration config;
    config.velocitySensitive = true;
    config.slashChordDetection = true;
    config.advancedAlternatives = true;
    config.maxAlternatives = 5;
    config.confidenceThreshold = 0.3f;
    
    g_chordlock = std::make_unique<Chordlock>(config);
}

EMSCRIPTEN_KEEPALIVE
void chordlock_cleanup() {
    g_chordlock.reset();
}

EMSCRIPTEN_KEEPALIVE
void chordlock_note_on(int midiNote, int velocity) {
    if (g_chordlock && midiNote >= 0 && midiNote < 128) {
        g_chordlock->noteOn(static_cast<uint8_t>(midiNote), static_cast<uint8_t>(velocity));
    }
}

EMSCRIPTEN_KEEPALIVE
void chordlock_note_off(int midiNote) {
    if (g_chordlock && midiNote >= 0 && midiNote < 128) {
        g_chordlock->noteOff(static_cast<uint8_t>(midiNote));
    }
}

EMSCRIPTEN_KEEPALIVE
void chordlock_clear_all_notes() {
    if (g_chordlock) {
        g_chordlock->clearAllNotes();
    }
}

EMSCRIPTEN_KEEPALIVE
const char* chordlock_detect_chord() {
    if (!g_chordlock) {
        return "{}";
    }
    
    static std::string result;
    result = g_chordlock->detectChordJSON();
    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* chordlock_detect_chord_detailed(int maxCandidates) {
    if (!g_chordlock) {
        return "{}";
    }
    
    auto detectionResult = g_chordlock->detectChordWithDetailedAnalysis(maxCandidates);
    
    static std::string result;
    result = "{";
    result += "\"chord\": \"" + detectionResult.chordName + "\",";
    result += "\"confidence\": " + std::to_string(detectionResult.confidence) + ",";
    result += "\"detectionTime\": " + std::to_string(detectionResult.detectionTimeMs) + ",";
    result += "\"hasValidChord\": ";
    result += detectionResult.hasValidChord ? "true" : "false";
    result += ",";
    result += "\"mask\": " + std::to_string(detectionResult.pitchMask) + ",";
    
    // Add detailed candidates
    result += "\"detailedCandidates\": [";
    for (size_t i = 0; i < detectionResult.detailedCandidates.size(); i++) {
        const auto& candidate = detectionResult.detailedCandidates[i];
        result += "{";
        result += "\"name\": \"" + candidate.name + "\",";
        result += "\"confidence\": " + std::to_string(candidate.confidence) + ",";
        result += "\"root\": \"" + candidate.root + "\",";
        result += "\"isInversion\": ";
        result += candidate.isInversion ? "true" : "false";
        result += ",";
        result += "\"inversionDegree\": " + std::to_string(candidate.inversionDegree) + ",";
        result += "\"interpretationType\": \"" + candidate.interpretationType + "\",";
        result += "\"matchScore\": " + std::to_string(candidate.matchScore) + ",";
        
        // Missing notes
        result += "\"missingNotes\": [";
        for (size_t j = 0; j < candidate.missingNotes.size(); j++) {
            result += std::to_string(candidate.missingNotes[j]);
            if (j < candidate.missingNotes.size() - 1) result += ",";
        }
        result += "],";
        
        // Extra notes
        result += "\"extraNotes\": [";
        for (size_t j = 0; j < candidate.extraNotes.size(); j++) {
            result += std::to_string(candidate.extraNotes[j]);
            if (j < candidate.extraNotes.size() - 1) result += ",";
        }
        result += "]";
        
        result += "}";
        if (i < detectionResult.detailedCandidates.size() - 1) result += ",";
    }
    result += "],";
    
    // Traditional alternatives for backward compatibility
    result += "\"alternatives\": [";
    for (size_t i = 0; i < detectionResult.alternativeChords.size(); i++) {
        result += "\"" + detectionResult.alternativeChords[i] + "\"";
        if (i < detectionResult.alternativeChords.size() - 1) result += ",";
    }
    result += "]";
    
    result += "}";
    
    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* chordlock_get_version() {
    if (!g_chordlock) {
        return "1.0.0";
    }
    
    static std::string version;
    version = g_chordlock->getEngineVersion();
    return version.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* chordlock_get_statistics() {
    if (!g_chordlock) {
        return "{}";
    }
    
    static std::string stats;
    stats = g_chordlock->getStatisticsJSON();
    return stats.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* chordlock_get_engine_info() {
    if (!g_chordlock) {
        return "Chordlock not initialized";
    }
    
    static std::string info;
    info = g_chordlock->getEngineInfo();
    return info.c_str();
}

EMSCRIPTEN_KEEPALIVE
void chordlock_set_notes_from_json(const char* jsonNotes) {
    if (g_chordlock && jsonNotes) {
        g_chordlock->setNotesFromJSON(std::string(jsonNotes));
    }
}

EMSCRIPTEN_KEEPALIVE
void chordlock_set_velocity_sensitivity(int enabled) {
    if (g_chordlock) {
        g_chordlock->setVelocitySensitivity(enabled != 0);
    }
}

EMSCRIPTEN_KEEPALIVE
void chordlock_set_slash_chord_detection(int enabled) {
    if (g_chordlock) {
        g_chordlock->setSlashChordDetection(enabled != 0);
    }
}

EMSCRIPTEN_KEEPALIVE
void chordlock_set_confidence_threshold(float threshold) {
    if (g_chordlock) {
        g_chordlock->setConfidenceThreshold(threshold);
    }
}

EMSCRIPTEN_KEEPALIVE
void chordlock_set_key_signature(int keySignature) {
    if (g_chordlock) {
        g_chordlock->setKeySignature(keySignature);
    }
}

EMSCRIPTEN_KEEPALIVE
int chordlock_is_chord_active() {
    if (!g_chordlock) {
        return 0;
    }
    
    return g_chordlock->isChordActive() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int chordlock_get_current_mask() {
    if (!g_chordlock) {
        return 0;
    }
    
    return static_cast<int>(g_chordlock->getCurrentPitchMask());
}

EMSCRIPTEN_KEEPALIVE
float chordlock_calculate_complexity() {
    if (!g_chordlock) {
        return 0.0f;
    }
    
    return g_chordlock->calculateChordComplexity();
}

// Batch processing for performance
EMSCRIPTEN_KEEPALIVE
void chordlock_set_chord_from_array(int* midiNotes, int noteCount, int velocity) {
    if (!g_chordlock || !midiNotes || noteCount <= 0) {
        return;
    }
    
    std::vector<int> notes;
    for (int i = 0; i < noteCount; i++) {
        notes.push_back(midiNotes[i]);
    }
    
    g_chordlock->setChordFromMidiNotes(notes, static_cast<uint8_t>(velocity));
}

// Test functions for validation
EMSCRIPTEN_KEEPALIVE
const char* chordlock_test_dominant_11th() {
    if (!g_chordlock) {
        return "Engine not initialized";
    }
    
    // Test C11 chord (C-E-G-Bb-D-F)
    g_chordlock->setChordFromMidiNotes({48, 52, 55, 58, 62, 65});
    
    static std::string result;
    result = g_chordlock->detectChordJSON();
    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* chordlock_benchmark_performance(int iterations) {
    if (!g_chordlock || iterations <= 0) {
        return "{\"error\": \"Invalid parameters\"}";
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        // Test different chord combinations
        g_chordlock->setChordFromMidiNotes({60 + (i % 12), 64 + (i % 12), 67 + (i % 12)});
        g_chordlock->detectChord();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    static std::string result;
    result = "{\"iterations\": " + std::to_string(iterations) + 
             ", \"totalTime\": " + std::to_string(duration.count()) + 
             ", \"averageTime\": " + std::to_string(duration.count() / static_cast<double>(iterations)) + 
             ", \"throughput\": " + std::to_string(iterations * 1000000.0 / duration.count()) + "}";
    
    return result.c_str();
}

} // extern "C"

// Emscripten bindings for modern JavaScript (optional, but nice to have)
EMSCRIPTEN_BINDINGS(chordlock_v3) {
    emscripten::function("init", &chordlock_init);
    emscripten::function("cleanup", &chordlock_cleanup);
    emscripten::function("noteOn", &chordlock_note_on);
    emscripten::function("noteOff", &chordlock_note_off);
    emscripten::function("clearAllNotes", &chordlock_clear_all_notes);
    emscripten::function("detectChord", &chordlock_detect_chord, emscripten::allow_raw_pointers());
    emscripten::function("detectChordDetailed", &chordlock_detect_chord_detailed, emscripten::allow_raw_pointers());
    emscripten::function("getVersion", &chordlock_get_version, emscripten::allow_raw_pointers());
    emscripten::function("getStatistics", &chordlock_get_statistics, emscripten::allow_raw_pointers());
    emscripten::function("getEngineInfo", &chordlock_get_engine_info, emscripten::allow_raw_pointers());
    emscripten::function("setNotesFromJSON", &chordlock_set_notes_from_json, emscripten::allow_raw_pointers());
    emscripten::function("setVelocitySensitivity", &chordlock_set_velocity_sensitivity);
    emscripten::function("setSlashChordDetection", &chordlock_set_slash_chord_detection);
    emscripten::function("setConfidenceThreshold", &chordlock_set_confidence_threshold);
    emscripten::function("setKeySignature", &chordlock_set_key_signature);
    emscripten::function("isChordActive", &chordlock_is_chord_active);
    emscripten::function("getCurrentMask", &chordlock_get_current_mask);
    emscripten::function("calculateComplexity", &chordlock_calculate_complexity);
    emscripten::function("testDominant11th", &chordlock_test_dominant_11th, emscripten::allow_raw_pointers());
    emscripten::function("benchmarkPerformance", &chordlock_benchmark_performance, emscripten::allow_raw_pointers());
}