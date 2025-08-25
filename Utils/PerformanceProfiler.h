#pragma once

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <string>
#include <iomanip>
#include <sstream>

namespace ChordLock {

struct PerformanceMetrics {
    double mean_ns;
    double median_ns;
    double stddev_ns;
    double p50_ns;  // 50th percentile (median)
    double p95_ns;  // 95th percentile
    double p99_ns;  // 99th percentile
    double min_ns;
    double max_ns;
    size_t sample_count;
    size_t outliers_removed;
    
    std::string toString() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "Performance Metrics:\n";
        ss << "  Mean: " << mean_ns << " ns\n";
        ss << "  Median (P50): " << median_ns << " ns\n";
        ss << "  StdDev: " << stddev_ns << " ns\n";
        ss << "  P95: " << p95_ns << " ns\n";
        ss << "  P99: " << p99_ns << " ns\n";
        ss << "  Min: " << min_ns << " ns\n";
        ss << "  Max: " << max_ns << " ns\n";
        ss << "  Samples: " << sample_count;
        if (outliers_removed > 0) {
            ss << " (removed " << outliers_removed << " outliers)";
        }
        return ss.str();
    }
    
    double getMicroseconds() const { return mean_ns / 1000.0; }
    bool meetsTarget(double target_ns) const { return p95_ns < target_ns; }
};

class PerformanceProfiler {
private:
    static constexpr size_t DEFAULT_WARMUP_RUNS = 100;
    static constexpr size_t DEFAULT_SAMPLE_SIZE = 1000;
    static constexpr double OUTLIER_THRESHOLD = 3.0; // Z-score for outlier detection
    
    std::vector<double> samples_;
    size_t warmup_runs_;
    size_t target_samples_;
    bool remove_outliers_;
    
    double calculatePercentile(const std::vector<double>& sorted_samples, double percentile) const {
        if (sorted_samples.empty()) return 0.0;
        
        size_t index = static_cast<size_t>(percentile * sorted_samples.size() / 100.0);
        index = std::min(index, sorted_samples.size() - 1);
        return sorted_samples[index];
    }
    
    std::vector<double> removeOutliers(const std::vector<double>& samples) const {
        if (samples.size() < 10) return samples; // Too few samples to remove outliers
        
        // Calculate mean and stddev
        double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
        double sq_sum = std::inner_product(samples.begin(), samples.end(), samples.begin(), 0.0);
        double stddev = std::sqrt(sq_sum / samples.size() - mean * mean);
        
        // Remove outliers based on z-score
        std::vector<double> filtered;
        for (double sample : samples) {
            double z_score = std::abs((sample - mean) / stddev);
            if (z_score < OUTLIER_THRESHOLD) {
                filtered.push_back(sample);
            }
        }
        
        return filtered;
    }
    
public:
    PerformanceProfiler(size_t warmup_runs = DEFAULT_WARMUP_RUNS,
                       size_t target_samples = DEFAULT_SAMPLE_SIZE,
                       bool remove_outliers = true)
        : warmup_runs_(warmup_runs)
        , target_samples_(target_samples)
        , remove_outliers_(remove_outliers) {
        samples_.reserve(target_samples);
    }
    
    template<typename Func>
    PerformanceMetrics profile(Func&& func, const std::string& description = "") {
        (void)description; // Suppress unused parameter warning
        samples_.clear();
        
        // Warmup phase - not measured
        for (size_t i = 0; i < warmup_runs_; ++i) {
            func();
        }
        
        // Measurement phase
        for (size_t i = 0; i < target_samples_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            samples_.push_back(static_cast<double>(duration.count()));
        }
        
        return analyze();
    }
    
    template<typename Func, typename Input>
    PerformanceMetrics profileWithInput(Func&& func, const std::vector<Input>& inputs) {
        samples_.clear();
        
        // Warmup with first input
        if (!inputs.empty()) {
            for (size_t i = 0; i < warmup_runs_; ++i) {
                func(inputs[0]);
            }
        }
        
        // Measure each input
        for (const auto& input : inputs) {
            auto start = std::chrono::high_resolution_clock::now();
            func(input);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            samples_.push_back(static_cast<double>(duration.count()));
        }
        
        return analyze();
    }
    
    PerformanceMetrics analyze() const {
        PerformanceMetrics metrics{};
        
        if (samples_.empty()) {
            return metrics;
        }
        
        // Copy samples for processing
        std::vector<double> processed_samples = samples_;
        size_t original_count = processed_samples.size();
        
        // Remove outliers if requested
        if (remove_outliers_ && processed_samples.size() > 10) {
            processed_samples = removeOutliers(processed_samples);
            metrics.outliers_removed = original_count - processed_samples.size();
        }
        
        // Sort for percentile calculations
        std::sort(processed_samples.begin(), processed_samples.end());
        
        // Calculate statistics
        metrics.sample_count = processed_samples.size();
        metrics.min_ns = processed_samples.front();
        metrics.max_ns = processed_samples.back();
        
        // Mean
        metrics.mean_ns = std::accumulate(processed_samples.begin(), processed_samples.end(), 0.0) 
                         / processed_samples.size();
        
        // Median
        metrics.median_ns = calculatePercentile(processed_samples, 50);
        metrics.p50_ns = metrics.median_ns;
        
        // Percentiles
        metrics.p95_ns = calculatePercentile(processed_samples, 95);
        metrics.p99_ns = calculatePercentile(processed_samples, 99);
        
        // Standard deviation
        double sq_sum = std::inner_product(processed_samples.begin(), processed_samples.end(), 
                                          processed_samples.begin(), 0.0);
        metrics.stddev_ns = std::sqrt(sq_sum / processed_samples.size() - 
                                      metrics.mean_ns * metrics.mean_ns);
        
        return metrics;
    }
    
    // Convenience method for comparing two implementations
    template<typename Func1, typename Func2>
    static std::pair<PerformanceMetrics, PerformanceMetrics> 
    compare(Func1&& func1, Func2&& func2, const std::string& name1 = "Implementation 1", 
            const std::string& name2 = "Implementation 2") {
        PerformanceProfiler profiler;
        
        auto metrics1 = profiler.profile(func1, name1);
        auto metrics2 = profiler.profile(func2, name2);
        
        return {metrics1, metrics2};
    }
};

} // namespace ChordLock