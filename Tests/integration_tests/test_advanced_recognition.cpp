#include <gtest/gtest.h>
#include "../../Core/AdvancedChordRecognition.h"
#include <vector>
#include <string>

using namespace ChordLock;

class AdvancedChordRecognitionTest : public ::testing::Test {
protected:
    void SetUp() override {
        recognizer = std::make_unique<AdvancedChordRecognition>();
    }
    
    std::unique_ptr<AdvancedChordRecognition> recognizer;
};

// ========== Jazz Chord Tests ==========

TEST_F(AdvancedChordRecognitionTest, RecognizeJazzNinthChord) {
    // C9 chord: C E G Bb D
    std::vector<int> notes = {60, 64, 67, 70, 62};  // C E G Bb D
    
    auto result = recognizer->recognize(notes, AdvancedChordRecognition::RecognitionMode::JAZZ);
    
    EXPECT_EQ(result.primary_chord, "dominant-seventh");
    EXPECT_TRUE(std::find(result.extensions.begin(), result.extensions.end(), "9") != result.extensions.end());
    EXPECT_GT(result.confidence, 0.8f);
}

TEST_F(AdvancedChordRecognitionTest, RecognizeAlteredDominant) {
    // C7#9b5: C E Gb Bb D#
    std::vector<int> notes = {60, 64, 66, 70, 63};  // C E Gb Bb Eb
    
    auto result = recognizer->recognize(notes, AdvancedChordRecognition::RecognitionMode::JAZZ);
    
    EXPECT_EQ(result.primary_chord, "dominant-seventh");
    EXPECT_TRUE(std::find(result.alterations.begin(), result.alterations.end(), "b5") != result.alterations.end());
    EXPECT_TRUE(std::find(result.extensions.begin(), result.extensions.end(), "#9") != result.extensions.end());
    EXPECT_TRUE(result.has_altered_extensions);
}

TEST_F(AdvancedChordRecognitionTest, RecognizeRootlessVoicing) {
    // Rootless Cmaj7: E G B (no C)
    std::vector<int> notes = {64, 67, 71};  // E G B
    
    auto result = recognizer->recognize(notes, AdvancedChordRecognition::RecognitionMode::JAZZ);
    
    EXPECT_TRUE(result.is_rootless_voicing);
    EXPECT_GT(result.confidence, 0.7f);
}

// ========== Quartal Harmony Tests ==========

TEST_F(AdvancedChordRecognitionTest, RecognizeQuartalChord) {
    // Quartal voicing: C F Bb Eb
    std::vector<int> notes = {60, 65, 70, 75};  // Stacked fourths
    
    auto result = recognizer->detectQuartalHarmony(notes);
    
    EXPECT_TRUE(result.is_quartal);
    EXPECT_EQ(result.primary_chord, "quartal-voicing");
    EXPECT_GT(result.confidence, 0.8f);
}

TEST_F(AdvancedChordRecognitionTest, RecognizeSoWhatChord) {
    // "So What" chord: D G C F A
    std::vector<int> notes = {62, 67, 72, 65, 69};  // D G C F A
    
    auto result = recognizer->detectQuartalHarmony(notes);
    
    EXPECT_EQ(result.primary_chord, "so-what-chord");
    EXPECT_GT(result.confidence, 0.9f);
}

// ========== Polychord Tests ==========

TEST_F(AdvancedChordRecognitionTest, RecognizePolychord) {
    // C major over D major: C E G | D F# A
    std::vector<int> notes = {60, 64, 67, 74, 78, 81};  // C E G | D F# A (higher octave)
    
    auto polychord = recognizer->detectPolychord(notes);
    
    ASSERT_TRUE(polychord.has_value());
    EXPECT_TRUE(polychord->first.is_polychord);
    EXPECT_TRUE(polychord->second.is_polychord);
    EXPECT_GT(polychord->first.confidence, 0.7f);
    EXPECT_GT(polychord->second.confidence, 0.7f);
}

// ========== Microtonal Tests ==========

TEST_F(AdvancedChordRecognitionTest, RecognizeQuarterToneChord) {
    // Quarter-tone chord with microtonal intervals
    std::vector<float> frequencies = {261.63f, 277.18f, 329.63f, 349.23f};  // C, C#, E, F with quarter-tones
    
    auto result = recognizer->recognizeMicrotonal(frequencies);
    
    EXPECT_EQ(result.primary_chord, "quarter-tone-chord");
    EXPECT_EQ(result.modal_context, "microtonal");
    EXPECT_GT(result.confidence, 0.7f);
}

TEST_F(AdvancedChordRecognitionTest, RecognizeJustIntonationChord) {
    // Just intonation major triad
    std::vector<float> frequencies = {261.63f, 327.03f, 392.44f};  // C (1:1), E (5:4), G (3:2)
    
    auto result = recognizer->recognizeMicrotonal(frequencies);
    
    EXPECT_EQ(result.primary_chord, "just-intonation-chord");
    EXPECT_EQ(result.modal_context, "just-intonation");
    EXPECT_GT(result.confidence, 0.8f);
}

// ========== Context-Based Recognition Tests ==========

TEST_F(AdvancedChordRecognitionTest, RecognizeWithContext) {
    // ii-V-I progression context
    std::vector<int> current = {60, 64, 67, 70};  // C7
    std::vector<int> previous = {62, 65, 69, 72};  // Dm7
    std::vector<int> next = {65, 69, 72, 76};  // Fmaj7
    
    auto result = recognizer->recognizeInContext(current, previous, next);
    
    // Should have higher confidence due to good voice leading
    EXPECT_GT(result.confidence, 0.85f);
}

// ========== Modal Interchange Tests ==========

TEST_F(AdvancedChordRecognitionTest, DetectModalInterchange) {
    // bIII chord in C major (Eb major borrowed from C minor)
    std::vector<int> notes = {63, 67, 70};  // Eb G Bb
    
    std::string interchange = recognizer->detectModalInterchange(notes, "C major");
    
    EXPECT_EQ(interchange, "borrowed-from-parallel-minor");
}

// ========== Cluster Chord Tests ==========

TEST_F(AdvancedChordRecognitionTest, RecognizeClusterChord) {
    // Chromatic cluster: C C# D D# E
    std::vector<int> notes = {60, 61, 62, 63, 64};
    
    auto result = recognizer->recognize(notes, AdvancedChordRecognition::RecognitionMode::CONTEMPORARY);
    
    EXPECT_TRUE(result.is_cluster);
    EXPECT_GT(result.tonal_ambiguity, 0.7f);
}

// ========== Upper Structure Tests ==========

TEST_F(AdvancedChordRecognitionTest, DetectUpperStructureTriad) {
    // C7 with D major upper structure
    std::vector<int> notes = {60, 64, 67, 70, 74, 78, 81};  // C E G Bb | D F# A
    
    auto upper = recognizer->detectUpperStructure(notes);
    
    ASSERT_TRUE(upper.has_value());
    EXPECT_FALSE(upper->first.empty());
    EXPECT_FALSE(upper->second.empty());
}

// ========== AI Learning Tests ==========

TEST_F(AdvancedChordRecognitionTest, LearnNewPattern) {
    // Teach the system a new chord pattern
    std::vector<int> custom_notes = {60, 65, 71};  // Custom voicing
    std::string custom_name = "custom-sus-voicing";
    
    recognizer->learnPattern(custom_notes, custom_name);
    
    // Now it should recognize the pattern
    auto result = recognizer->recognizeWithAI(custom_notes);
    
    EXPECT_EQ(result.primary_chord, custom_name);
    EXPECT_GT(result.confidence, 0.85f);
}

// ========== Voice Leading Tests ==========

TEST_F(AdvancedChordRecognitionTest, AnalyzeVoiceLeading) {
    // Good voice leading: C major to F major
    std::vector<int> chord1 = {60, 64, 67};  // C E G
    std::vector<int> chord2 = {60, 65, 69};  // C F A
    
    float quality = recognizer->analyzeVoiceLeading(chord1, chord2);
    
    EXPECT_GT(quality, 0.7f);  // Good voice leading should score high
}

// ========== Harmonic Function Tests ==========

TEST_F(AdvancedChordRecognitionTest, DetectHarmonicFunction) {
    // G7 in key of C (dominant function)
    std::vector<int> notes = {67, 71, 74, 77};  // G B D F
    
    std::string function = recognizer->detectHarmonicFunction(notes, "C");
    
    EXPECT_EQ(function, "Dominant");
}

// ========== Adaptive Mode Tests ==========

TEST_F(AdvancedChordRecognitionTest, AdaptiveRecognition) {
    // Complex chord that might be interpreted differently
    std::vector<int> notes = {60, 64, 67, 70, 74};  // C E G Bb D
    
    auto result = recognizer->recognize(notes, AdvancedChordRecognition::RecognitionMode::ADAPTIVE);
    
    // Should successfully recognize as some form of C9
    EXPECT_FALSE(result.primary_chord.empty());
    EXPECT_GT(result.confidence, 0.7f);
    EXPECT_EQ(result.mode_used, AdvancedChordRecognition::RecognitionMode::ADAPTIVE);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}