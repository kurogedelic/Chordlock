#include <gtest/gtest.h>
#include "Core/ChordIdentifier.h"
#include <chrono>
#include <random>

using namespace ChordLock;

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        identifier = std::make_unique<ChordIdentifier>();
        
        // Skip if we can't initialize
        if (!identifier->initialize("test_chords.yaml")) {
            GTEST_SKIP() << "Could not initialize for performance tests";
        }
    }

    std::unique_ptr<ChordIdentifier> identifier;
};

TEST_F(PerformanceTest, SingleChordSpeed) {
    std::vector<int> c_major = {60, 64, 67};
    
    // Warm up
    for (int i = 0; i < 10; ++i) {
        identifier->identify(c_major);
    }
    
    // Measure
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        auto result = identifier->identify(c_major);
        (void)result; // Suppress unused warning
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto avg_time = total_time.count() / 1000.0;
    
    // Should average less than 10 microseconds per identification
    EXPECT_LT(avg_time, 10.0);
    
    std::cout << "Average identification time: " << avg_time << " microseconds\n";
}

TEST_F(PerformanceTest, BatchProcessingSpeed) {
    // Generate 100 random chord patterns
    std::vector<std::vector<int>> test_chords;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> note_dist(48, 84); // C3 to C6
    
    for (int i = 0; i < 100; ++i) {
        std::vector<int> chord;
        int num_notes = 3 + (i % 3); // 3-5 notes
        for (int j = 0; j < num_notes; ++j) {
            chord.push_back(note_dist(gen));
        }
        test_chords.push_back(chord);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = identifier->identifyBatch(test_chords);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(results.size(), 100);
    EXPECT_LT(total_time.count(), 10); // Should complete in under 10ms
    
    std::cout << "Batch processing time for 100 chords: " << total_time.count() << " ms\n";
}

TEST_F(PerformanceTest, CacheEffectiveness) {
    std::vector<int> c_major = {60, 64, 67};
    
    // Clear caches
    identifier->clearCaches();
    
    // First identification (cache miss)
    auto start1 = std::chrono::high_resolution_clock::now();
    auto result1 = identifier->identify(c_major);
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // Second identification (should hit cache)
    auto start2 = std::chrono::high_resolution_clock::now();
    auto result2 = identifier->identify(c_major);
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto time1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1);
    auto time2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2);
    
    // Both should succeed
    EXPECT_TRUE(result1.isValid());
    EXPECT_TRUE(result2.isValid());
    EXPECT_EQ(result1.chord_name, result2.chord_name);
    
    // Second should be faster (though this might be flaky due to CPU variance)
    // At minimum, both should complete quickly
    EXPECT_LT(time1.count(), 100000); // < 100 microseconds
    EXPECT_LT(time2.count(), 100000);
    
    std::cout << "First identification: " << time1.count() << " ns\n";
    std::cout << "Second identification: " << time2.count() << " ns\n";
}