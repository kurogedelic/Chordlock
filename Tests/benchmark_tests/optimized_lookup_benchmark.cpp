#include <benchmark/benchmark.h>
#include "Core/ChordDatabase.h"
#include "Core/ChordIdentifier.h"
#include <random>
#include <vector>

using namespace ChordLock;

/**
 * Benchmark suite for optimized lookup strategies
 * Measures performance improvements from Robin Hood, Perfect Hash, and Cache-Oblivious structures
 */

// Test data generation
static std::vector<std::vector<int>> generateTestChords(size_t count) {
    std::vector<std::vector<int>> chords;
    chords.reserve(count);
    
    // Common chord patterns
    std::vector<std::vector<int>> patterns = {
        {0, 4, 7},           // Major triad
        {0, 3, 7},           // Minor triad
        {0, 4, 7, 10},       // Dominant 7th
        {0, 4, 7, 11},       // Major 7th
        {0, 3, 7, 10},       // Minor 7th
        {0, 4, 7, 10, 2},    // 9th chord
        {0, 4, 7, 10, 2, 5}, // 11th chord
        {0, 3, 6, 9},        // Diminished 7th
        {0, 4, 8},           // Augmented triad
        {0, 2, 7},           // Sus2
        {0, 5, 7},           // Sus4
        {0, 4, 7, 9},        // 6th chord
    };
    
    // Generate test set by cycling through patterns
    for (size_t i = 0; i < count; ++i) {
        chords.push_back(patterns[i % patterns.size()]);
    }
    
    return chords;
}

// Global test data
static auto test_chords = generateTestChords(1000);
static ChordDatabase* database = nullptr;
static ChordIdentifier* identifier = nullptr;

// Setup function
static void SetupBenchmark() {
    if (!database) {
        database = new ChordDatabase();
        database->buildOptimizedStructures();
        
        identifier = new ChordIdentifier();
    }
}

// Benchmark: Standard unordered_map lookup
static void BM_StandardHashMapLookup(benchmark::State& state) {
    SetupBenchmark();
    
    size_t idx = 0;
    for (auto _ : state) {
        auto result = database->findExactMatch(test_chords[idx % test_chords.size()]);
        benchmark::DoNotOptimize(result);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StandardHashMapLookup);

// Benchmark: Robin Hood hash lookup
static void BM_RobinHoodHashLookup(benchmark::State& state) {
    SetupBenchmark();
    
#ifdef USE_ROBIN_HOOD
    size_t idx = 0;
    for (auto _ : state) {
        auto result = database->findExactMatchOptimized(test_chords[idx % test_chords.size()]);
        benchmark::DoNotOptimize(result);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Robin Hood Enabled");
#else
    state.SkipWithError("Robin Hood hashing not enabled");
#endif
}
BENCHMARK(BM_RobinHoodHashLookup);

// Benchmark: Perfect Hash lookup
static void BM_PerfectHashLookup(benchmark::State& state) {
    SetupBenchmark();
    
#ifdef USE_PERFECT_HASH
    size_t idx = 0;
    for (auto _ : state) {
        auto result = database->findExactMatchOptimized(test_chords[idx % test_chords.size()]);
        benchmark::DoNotOptimize(result);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Perfect Hash Enabled");
#else
    state.SkipWithError("Perfect Hash not enabled");
#endif
}
BENCHMARK(BM_PerfectHashLookup);

// Benchmark: Cache-Oblivious B-tree lookup
static void BM_CacheObliviousLookup(benchmark::State& state) {
    SetupBenchmark();
    
#ifdef USE_CACHE_OBLIVIOUS
    size_t idx = 0;
    for (auto _ : state) {
        auto result = database->findExactMatchOptimized(test_chords[idx % test_chords.size()]);
        benchmark::DoNotOptimize(result);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Cache-Oblivious Enabled");
#else
    state.SkipWithError("Cache-Oblivious B-tree not enabled");
#endif
}
BENCHMARK(BM_CacheObliviousLookup);

// Benchmark: Batch lookup performance
static void BM_BatchLookup(benchmark::State& state) {
    SetupBenchmark();
    
    size_t batch_size = state.range(0);
    std::vector<std::vector<int>> batch;
    batch.reserve(batch_size);
    
    for (auto _ : state) {
        state.PauseTiming();
        batch.clear();
        for (size_t i = 0; i < batch_size; ++i) {
            batch.push_back(test_chords[i % test_chords.size()]);
        }
        state.ResumeTiming();
        
        auto results = database->findBatchParallel(batch);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_BatchLookup)->Range(8, 256);

// Benchmark: Memory usage comparison
static void BM_MemoryUsage(benchmark::State& state) {
    SetupBenchmark();
    
    for (auto _ : state) {
        size_t standard_memory = database->getMemoryUsage();
        size_t optimized_memory = database->getOptimizedMemoryUsage();
        
        state.counters["standard_kb"] = standard_memory / 1024.0;
        state.counters["optimized_kb"] = optimized_memory / 1024.0;
        state.counters["reduction_percent"] = 
            100.0 * (1.0 - static_cast<double>(optimized_memory) / standard_memory);
    }
}
BENCHMARK(BM_MemoryUsage);

// Benchmark: Cache miss rate
static void BM_CacheMissRate(benchmark::State& state) {
    SetupBenchmark();
    
#ifdef USE_CACHE_OBLIVIOUS
    // Simulate random access pattern to test cache behavior
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, test_chords.size() - 1);
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            int idx = dis(gen);
            auto result = database->findExactMatchOptimized(test_chords[idx]);
            benchmark::DoNotOptimize(result);
        }
    }
    
    // Report cache statistics
    // state.counters["cache_miss_rate"] = database->getCacheMissRate();
    state.SetItemsProcessed(state.iterations() * 100);
#else
    state.SkipWithError("Cache-Oblivious not enabled");
#endif
}
BENCHMARK(BM_CacheMissRate);

// Benchmark: Worst-case lookup (non-existent chord)
static void BM_WorstCaseLookup(benchmark::State& state) {
    SetupBenchmark();
    
    // Create non-existent chord patterns
    std::vector<int> non_existent = {1, 2, 3, 5, 8, 11, 13};  // Unlikely pattern
    
    for (auto _ : state) {
        auto result = database->findExactMatchOptimized(non_existent);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WorstCaseLookup);

// Custom main to initialize and cleanup
int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    
    std::cout << "=== ChordLock Optimized Lookup Benchmark ===" << std::endl;
    std::cout << "Testing with " << test_chords.size() << " chord patterns" << std::endl;
    
#ifdef USE_ROBIN_HOOD
    std::cout << "✓ Robin Hood hashing enabled" << std::endl;
#endif
#ifdef USE_PERFECT_HASH
    std::cout << "✓ Perfect Minimal Hash Function enabled" << std::endl;
#endif
#ifdef USE_CACHE_OBLIVIOUS
    std::cout << "✓ Cache-Oblivious B-tree enabled" << std::endl;
#endif
    
    std::cout << "============================================" << std::endl;
    
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Cleanup
    delete database;
    delete identifier;
    
    return 0;
}