#include <gtest/gtest.h>
#include "Core/ChordDatabase.h"
#include "Core/ChordIdentifier.h"
#include "Utils/MemoryTracker.h"
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

using namespace ChordLock;

class MemoryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& tracker = MemoryTracker::getInstance();
        tracker.setTrackingEnabled(true);
        tracker.clearSnapshots();
        tracker.takeSnapshot("Test setup");
    }
    
    void TearDown() override {
        auto& tracker = MemoryTracker::getInstance();
        
        // Generate memory report
        auto report = tracker.generateMemoryReport();
        if (!report.empty()) {
            std::cout << "\n" << report << std::endl;
        }
    }
};

// Test memory usage during component lifecycle
TEST_F(MemoryIntegrationTest, ComponentLifecycleMemory) {
    auto& tracker = MemoryTracker::getInstance();
    
    tracker.takeSnapshot("Before component creation");
    
    // Create and destroy components multiple times
    for (int i = 0; i < 10; ++i) {
        {
            TRACK_MEMORY_SCOPE("Component lifecycle " + std::to_string(i));
            
            auto database = std::make_unique<ChordDatabase>();
            auto identifier = std::make_unique<ChordIdentifier>();
            
            // Track memory usage
            database->trackMemoryUsage();
            
            // Perform some operations
            std::vector<int> test_chord = {60, 64, 67, 70, 74, 77}; // C11
            auto result = identifier->identify(test_chord);
            EXPECT_TRUE(result.isValid());
            
            if (i == 0) {
                tracker.takeSnapshot("After first component creation");
            }
        }
        
        // Components should be destroyed here
        if (i == 4) {
            tracker.takeSnapshot("After 5 iterations");
        }
    }
    
    tracker.takeSnapshot("After all component destruction");
    
    // Check for memory recovery
    auto snapshots = tracker.getSnapshots();
    ASSERT_GE(snapshots.size(), 3);
    
    size_t initial_memory = snapshots[0].resident_memory_kb;
    size_t peak_memory = snapshots[1].resident_memory_kb;
    size_t final_memory = snapshots.back().resident_memory_kb;
    
    // Memory should not grow excessively
    size_t memory_growth = final_memory - initial_memory;
    EXPECT_LT(memory_growth, 5000) // Less than 5MB permanent growth
        << "Excessive memory growth: " << memory_growth << " KB";
    
    // Calculate recovery rate
    if (peak_memory > initial_memory) {
        float recovery_rate = static_cast<float>(peak_memory - final_memory) / 
                             (peak_memory - initial_memory);
        
        std::cout << "Memory recovery rate: " << (recovery_rate * 100.0f) << "%" << std::endl;
        
        // At least 70% of temporary memory should be recovered
        EXPECT_GT(recovery_rate, 0.7f) 
            << "Poor memory recovery: " << (recovery_rate * 100.0f) << "%";
    }
}

// Test memory usage during high-frequency operations
TEST_F(MemoryIntegrationTest, HighFrequencyOperationMemory) {
    auto& tracker = MemoryTracker::getInstance();
    
    ChordDatabase database;
    ChordIdentifier identifier;
    
    database.trackMemoryUsage();
    tracker.takeSnapshot("After component initialization");
    
    // Perform many chord identifications rapidly
    std::vector<std::vector<int>> test_patterns = {
        {60, 64, 67},           // C major
        {60, 63, 67},           // C minor
        {60, 64, 67, 70},       // C7
        {60, 64, 67, 71},       // CM7
        {60, 63, 67, 70},       // Cm7
        {60, 64, 67, 70, 74},   // C9
        {60, 64, 67, 70, 74, 77}, // C11
        {60, 64, 67, 70, 74, 77, 81}, // C13
    };
    
    // High-frequency identification loop
    for (int iteration = 0; iteration < 1000; ++iteration) {
        for (const auto& pattern : test_patterns) {
            auto result = identifier.identify(pattern);
            TRACK_ALLOCATION("HighFrequency::identify");
            
            // Verify successful identification
            ASSERT_TRUE(result.isValid());
        }
        
        // Take periodic snapshots
        if (iteration % 200 == 0) {
            tracker.takeSnapshot("High frequency iteration " + std::to_string(iteration));
        }
    }
    
    tracker.takeSnapshot("After high frequency operations");
    
    // Analyze memory usage pattern
    auto snapshots = tracker.getSnapshots();
    
    if (snapshots.size() >= 3) {
        size_t start_memory = snapshots[1].resident_memory_kb; // After initialization
        size_t end_memory = snapshots.back().resident_memory_kb;
        
        size_t memory_delta = end_memory - start_memory;
        
        std::cout << "Memory usage during high-frequency operations:" << std::endl;
        std::cout << "  Start: " << start_memory << " KB" << std::endl;
        std::cout << "  End: " << end_memory << " KB" << std::endl;
        std::cout << "  Delta: " << memory_delta << " KB" << std::endl;
        
        // Memory growth should be minimal for high-frequency operations
        EXPECT_LT(memory_delta, 2000) // Less than 2MB growth
            << "High-frequency operations causing memory growth: " << memory_delta << " KB";
    }
    
    // Check for memory leaks
    auto leaks = tracker.detectPotentialLeaks();
    EXPECT_LE(leaks.size(), 2) // Allow up to 2 minor leak warnings
        << "Too many potential memory leaks detected";
}

// Test concurrent memory usage
TEST_F(MemoryIntegrationTest, ConcurrentMemoryUsage) {
    auto& tracker = MemoryTracker::getInstance();
    
    tracker.takeSnapshot("Before concurrent test");
    
    const int num_threads = 4;
    const int iterations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<ChordIdentifier>> identifiers;
    
    // Create identifiers for each thread
    for (int i = 0; i < num_threads; ++i) {
        identifiers.push_back(std::make_unique<ChordIdentifier>());
    }
    
    tracker.takeSnapshot("After identifier creation");
    
    // Launch concurrent chord identification threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            std::vector<std::vector<int>> test_chords = {
                {60, 64, 67},           // C major
                {60, 63, 67},           // C minor
                {60, 64, 67, 70},       // C7
                {60, 64, 67, 70, 74, 77}, // C11
            };
            
            for (int i = 0; i < iterations_per_thread; ++i) {
                for (const auto& chord : test_chords) {
                    auto result = identifiers[t]->identify(chord);
                    TRACK_ALLOCATION("Concurrent::identify");
                    
                    // Basic validation
                    if (!result.isValid()) {
                        std::cerr << "Thread " << t << " failed to identify chord" << std::endl;
                    }
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    tracker.takeSnapshot("After concurrent operations");
    
    // Clean up
    identifiers.clear();
    tracker.takeSnapshot("After concurrent cleanup");
    
    // Analyze concurrent memory usage
    auto snapshots = tracker.getSnapshots();
    ASSERT_GE(snapshots.size(), 3);
    
    size_t before_concurrent = snapshots[snapshots.size()-3].resident_memory_kb;
    size_t during_concurrent = snapshots[snapshots.size()-2].resident_memory_kb;
    size_t after_cleanup = snapshots.back().resident_memory_kb;
    
    std::cout << "Concurrent memory usage:" << std::endl;
    std::cout << "  Before: " << before_concurrent << " KB" << std::endl;
    std::cout << "  During: " << during_concurrent << " KB" << std::endl;
    std::cout << "  After cleanup: " << after_cleanup << " KB" << std::endl;
    
    // Memory should be managed properly during concurrent operations
    size_t concurrent_growth = during_concurrent - before_concurrent;
    size_t cleanup_recovery = during_concurrent - after_cleanup;
    
    EXPECT_LT(concurrent_growth, 20000) // Less than 20MB growth during concurrent ops
        << "Excessive memory growth during concurrent operations: " << concurrent_growth << " KB";
    
    // Most memory should be recovered after cleanup
    if (concurrent_growth > 1000) { // Only check recovery if there was significant growth
        float recovery_rate = static_cast<float>(cleanup_recovery) / concurrent_growth;
        EXPECT_GT(recovery_rate, 0.6f)
            << "Poor memory recovery after concurrent operations: " 
            << (recovery_rate * 100.0f) << "%";
    }
}

// Test memory usage with different chord complexity levels
TEST_F(MemoryIntegrationTest, ChordComplexityMemoryUsage) {
    auto& tracker = MemoryTracker::getInstance();
    
    ChordDatabase database;
    ChordIdentifier identifier;
    database.trackMemoryUsage();
    
    // Test different complexity levels
    struct ComplexityTest {
        std::vector<std::vector<int>> chords;
        std::string description;
    };
    
    std::vector<ComplexityTest> complexity_tests = {
        {
            {{60, 64, 67}, {60, 63, 67}, {60, 64, 68}}, // Simple triads
            "Simple triads"
        },
        {
            {{60, 64, 67, 70}, {60, 63, 67, 70}, {60, 64, 67, 71}}, // Seventh chords
            "Seventh chords"
        },
        {
            {{60, 64, 67, 70, 74}, {60, 63, 67, 70, 74}}, // Ninth chords
            "Ninth chords"
        },
        {
            {{60, 64, 67, 70, 74, 77}, {60, 63, 67, 70, 74, 77}}, // Eleventh chords
            "Eleventh chords"
        },
        {
            {{60, 64, 67, 70, 74, 77, 81}, {60, 63, 67, 70, 74, 77, 81}}, // Thirteenth chords
            "Thirteenth chords"
        }
    };
    
    for (const auto& test : complexity_tests) {
        tracker.takeSnapshot("Before " + test.description);
        
        // Process chords of this complexity level multiple times
        for (int i = 0; i < 500; ++i) {
            for (const auto& chord : test.chords) {
                auto result = identifier.identify(chord);
                TRACK_ALLOCATION("ComplexityTest::" + test.description);
                
                // Verify identification
                EXPECT_TRUE(result.isValid()) 
                    << "Failed to identify chord in " << test.description;
            }
        }
        
        tracker.takeSnapshot("After " + test.description);
    }
    
    // Analyze memory usage across complexity levels
    auto snapshots = tracker.getSnapshots();
    
    std::cout << "Memory usage by chord complexity:" << std::endl;
    for (size_t i = 1; i < snapshots.size(); i += 2) {
        if (i + 1 < snapshots.size()) {
            size_t before = snapshots[i].resident_memory_kb;
            size_t after = snapshots[i + 1].resident_memory_kb;
            size_t delta = after - before;
            
            size_t test_index = (i - 1) / 2;
            if (test_index < complexity_tests.size()) {
                std::cout << "  " << complexity_tests[test_index].description 
                          << ": " << delta << " KB" << std::endl;
                
                // Memory usage should not increase dramatically with complexity
                EXPECT_LT(delta, 1000) // Less than 1MB per complexity level
                    << "Excessive memory usage for " << complexity_tests[test_index].description;
            }
        }
    }
    
    // Check component memory breakdown
    auto components = tracker.getAllComponents();
    std::cout << "\nComponent memory breakdown:" << std::endl;
    for (const auto& component : components) {
        if (component.estimated_size_bytes > 1024) {
            std::cout << "  " << component.component_name 
                      << ": " << (component.estimated_size_bytes / 1024) << " KB" << std::endl;
        }
    }
}

// Test memory efficiency of the new 11th chord implementation
TEST_F(MemoryIntegrationTest, EleventhChordMemoryEfficiency) {
    auto& tracker = MemoryTracker::getInstance();
    
    ChordDatabase database;
    ChordIdentifier identifier;
    
    tracker.takeSnapshot("Before 11th chord test");
    
    // Test various 11th chord patterns
    std::vector<std::vector<int>> eleventh_chords = {
        {60, 64, 67, 70, 74, 77},       // C11 standard
        {60, 64, 67, 70, 74, 77, 81},   // C11 with 13th
        {60, 63, 67, 70, 74, 77},       // Cm11
        {60, 64, 67, 71, 74, 77},       // CM11 (if implemented)
        
        // Different voicings
        {48, 64, 67, 70, 74, 77},       // Wide voicing
        {60, 76, 79, 82, 86, 89},       // High voicing
        {36, 52, 55, 58, 62, 65},       // Low voicing
    };
    
    // Process 11th chords extensively
    for (int iteration = 0; iteration < 200; ++iteration) {
        for (const auto& chord : eleventh_chords) {
            auto result = identifier.identify(chord);
            TRACK_ALLOCATION("EleventhChord::identify");
            
            if (result.isValid()) {
                // Verify it's identified as some form of 11th chord or extended chord
                std::string chord_name = result.chord_name;
                bool is_extended = (chord_name.find("eleventh") != std::string::npos ||
                                  chord_name.find("extended") != std::string::npos ||
                                  chord_name.find("ninth") != std::string::npos ||
                                  result.confidence > 0.7f);
                
                EXPECT_TRUE(is_extended)
                    << "11th chord not properly identified: " << chord_name;
            }
        }
        
        if (iteration % 50 == 0) {
            tracker.takeSnapshot("11th chord iteration " + std::to_string(iteration));
        }
    }
    
    tracker.takeSnapshot("After 11th chord test");
    
    // Analyze memory efficiency
    auto snapshots = tracker.getSnapshots();
    ASSERT_GE(snapshots.size(), 2);
    
    size_t initial_memory = snapshots[0].resident_memory_kb;
    size_t final_memory = snapshots.back().resident_memory_kb;
    size_t memory_growth = final_memory - initial_memory;
    
    std::cout << "11th chord memory efficiency:" << std::endl;
    std::cout << "  Initial memory: " << initial_memory << " KB" << std::endl;
    std::cout << "  Final memory: " << final_memory << " KB" << std::endl;
    std::cout << "  Memory growth: " << memory_growth << " KB" << std::endl;
    
    // Memory growth should be minimal for 11th chord processing
    EXPECT_LT(memory_growth, 500) // Less than 500KB growth
        << "11th chord implementation not memory efficient: " << memory_growth << " KB";
    
    // Check that ChordDatabase memory usage is reasonable
    size_t database_memory = database.getMemoryUsage();
    std::cout << "  ChordDatabase memory: " << (database_memory / 1024) << " KB" << std::endl;
    
    EXPECT_LT(database_memory, 200000) // Less than 200KB for database
        << "ChordDatabase using too much memory: " << (database_memory / 1024) << " KB";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== ChordLock Memory Integration Tests ===" << std::endl;
    
    return RUN_ALL_TESTS();
}