#!/usr/bin/env python3
"""
Chordlock Performance Benchmark Suite
Comprehensive testing of CLI performance across all modes and arguments
"""

import subprocess
import time
import json
import statistics
import sys
import os
from typing import List, Dict, Any, Tuple
from dataclasses import dataclass

@dataclass
class BenchmarkResult:
    test_name: str
    command: List[str]
    runs: int
    total_time: float
    avg_time: float
    min_time: float
    max_time: float
    std_dev: float
    success_rate: float
    output_sample: str

class ChordlockBenchmark:
    def __init__(self, binary_path: str = "./build/chordlock", runs: int = 100):
        self.binary_path = binary_path
        self.runs = runs
        self.results: List[BenchmarkResult] = []
        
        # Verify binary exists
        if not os.path.exists(binary_path):
            print(f"Error: Binary not found at {binary_path}")
            print("Please build the CLI first: make build")
            sys.exit(1)
    
    def run_single_test(self, command: List[str], test_name: str) -> BenchmarkResult:
        """Run a single benchmark test with multiple iterations"""
        print(f"Running {test_name}... ", end="", flush=True)
        
        times = []
        successes = 0
        sample_output = ""
        
        for i in range(self.runs):
            if i % 10 == 0:
                print(".", end="", flush=True)
            
            start_time = time.perf_counter()
            try:
                result = subprocess.run(
                    command,
                    capture_output=True,
                    text=True,
                    timeout=5.0
                )
                end_time = time.perf_counter()
                
                if result.returncode == 0:
                    successes += 1
                    times.append(end_time - start_time)
                    if not sample_output:
                        sample_output = result.stdout.strip()
                        
            except subprocess.TimeoutExpired:
                print("T", end="", flush=True)
                continue
            except Exception as e:
                print("E", end="", flush=True)
                continue
        
        if not times:
            print(" FAILED")
            return BenchmarkResult(
                test_name=test_name,
                command=command,
                runs=0,
                total_time=0,
                avg_time=0,
                min_time=0,
                max_time=0,
                std_dev=0,
                success_rate=0,
                output_sample="No successful runs"
            )
        
        total_time = sum(times)
        avg_time = statistics.mean(times)
        min_time = min(times)
        max_time = max(times)
        std_dev = statistics.stdev(times) if len(times) > 1 else 0
        success_rate = successes / self.runs
        
        print(f" OK ({avg_time*1000:.2f}ms avg)")
        
        return BenchmarkResult(
            test_name=test_name,
            command=command,
            runs=len(times),
            total_time=total_time,
            avg_time=avg_time,
            min_time=min_time,
            max_time=max_time,
            std_dev=std_dev,
            success_rate=success_rate,
            output_sample=sample_output
        )
    
    def run_benchmark_suite(self):
        """Run comprehensive benchmark suite"""
        print("=== Chordlock Performance Benchmark ===\n")
        
        # Test cases covering all modes and variations
        test_cases = [
            # Basic chord detection (MIDI notes)
            ([self.binary_path, "-N", "60,64,67"], "Basic Triad (C Major)"),
            ([self.binary_path, "-N", "60,64,67,71"], "7th Chord (Cmaj7)"),
            ([self.binary_path, "-N", "60,63,67,70,74"], "Complex Jazz (Cm7b5)"),
            ([self.binary_path, "-N", "52,60,64,67,74"], "Slash Chord (C/E)"),
            
            # Chord name to notes conversion
            ([self.binary_path, "-c", "Cmaj7"], "Name to Notes (Cmaj7)"),
            ([self.binary_path, "-c", "F#m7b5"], "Name to Notes (F#m7b5)"),
            ([self.binary_path, "-c", "Db13#11"], "Name to Notes (Complex)"),
            ([self.binary_path, "-c", "C/E"], "Name to Notes (Slash)"),
            
            # Key context analysis
            ([self.binary_path, "-N", "60,64,67", "-k", "C"], "Key Context (C Major)"),
            ([self.binary_path, "-N", "60,64,67", "-k", "Am"], "Key Context (A Minor)"),
            ([self.binary_path, "-N", "60,63,67", "-k", "Eb"], "Key Context (Eb Major)"),
            ([self.binary_path, "-N", "66,70,73,76", "-k", "F#m"], "Key Context (F# Minor)"),
            
            # Degree analysis (Roman numerals)
            ([self.binary_path, "-d", "I", "-k", "C"], "Degree Analysis (I)"),
            ([self.binary_path, "-d", "V7", "-k", "G"], "Degree Analysis (V7)"),
            ([self.binary_path, "-d", "ii7b5", "-k", "Bb"], "Degree Analysis (ii7b5)"),
            ([self.binary_path, "-d", "bVII", "-k", "Am"], "Degree Analysis (bVII)"),
            
            # Multiple output formats
            ([self.binary_path, "-N", "60,64,67", "-v"], "Verbose Output"),
            ([self.binary_path, "-N", "60,64,67", "-n", "5"], "Multiple Candidates"),
            ([self.binary_path, "-N", "60,64,67", "-t", "0.8"], "Confidence Threshold"),
            
            # File input (if test files exist)
            # ([self.binary_path, "-f", "test.mid"], "MIDI File Input"),
            
            # Octave variations
            ([self.binary_path, "-c", "Cmaj7", "-o", "3"], "Octave 3"),
            ([self.binary_path, "-c", "Cmaj7", "-o", "5"], "Octave 5"),
            ([self.binary_path, "-c", "Cmaj7", "-o", "6"], "Octave 6"),
            
            # Edge cases
            ([self.binary_path, "-N", "60"], "Single Note"),
            ([self.binary_path, "-N", "60,61"], "Semitone Cluster"),
            ([self.binary_path, "-N", "60,64,67,71,74,77,81"], "Large Chord"),
            
            # Stress tests
            ([self.binary_path, "-N", "48,52,55,60,64,67,72,76"], "Wide Voicing"),
            ([self.binary_path, "-N", "60,61,62,63,64,65,66"], "Chromatic Cluster"),
        ]
        
        # Add key variations for comprehensive testing
        keys = ["C", "G", "D", "A", "E", "B", "F#", "Db", "Ab", "Eb", "Bb", "F", 
                "Am", "Em", "Bm", "F#m", "C#m", "G#m", "Ebm", "Bbm", "Fm", "Cm", "Gm", "Dm"]
        
        for key in keys[:8]:  # Test subset of keys to avoid too many tests
            test_cases.append(([self.binary_path, "-N", "60,64,67", "-k", key], f"Key Test ({key})"))
        
        # Run all tests
        for command, test_name in test_cases:
            result = self.run_single_test(command, test_name)
            self.results.append(result)
        
        print(f"\n=== Benchmark Complete ===")
        print(f"Total tests: {len(self.results)}")
        print(f"Runs per test: {self.runs}")
    
    def print_results(self):
        """Print detailed benchmark results"""
        print("\n" + "="*80)
        print("DETAILED RESULTS")
        print("="*80)
        
        successful_results = [r for r in self.results if r.success_rate > 0]
        
        if not successful_results:
            print("No successful tests!")
            return
        
        # Sort by average time
        successful_results.sort(key=lambda x: x.avg_time)
        
        print(f"{'Test Name':<35} {'Avg (ms)':<10} {'Min (ms)':<10} {'Max (ms)':<10} {'StdDev':<8} {'Success%':<8}")
        print("-" * 80)
        
        for result in successful_results:
            print(f"{result.test_name:<35} "
                  f"{result.avg_time*1000:<10.2f} "
                  f"{result.min_time*1000:<10.2f} "
                  f"{result.max_time*1000:<10.2f} "
                  f"{result.std_dev*1000:<8.2f} "
                  f"{result.success_rate*100:<8.1f}")
    
    def print_summary(self):
        """Print benchmark summary statistics"""
        successful_results = [r for r in self.results if r.success_rate > 0]
        
        if not successful_results:
            print("\nNo successful tests to summarize!")
            return
        
        times = [r.avg_time for r in successful_results]
        
        print("\n" + "="*50)
        print("PERFORMANCE SUMMARY")
        print("="*50)
        print(f"Tests completed:     {len(successful_results)}/{len(self.results)}")
        print(f"Total runtime:       {sum(r.total_time for r in successful_results):.2f}s")
        print(f"Average time:        {statistics.mean(times)*1000:.2f}ms")
        print(f"Median time:         {statistics.median(times)*1000:.2f}ms")
        print(f"Fastest test:        {min(times)*1000:.2f}ms ({min(successful_results, key=lambda x: x.avg_time).test_name})")
        print(f"Slowest test:        {max(times)*1000:.2f}ms ({max(successful_results, key=lambda x: x.avg_time).test_name})")
        print(f"Standard deviation:  {statistics.stdev(times)*1000:.2f}ms")
        
        # Performance categories
        fast_tests = [r for r in successful_results if r.avg_time < 0.001]  # < 1ms
        medium_tests = [r for r in successful_results if 0.001 <= r.avg_time < 0.010]  # 1-10ms
        slow_tests = [r for r in successful_results if r.avg_time >= 0.010]  # > 10ms
        
        print(f"\nPerformance breakdown:")
        print(f"  Very fast (< 1ms):   {len(fast_tests)} tests")
        print(f"  Medium (1-10ms):     {len(medium_tests)} tests")
        print(f"  Slow (> 10ms):       {len(slow_tests)} tests")
        
        if slow_tests:
            print(f"\nSlowest tests:")
            for test in sorted(slow_tests, key=lambda x: x.avg_time, reverse=True)[:5]:
                print(f"  {test.test_name}: {test.avg_time*1000:.2f}ms")
    
    def save_results(self, filename: str = "benchmark_results.json"):
        """Save results to JSON file"""
        data = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "binary_path": self.binary_path,
            "runs_per_test": self.runs,
            "results": []
        }
        
        for result in self.results:
            data["results"].append({
                "test_name": result.test_name,
                "command": " ".join(result.command),
                "runs": result.runs,
                "total_time": result.total_time,
                "avg_time_ms": result.avg_time * 1000,
                "min_time_ms": result.min_time * 1000,
                "max_time_ms": result.max_time * 1000,
                "std_dev_ms": result.std_dev * 1000,
                "success_rate": result.success_rate,
                "output_sample": result.output_sample
            })
        
        filepath = os.path.join("benchmark", filename)
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"\nResults saved to: {filepath}")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Chordlock Performance Benchmark")
    parser.add_argument("--binary", "-b", default="./build/chordlock", 
                       help="Path to chordlock binary")
    parser.add_argument("--runs", "-r", type=int, default=100,
                       help="Number of runs per test")
    parser.add_argument("--output", "-o", default="benchmark_results.json",
                       help="Output JSON file")
    
    args = parser.parse_args()
    
    # Change to project root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    os.chdir(project_root)
    
    benchmark = ChordlockBenchmark(args.binary, args.runs)
    benchmark.run_benchmark_suite()
    benchmark.print_results()
    benchmark.print_summary()
    benchmark.save_results(args.output)

if __name__ == "__main__":
    main()