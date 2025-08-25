#include <gtest/gtest.h>
#include "Core/ChordDatabase.h"
#include "Core/ChordIdentifier.h"
#include "Utils/MemoryTracker.h"
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>

using namespace ChordLock;

class PerformanceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        database_ = std::make_unique<ChordDatabase>();
        identifier_ = std::make_unique<ChordIdentifier>();
        
        // Pre-warm the system
        std::vector<int> warmup_chord = {60, 64, 67};
        for (int i = 0; i < 10; ++i) {
            identifier_->identify(warmup_chord);
        }
    }
    
    std::unique_ptr<ChordDatabase> database_;
    std::unique_ptr<ChordIdentifier> identifier_;
    
    // Helper function to measure execution time
    template<typename Func>
    double measureExecutionTime(Func&& func, int iterations = 1000) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        return static_cast<double>(duration.count()) / iterations / 1000.0; // Return microseconds
    }
    
    // Statistical analysis helper
    struct PerformanceStats {
        double mean;
        double median;
        double std_dev;
        double min_time;
        double max_time;
        double p95;
        double p99;
    };
    
    PerformanceStats analyzePerformance(std::vector<double>& times) {
        std::sort(times.begin(), times.end());
        
        PerformanceStats stats;
        stats.mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        stats.median = times[times.size() / 2];
        stats.min_time = times.front();
        stats.max_time = times.back();
        stats.p95 = times[static_cast<size_t>(times.size() * 0.95)];
        stats.p99 = times[static_cast<size_t>(times.size() * 0.99)];
        
        // Calculate standard deviation
        double variance = 0.0;
        for (double time : times) {
            variance += (time - stats.mean) * (time - stats.mean);
        }
        stats.std_dev = std::sqrt(variance / times.size());
        
        return stats;
    }
};

// Test basic chord identification performance
TEST_F(PerformanceIntegrationTest, BasicChordPerformance) {
    struct PerformanceTest {
        std::vector<int> chord;
        std::string name;
        double max_avg_time_us;
    };
    
    std::vector<PerformanceTest> tests = {
        {{60, 64, 67}, "Major triad", 15.0},
        {{60, 63, 67}, "Minor triad", 15.0},
        {{60, 64, 67, 70}, "Dominant 7th", 20.0},
        {{60, 64, 67, 71}, "Major 7th", 20.0},
        {{60, 64, 67, 70, 74}, "Dominant 9th", 25.0},
        {{60, 64, 67, 70, 74, 77}, "Dominant 11th", 30.0},
    };
    
    for (const auto& test : tests) {
        std::vector<double> times;
        times.reserve(1000);
        
        // Measure individual execution times
        for (int i = 0; i < 1000; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto result = identifier_->identify(test.chord);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            times.push_back(static_cast<double>(duration.count()) / 1000.0); // Convert to microseconds
            
            // Verify successful identification
            ASSERT_TRUE(result.isValid()) << "Failed to identify " << test.name;
        }
        
        auto stats = analyzePerformance(times);
        
        std::cout << test.name << " performance:" << std::endl;
        std::cout << "  Mean: " << stats.mean << " μs" << std::endl;
        std::cout << "  Median: " << stats.median << " μs" << std::endl;
        std::cout << "  95th percentile: " << stats.p95 << " μs" << std::endl;
        std::cout << "  99th percentile: " << stats.p99 << " μs" << std::endl;
        std::cout << "  Std dev: " << stats.std_dev << " μs" << std::endl;
        std::cout << "  Range: [" << stats.min_time << ", " << stats.max_time << "] μs" << std::endl;
        
        // Performance assertions
        EXPECT_LT(stats.mean, test.max_avg_time_us)
            << test.name << " average time too slow: " << stats.mean << " μs";
        
        EXPECT_LT(stats.p95, test.max_avg_time_us * 2.0)
            << test.name << " 95th percentile too slow: " << stats.p95 << " μs";
        
        // Consistency check - standard deviation should be reasonable
        EXPECT_LT(stats.std_dev, stats.mean * 0.5)
            << test.name << " performance too inconsistent, std dev: " << stats.std_dev << " μs";
    }
}

// Test performance scalability with chord complexity
TEST_F(PerformanceIntegrationTest, ComplexityScalability) {
    struct ComplexityTest {
        std::vector<std::vector<int>> chords;
        std::string description;
        int note_count;
    };
    
    std::vector<ComplexityTest> complexity_tests = {
        {{{60, 64, 67}, {60, 63, 67}}, "3-note chords", 3},
        {{{60, 64, 67, 70}, {60, 63, 67, 70}}, "4-note chords", 4},
        {{{60, 64, 67, 70, 74}, {60, 63, 67, 70, 74}}, "5-note chords", 5},
        {{{60, 64, 67, 70, 74, 77}, {60, 63, 67, 70, 74, 77}}, "6-note chords", 6},
        {{{60, 64, 67, 70, 74, 77, 81}, {60, 63, 67, 70, 74, 77, 81}}, "7-note chords", 7},
    };
    
    std::vector<double> avg_times;
    
    for (const auto& test : complexity_tests) {
        double total_time = 0.0;
        int total_tests = 0;
        
        for (const auto& chord : test.chords) {
            double avg_time = measureExecutionTime([&]() {
                auto result = identifier_->identify(chord);
                EXPECT_TRUE(result.isValid());
            }, 500);
            
            total_time += avg_time;
            total_tests++;
        }
        
        double complexity_avg = total_time / total_tests;
        avg_times.push_back(complexity_avg);
        
        std::cout << test.description << ": " << complexity_avg << " μs average" << std::endl;
        
        // Performance should scale reasonably with complexity
        double max_expected_time = 10.0 + (test.note_count - 3) * 5.0; // Base 10μs + 5μs per extra note
        EXPECT_LT(complexity_avg, max_expected_time)
            << test.description << " performance not scaling well: " << complexity_avg << " μs";
    }
    
    // Check that performance doesn't degrade exponentially
    for (size_t i = 1; i < avg_times.size(); ++i) {
        double ratio = avg_times[i] / avg_times[i-1];
        EXPECT_LT(ratio, 2.0) // Should not double with each complexity increase
            << "Performance degrading exponentially at complexity level " << (i + 3);
    }
}

// Test batch processing performance
TEST_F(PerformanceIntegrationTest, BatchProcessingPerformance) {
    // Generate test chord sequences of different sizes
    std::vector<std::vector<int>> base_chords = {
        {60, 64, 67},           // C major
        {65, 69, 72},           // F major
        {67, 71, 74},           // G major
        {57, 61, 64},           // A minor
        {62, 66, 69},           // D minor
        {64, 68, 71},           // E minor
    };
    
    std::vector<int> batch_sizes = {1, 5, 10, 20, 50, 100};
    
    for (int batch_size : batch_sizes) {
        // Create batch of specified size
        std::vector<std::vector<int>> batch;
        for (int i = 0; i < batch_size; ++i) {
            batch.push_back(base_chords[i % base_chords.size()]);
        }
        
        // Measure batch processing time
        double batch_time = measureExecutionTime([&]() {
            auto results = identifier_->identifyBatch(batch);
            EXPECT_EQ(results.size(), batch.size());
            
            // Verify all chords were identified
            for (const auto& result : results) {
                EXPECT_TRUE(result.isValid());
            }
        }, 100);
        
        // Measure individual processing time for comparison
        double individual_time = 0.0;
        for (const auto& chord : batch) {
            individual_time += measureExecutionTime([&]() {
                auto result = identifier_->identify(chord);
                EXPECT_TRUE(result.isValid());
            }, 100);
        }
        
        double efficiency = individual_time / batch_time;
        
        std::cout << "Batch size " << batch_size << ":" << std::endl;
        std::cout << "  Batch time: " << batch_time << " μs" << std::endl;
        std::cout << "  Individual time: " << individual_time << " μs" << std::endl;
        std::cout << "  Efficiency: " << efficiency << "x" << std::endl;
        
        // Batch processing should be at least as fast as individual processing
        EXPECT_GE(efficiency, 0.8) // Allow for some overhead
            << "Batch processing slower than individual for size " << batch_size;
        
        // For larger batches, expect some efficiency gain
        if (batch_size >= 10) {
            EXPECT_GE(efficiency, 1.1)
                << "No efficiency gain for batch size " << batch_size;
        }
    }
}

// Test performance under different load conditions
TEST_F(PerformanceIntegrationTest, LoadPerformance) {
    // Test different load scenarios
    struct LoadTest {
        int iterations;
        std::string description;
        double max_avg_time_us;
    };
    
    std::vector<LoadTest> load_tests = {
        {100, "Light load", 20.0},
        {1000, "Medium load", 25.0},
        {5000, "Heavy load", 30.0},
        {10000, "Extreme load", 35.0},
    };
    
    std::vector<int> test_chord = {60, 64, 67, 70, 74, 77}; // C11 chord
    
    for (const auto& test : load_tests) {
        std::vector<double> times;
        times.reserve(test.iterations);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Perform many identifications
        for (int i = 0; i < test.iterations; ++i) {
            auto chord_start = std::chrono::high_resolution_clock::now();
            auto result = identifier_->identify(test_chord);
            auto chord_end = std::chrono::high_resolution_clock::now();
            
            ASSERT_TRUE(result.isValid());
            
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(chord_end - chord_start);
            times.push_back(static_cast<double>(duration.count()) / 1000.0);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        auto stats = analyzePerformance(times);
        
        std::cout << test.description << " (" << test.iterations << " iterations):" << std::endl;
        std::cout << "  Total time: " << total_duration.count() << " ms" << std::endl;
        std::cout << "  Mean time: " << stats.mean << " μs" << std::endl;
        std::cout << "  95th percentile: " << stats.p95 << " μs" << std::endl;
        std::cout << "  Performance consistency: " << (stats.std_dev / stats.mean * 100) << "% CV" << std::endl;
        
        // Performance should remain consistent under load
        EXPECT_LT(stats.mean, test.max_avg_time_us)
            << test.description << " average performance degraded: " << stats.mean << " μs";
        
        // Performance should be consistent (coefficient of variation < 50%)
        double cv = stats.std_dev / stats.mean;
        EXPECT_LT(cv, 0.5)
            << test.description << " performance too inconsistent, CV: " << (cv * 100) << "%";
        
        // 95th percentile should not be more than 3x the mean
        EXPECT_LT(stats.p95, stats.mean * 3.0)
            << test.description << " has performance outliers, P95: " << stats.p95 << " μs";
    }
}

// Test memory-performance trade-offs
TEST_F(PerformanceIntegrationTest, MemoryPerformanceTradeoffs) {
    auto& tracker = MemoryTracker::getInstance();
    tracker.setTrackingEnabled(true);
    
    // Test different chord types and their memory/performance characteristics
    struct TradeoffTest {
        std::vector<int> chord;
        std::string name;
    };
    
    std::vector<TradeoffTest> tests = {
        {{60, 64, 67}, "Simple triad"},
        {{60, 64, 67, 70}, "Seventh chord"},
        {{60, 64, 67, 70, 74}, "Extended chord"},
        {{60, 64, 67, 70, 74, 77}, "Complex chord (11th)"},
    };
    
    for (const auto& test : tests) {
        tracker.takeSnapshot("Before " + test.name);
        
        // Measure both performance and memory
        double avg_time = measureExecutionTime([&]() {
            auto result = identifier_->identify(test.chord);
            EXPECT_TRUE(result.isValid());
            TRACK_ALLOCATION("TradeoffTest::" + test.name);
        }, 1000);
        
        tracker.takeSnapshot("After " + test.name);
        
        // Get memory usage for this component
        auto component_info = tracker.getComponentInfo("TradeoffTest::" + test.name);
        
        std::cout << test.name << ":" << std::endl;
        std::cout << "  Performance: " << avg_time << " μs" << std::endl;
        std::cout << "  Memory allocations: " << component_info.allocation_count << std::endl;
        
        // Calculate performance per memory ratio
        if (component_info.allocation_count > 0) {
            double perf_per_allocation = avg_time / component_info.allocation_count;
            std::cout << "  Performance per allocation: " << perf_per_allocation << " μs/alloc" << std::endl;
            
            // Performance per allocation should be reasonable
            EXPECT_LT(perf_per_allocation, 0.1) // Less than 0.1μs per allocation
                << test.name << " has poor performance/memory ratio";
        }
    }
    
    // Check overall memory efficiency
    auto snapshots = tracker.getSnapshots();
    if (snapshots.size() >= 2) {
        size_t total_memory_growth = snapshots.back().resident_memory_kb - snapshots[0].resident_memory_kb;
        
        std::cout << "Total memory growth: " << total_memory_growth << " KB" << std::endl;
        
        // Memory growth should be minimal for performance tests
        EXPECT_LT(total_memory_growth, 1000) // Less than 1MB growth
            << "Excessive memory growth during performance tests";
    }
}

// Test cold start vs warm performance
TEST_F(PerformanceIntegrationTest, ColdStartWarmPerformance) {
    // Create fresh components for cold start test
    auto cold_database = std::make_unique<ChordDatabase>();
    auto cold_identifier = std::make_unique<ChordIdentifier>();
    
    std::vector<int> test_chord = {60, 64, 67, 70, 74, 77}; // C11
    
    // Measure cold start performance
    auto cold_start = std::chrono::high_resolution_clock::now();
    auto cold_result = cold_identifier->identify(test_chord);
    auto cold_end = std::chrono::high_resolution_clock::now();
    
    ASSERT_TRUE(cold_result.isValid());
    
    auto cold_duration = std::chrono::duration_cast<std::chrono::microseconds>(cold_end - cold_start);
    double cold_time = static_cast<double>(cold_duration.count());
    
    // Measure warm performance (after several operations)
    for (int i = 0; i < 10; ++i) {
        cold_identifier->identify(test_chord);
    }
    
    double warm_time = measureExecutionTime([&]() {
        auto result = cold_identifier->identify(test_chord);
        EXPECT_TRUE(result.isValid());
    }, 100);
    
    std::cout << "Cold start vs warm performance:" << std::endl;
    std::cout << "  Cold start: " << cold_time << " μs" << std::endl;
    std::cout << "  Warm average: " << warm_time << " μs" << std::endl;
    std::cout << "  Speedup ratio: " << (cold_time / warm_time) << "x" << std::endl;
    
    // Cold start should not be excessively slow
    EXPECT_LT(cold_time, 1000.0) // Less than 1ms cold start
        << "Cold start too slow: " << cold_time << " μs";
    
    // Warm performance should be significantly better
    EXPECT_LT(warm_time, cold_time * 0.5) // At least 2x speedup when warm
        << "Insufficient warm-up benefit";
    
    // Warm performance should meet our target
    EXPECT_LT(warm_time, 30.0) // Less than 30μs when warm
        << "Warm performance not meeting target: " << warm_time << " μs";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== ChordLock Performance Integration Tests ===" << std::endl;
    
    return RUN_ALL_TESTS();
}