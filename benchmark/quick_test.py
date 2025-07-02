#!/usr/bin/env python3
"""
Quick Chordlock Performance Test
Fast smoke test to verify basic performance
"""

import subprocess
import time
import os
import sys

def run_quick_test():
    """Run a quick performance test"""
    binary_path = "./build/chordlock"
    
    if not os.path.exists(binary_path):
        print(f"Error: Binary not found at {binary_path}")
        print("Please build the CLI first: make build")
        return False
    
    test_cases = [
        ([binary_path, "-N", "60,64,67"], "C Major Triad"),
        ([binary_path, "-c", "Cmaj7"], "Cmaj7 to Notes"),
        ([binary_path, "-N", "60,64,67", "-k", "C"], "C Major with Key"),
        ([binary_path, "-d", "V7", "-k", "C"], "V7 Degree Analysis"),
        ([binary_path, "-N", "60,64,67,71", "-v"], "Verbose Mode"),
    ]
    
    print("=== Quick Performance Test ===\n")
    
    for command, test_name in test_cases:
        print(f"Testing {test_name}... ", end="", flush=True)
        
        # Warm-up run
        subprocess.run(command, capture_output=True, text=True, timeout=5.0)
        
        # Timed runs
        times = []
        for _ in range(10):
            start = time.perf_counter()
            result = subprocess.run(command, capture_output=True, text=True, timeout=5.0)
            end = time.perf_counter()
            
            if result.returncode == 0:
                times.append(end - start)
        
        if times:
            avg_time = sum(times) / len(times)
            print(f"{avg_time*1000:.2f}ms avg")
            if times[0]:  # Show sample output for first test
                print(f"  Sample: {result.stdout.strip()}")
        else:
            print("FAILED")
    
    print(f"\nQuick test complete! Run full benchmark with: python3 benchmark/benchmark.py")

if __name__ == "__main__":
    # Change to project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    os.chdir(project_root)
    
    run_quick_test()