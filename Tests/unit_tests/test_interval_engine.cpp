#include <gtest/gtest.h>
#include "Core/IntervalEngine.h"
#include <vector>
#include <algorithm>

using namespace ChordLock;

class IntervalEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<IntervalEngine>();
    }

    void TearDown() override {
        engine.reset();
    }

    std::unique_ptr<IntervalEngine> engine;
};

// Test basic interval calculation
TEST_F(IntervalEngineTest, BasicIntervalCalculation) {
    // C major triad: C4(60), E4(64), G4(67)
    std::vector<int> notes = {60, 64, 67};
    auto result = engine->calculateIntervals(notes);
    
    EXPECT_FALSE(result.intervals.empty());
    EXPECT_EQ(result.bass_note, 60);
    
    // Expected intervals: [0, 4, 7]
    std::vector<int> expected = {0, 4, 7};
    EXPECT_EQ(result.intervals, expected);
}

// Test interval calculation with different octaves
TEST_F(IntervalEngineTest, DifferentOctaves) {
    // C major triad spread across octaves: C3(48), E4(64), G5(79)
    std::vector<int> notes = {48, 64, 79};
    auto result = engine->calculateIntervals(notes);
    
    EXPECT_EQ(result.bass_note, 48);
    
    // Should still resolve to [0, 4, 7] regardless of octave
    std::vector<int> expected = {0, 4, 7};
    EXPECT_EQ(result.intervals, expected);
}

// Test inversion detection
TEST_F(IntervalEngineTest, InversionDetection) {
    // C major first inversion: E4(64), G4(67), C5(72)
    std::vector<int> notes = {64, 67, 72};
    auto result = engine->calculateIntervals(notes);
    
    EXPECT_EQ(result.bass_note, 64); // E is bass
    EXPECT_TRUE(result.has_inversion);
    
    // Bass is E, so intervals from E: [0, 3, 8]
    std::vector<int> expected = {0, 3, 8};
    EXPECT_EQ(result.intervals, expected);
}

// Test duplicate note removal
TEST_F(IntervalEngineTest, DuplicateRemoval) {
    // C major with duplicate C: C4(60), C4(60), E4(64), G4(67)
    std::vector<int> notes = {60, 60, 64, 67};
    auto result = engine->calculateIntervals(notes);
    
    // Should remove duplicate C
    std::vector<int> expected = {0, 4, 7};
    EXPECT_EQ(result.intervals, expected);
}

// Test empty input
TEST_F(IntervalEngineTest, EmptyInput) {
    std::vector<int> empty_notes;
    auto result = engine->calculateIntervals(empty_notes);
    
    EXPECT_TRUE(result.intervals.empty());
    EXPECT_EQ(result.bass_note, -1);
}

// Test invalid MIDI notes
TEST_F(IntervalEngineTest, InvalidMidiNotes) {
    std::vector<int> invalid_notes = {-1, 128, 200};
    
    EXPECT_FALSE(engine->validateInput(invalid_notes));
}

// Test valid MIDI note range
TEST_F(IntervalEngineTest, ValidMidiRange) {
    std::vector<int> valid_notes = {0, 64, 127};
    
    EXPECT_TRUE(engine->validateInput(valid_notes));
}

// Test single note
TEST_F(IntervalEngineTest, SingleNote) {
    std::vector<int> single_note = {60};
    auto result = engine->calculateIntervals(single_note);
    
    EXPECT_EQ(result.intervals.size(), 1);
    EXPECT_EQ(result.intervals[0], 0);
    EXPECT_EQ(result.bass_note, 60);
}

// Test chromatic intervals
TEST_F(IntervalEngineTest, ChromaticIntervals) {
    // All 12 chromatic notes from C4
    std::vector<int> chromatic = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};
    auto result = engine->calculateIntervals(chromatic);
    
    // Expected: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
    std::vector<int> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    EXPECT_EQ(result.intervals, expected);
}

// Test specified bass note
TEST_F(IntervalEngineTest, SpecifiedBass) {
    // C major triad: C4(60), E4(64), G4(67)
    std::vector<int> notes = {60, 64, 67};
    
    // Force E4(64) as bass
    auto result = engine->calculateIntervals(notes, 64);
    
    EXPECT_EQ(result.bass_note, 64);
    
    // Intervals from E: E(0), G(3), C(8)
    std::vector<int> expected = {0, 3, 8};
    EXPECT_EQ(result.intervals, expected);
}

// Test normalization to octave
TEST_F(IntervalEngineTest, NormalizeToOctave) {
    std::vector<int> notes = {60, 72, 84}; // C4, C5, C6
    auto normalized = engine->normalizeToOctave(notes);
    
    // All should become C (0)
    std::vector<int> expected = {0, 0, 0};
    EXPECT_EQ(normalized, expected);
}

// Test sort and deduplicate
TEST_F(IntervalEngineTest, SortAndDeduplicate) {
    std::vector<int> unsorted = {67, 60, 64, 60, 67}; // G, C, E, C, G
    auto sorted = engine->sortAndDeduplicate(unsorted);
    
    std::vector<int> expected = {60, 64, 67}; // C, E, G
    EXPECT_EQ(sorted, expected);
}

// Test chord span calculation
TEST_F(IntervalEngineTest, ChordSpan) {
    std::vector<int> wide_chord = {48, 64, 79}; // C3 to G5
    auto span = engine->getChordSpan(wide_chord);
    
    EXPECT_EQ(span, 31); // 79 - 48 = 31 semitones
}

// Test interval class calculation
TEST_F(IntervalEngineTest, IntervalClass) {
    EXPECT_EQ(IntervalEngine::getIntervalClass(60), 0);  // C
    EXPECT_EQ(IntervalEngine::getIntervalClass(61), 1);  // C#
    EXPECT_EQ(IntervalEngine::getIntervalClass(72), 0);  // C (octave higher)
    EXPECT_EQ(IntervalEngine::getIntervalClass(127), 7); // G
}

// Test octave calculation
TEST_F(IntervalEngineTest, OctaveCalculation) {
    EXPECT_EQ(IntervalEngine::getOctave(60), 4);   // C4
    EXPECT_EQ(IntervalEngine::getOctave(48), 3);   // C3
    EXPECT_EQ(IntervalEngine::getOctave(72), 5);   // C5
    EXPECT_EQ(IntervalEngine::getOctave(127), 9);  // G9
}

// Test transposition
TEST_F(IntervalEngineTest, Transposition) {
    std::vector<int> major_triad = {0, 4, 7};
    auto transposed = IntervalEngine::transposeIntervals(major_triad, 2);
    
    // Transpose up 2 semitones (whole step)
    std::vector<int> expected = {2, 6, 9};
    EXPECT_EQ(transposed, expected);
}

// Test transposition with wrap-around
TEST_F(IntervalEngineTest, TranspositionWrapAround) {
    std::vector<int> intervals = {10, 11}; // Bb, B
    auto transposed = IntervalEngine::transposeIntervals(intervals, 3);
    
    // Should wrap around: 10+3=13%12=1, 11+3=14%12=2
    std::vector<int> expected = {1, 2};
    EXPECT_EQ(transposed, expected);
}

// Performance test for large input
TEST_F(IntervalEngineTest, PerformanceLargeInput) {
    // Test with maximum allowed notes (16)
    std::vector<int> large_input;
    for (int i = 60; i < 76; ++i) {
        large_input.push_back(i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = engine->calculateIntervals(large_input);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete in reasonable time (less than 100 microseconds)
    EXPECT_LT(duration.count(), 100);
    EXPECT_FALSE(result.intervals.empty());
}

// Test with boundary MIDI values
TEST_F(IntervalEngineTest, BoundaryValues) {
    std::vector<int> boundary_notes = {0, 127}; // Lowest and highest MIDI
    auto result = engine->calculateIntervals(boundary_notes);
    
    EXPECT_EQ(result.bass_note, 0);
    EXPECT_EQ(result.intervals.size(), 2);
    EXPECT_EQ(result.intervals[0], 0);
    EXPECT_EQ(result.intervals[1], 7); // 127 % 12 = 7 (G)
}