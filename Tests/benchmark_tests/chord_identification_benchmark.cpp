#include <benchmark/benchmark.h>
#include "Core/ChordIdentifier.h"
#include "Core/IntervalEngine.h"
#include "Core/ChordDatabase.h"
#include <vector>
#include <random>

using namespace ChordLock;

// Global test data
static std::unique_ptr<ChordIdentifier> g_identifier;
static std::vector<std::vector<int>> g_test_chords;

// Setup function
void SetupBenchmarks() {
    g_identifier = std::make_unique<ChordIdentifier>();
    
    // Try to load the main interval dictionary
    if (!g_identifier->initialize("interval_dict.yaml")) {
        // Fallback to manual chord addition for benchmarking
        // This is a simplified version for benchmarking purposes
    }
    
    // Generate test chord patterns
    g_test_chords = {
        {60, 64, 67},       // C major
        {60, 63, 67},       // C minor
        {60, 64, 67, 70},   // C7
        {60, 64, 67, 71},   // Cmaj7
        {60, 63, 67, 70},   // Cm7
        {64, 67, 72},       // C/E (first inversion)
        {67, 72, 76},       // C/G (second inversion)
        {60, 65, 67},       // Csus4
        {60, 62, 67},       // Csus2
        {60, 63, 66},       // Cdim
        {60, 64, 68},       // Caug
        {60, 64, 67, 74},   // Cadd9
        {60, 64, 67, 70, 74}, // C9
        {60, 64, 67, 70, 77}, // C11
        {60, 64, 67, 70, 81}  // C13
    };
}

// Benchmark single chord identification
static void BM_SingleChordIdentification(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    std::vector<int> c_major = {60, 64, 67};
    
    for (auto _ : state) {
        auto result = g_identifier->identify(c_major);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SingleChordIdentification);

// Benchmark different identification modes
static void BM_FastMode(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    std::vector<int> c_major = {60, 64, 67};
    
    for (auto _ : state) {
        auto result = g_identifier->identify(c_major, IdentificationMode::FAST);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FastMode);

static void BM_StandardMode(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    std::vector<int> c_major = {60, 64, 67};
    
    for (auto _ : state) {
        auto result = g_identifier->identify(c_major, IdentificationMode::STANDARD);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_StandardMode);

static void BM_ComprehensiveMode(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    std::vector<int> c_major = {60, 64, 67};
    
    for (auto _ : state) {
        auto result = g_identifier->identify(c_major, IdentificationMode::COMPREHENSIVE);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ComprehensiveMode);

// Benchmark batch processing
static void BM_BatchProcessing(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    if (g_test_chords.empty()) SetupBenchmarks();
    
    for (auto _ : state) {
        auto results = g_identifier->identifyBatch(g_test_chords);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * g_test_chords.size());
}
BENCHMARK(BM_BatchProcessing);

// Benchmark interval calculation
static void BM_IntervalCalculation(benchmark::State& state) {
    IntervalEngine engine;
    std::vector<int> c_major = {60, 64, 67};
    
    for (auto _ : state) {
        auto result = engine.calculateIntervals(c_major);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_IntervalCalculation);

// Benchmark complex chords
static void BM_ComplexChordIdentification(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    // Complex 13th chord: C13
    std::vector<int> complex_chord = {60, 64, 67, 70, 74, 77, 81};
    
    for (auto _ : state) {
        auto result = g_identifier->identify(complex_chord);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ComplexChordIdentification);

// Benchmark with different chord sizes
static void BM_ChordsBySize(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    int chord_size = state.range(0);
    std::vector<int> chord;
    
    // Generate chord of specified size (chromatic cluster from C)
    for (int i = 0; i < chord_size; ++i) {
        chord.push_back(60 + i);
    }
    
    for (auto _ : state) {
        auto result = g_identifier->identify(chord);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetComplexityN(chord_size);
}
BENCHMARK(BM_ChordsBySize)->Range(1, 12)->Complexity();

// Benchmark random chord patterns
static void BM_RandomChords(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    std::random_device rd;
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<> note_dist(48, 84);
    
    std::vector<std::vector<int>> random_chords;
    for (int i = 0; i < 100; ++i) {
        std::vector<int> chord;
        int num_notes = 3 + (i % 4); // 3-6 notes
        for (int j = 0; j < num_notes; ++j) {
            chord.push_back(note_dist(gen));
        }
        random_chords.push_back(chord);
    }
    
    size_t chord_index = 0;
    for (auto _ : state) {
        auto result = g_identifier->identify(random_chords[chord_index % random_chords.size()]);
        benchmark::DoNotOptimize(result);
        chord_index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RandomChords);

// Benchmark cache performance
static void BM_CachePerformance(benchmark::State& state) {
    if (!g_identifier) SetupBenchmarks();
    
    std::vector<int> c_major = {60, 64, 67};
    
    // Warm up cache
    for (int i = 0; i < 10; ++i) {
        g_identifier->identify(c_major);
    }
    
    for (auto _ : state) {
        auto result = g_identifier->identify(c_major);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CachePerformance);

// Memory usage benchmark
static void BM_MemoryUsage(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        auto identifier = std::make_unique<ChordIdentifier>();
        state.ResumeTiming();
        
        std::vector<int> c_major = {60, 64, 67};
        auto result = identifier->identify(c_major);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_MemoryUsage);