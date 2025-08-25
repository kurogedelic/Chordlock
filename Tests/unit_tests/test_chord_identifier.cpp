#include <gtest/gtest.h>
#include "Core/ChordIdentifier.h"
#include <filesystem>

using namespace ChordLock;

class ChordIdentifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        identifier = std::make_unique<ChordIdentifier>(IdentificationMode::STANDARD);
        
        test_yaml_path = "test_chords.yaml";
        
        if (std::filesystem::exists(test_yaml_path)) {
            bool initialized = identifier->initialize(test_yaml_path);
            if (!initialized) {
                GTEST_SKIP() << "Failed to initialize ChordIdentifier";
            }
        } else {
            GTEST_SKIP() << "Test YAML file not found";
        }
    }

    void TearDown() override {
        identifier.reset();
    }

    std::unique_ptr<ChordIdentifier> identifier;
    std::string test_yaml_path;
};

// Test basic chord identification
TEST_F(ChordIdentifierTest, BasicChordIdentification) {
    // C major triad: C4(60), E4(64), G4(67)
    std::vector<int> c_major = {60, 64, 67};
    auto result = identifier->identify(c_major);
    
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.chord_name, "major-triad");
    EXPECT_GT(result.confidence, 0.9f);
    EXPECT_FALSE(result.is_slash_chord);
}

// Test minor chord identification
TEST_F(ChordIdentifierTest, MinorChordIdentification) {
    // C minor triad: C4(60), Eb4(63), G4(67)
    std::vector<int> c_minor = {60, 63, 67};
    auto result = identifier->identify(c_minor);
    
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.chord_name, "minor-triad");
    EXPECT_GT(result.confidence, 0.9f);
}

// Test seventh chord identification
TEST_F(ChordIdentifierTest, SeventhChordIdentification) {
    // C dominant 7th: C4(60), E4(64), G4(67), Bb4(70)
    std::vector<int> c_dom7 = {60, 64, 67, 70};
    auto result = identifier->identify(c_dom7);
    
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.chord_name, "dominant-seventh");
    EXPECT_GT(result.confidence, 0.9f);
}

// Test inversion identification
TEST_F(ChordIdentifierTest, InversionIdentification) {
    identifier->enableInversions(true);
    
    // C major first inversion: E4(64), G4(67), C5(72)
    std::vector<int> c_major_inv = {64, 67, 72};
    auto result = identifier->identify(c_major_inv);
    
    EXPECT_TRUE(result.isValid());
    EXPECT_TRUE(result.is_inversion);
}

// Test different identification modes
TEST_F(ChordIdentifierTest, DifferentModes) {
    std::vector<int> c_major = {60, 64, 67};
    
    // Fast mode
    auto fast_result = identifier->identify(c_major, IdentificationMode::FAST);
    EXPECT_TRUE(fast_result.isValid());
    
    // Comprehensive mode
    auto comp_result = identifier->identify(c_major, IdentificationMode::COMPREHENSIVE);
    EXPECT_TRUE(comp_result.isValid());
    
    // Should find the same chord but possibly with different confidence
    EXPECT_EQ(fast_result.chord_name, comp_result.chord_name);
}

// Test performance
TEST_F(ChordIdentifierTest, Performance) {
    std::vector<int> c_major = {60, 64, 67};
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = identifier->identify(c_major);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete quickly (under 100 microseconds for simple chord)
    EXPECT_LT(duration.count(), 100);
    EXPECT_TRUE(result.isValid());
}

// Test batch processing
TEST_F(ChordIdentifierTest, BatchProcessing) {
    std::vector<std::vector<int>> chord_sequences = {
        {60, 64, 67},      // C major
        {60, 63, 67},      // C minor
        {60, 64, 67, 70}   // C7
    };
    
    auto results = identifier->identifyBatch(chord_sequences);
    
    EXPECT_EQ(results.size(), 3);
    
    for (const auto& result : results) {
        EXPECT_TRUE(result.isValid());
    }
}

// Test unknown chord
TEST_F(ChordIdentifierTest, UnknownChord) {
    // Weird interval pattern not in database
    std::vector<int> unknown = {60, 61, 65, 68};
    auto result = identifier->identify(unknown);
    
    // Should handle gracefully
    EXPECT_FALSE(result.chord_name.empty());
    // Might be "UNKNOWN" or a fuzzy match with low confidence
}

// Test empty input
TEST_F(ChordIdentifierTest, EmptyInput) {
    std::vector<int> empty;
    auto result = identifier->identify(empty);
    
    EXPECT_FALSE(result.isValid());
    EXPECT_EQ(result.chord_name, "INVALID");
}

// Test configuration
TEST_F(ChordIdentifierTest, Configuration) {
    // Test changing confidence threshold
    identifier->setMinConfidenceThreshold(0.8f);
    EXPECT_FLOAT_EQ(identifier->getMinConfidenceThreshold(), 0.8f);
    
    // Test enabling/disabling features
    identifier->enableSlashChords(false);
    identifier->enableInversions(false);
    
    // Should still work with features disabled
    std::vector<int> c_major = {60, 64, 67};
    auto result = identifier->identify(c_major);
    EXPECT_TRUE(result.isValid());
}

// Test performance statistics
TEST_F(ChordIdentifierTest, PerformanceStats) {
    // Reset stats
    identifier->resetPerformanceStats();
    
    // Perform some identifications
    std::vector<int> c_major = {60, 64, 67};
    for (int i = 0; i < 10; ++i) {
        identifier->identify(c_major);
    }
    
    auto stats = identifier->getPerformanceStats();
    EXPECT_EQ(stats.total_identifications, 10);
    EXPECT_GT(stats.average_processing_time.count(), 0);
}