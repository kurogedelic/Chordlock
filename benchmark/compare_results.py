#!/usr/bin/env python3
"""
Benchmark Results Comparison Tool
Compare performance between different benchmark runs
"""

import json
import sys
import argparse
from typing import Dict, List, Any

def load_results(filename: str) -> Dict[str, Any]:
    """Load benchmark results from JSON file"""
    try:
        with open(filename, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: File {filename} not found")
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON in {filename}")
        sys.exit(1)

def compare_results(before_file: str, after_file: str):
    """Compare two benchmark result files"""
    before = load_results(before_file)
    after = load_results(after_file)
    
    print("=== Benchmark Comparison ===\n")
    print(f"Before: {before_file} ({before['timestamp']})")
    print(f"After:  {after_file} ({after['timestamp']})")
    print(f"Runs:   {before['runs_per_test']} vs {after['runs_per_test']}")
    print()
    
    # Create lookup dictionaries
    before_tests = {r['test_name']: r for r in before['results']}
    after_tests = {r['test_name']: r for r in after['results']}
    
    # Find common tests
    common_tests = set(before_tests.keys()) & set(after_tests.keys())
    
    if not common_tests:
        print("No common tests found between the two result sets!")
        return
    
    print(f"Comparing {len(common_tests)} common tests:\n")
    
    # Calculate changes
    improvements = []
    regressions = []
    
    print(f"{'Test Name':<35} {'Before':<10} {'After':<10} {'Change':<12} {'%':<8}")
    print("-" * 75)
    
    for test_name in sorted(common_tests):
        before_time = before_tests[test_name]['avg_time_ms']
        after_time = after_tests[test_name]['avg_time_ms']
        
        change = after_time - before_time
        percent_change = (change / before_time) * 100 if before_time > 0 else 0
        
        status = ""
        if abs(percent_change) < 5:  # Less than 5% change
            status = "~"
        elif percent_change < 0:  # Improvement
            status = "↑"
            improvements.append((test_name, percent_change))
        else:  # Regression
            status = "↓"
            regressions.append((test_name, percent_change))
        
        print(f"{test_name:<35} "
              f"{before_time:<10.2f} "
              f"{after_time:<10.2f} "
              f"{change:+8.2f}ms {status:<3} "
              f"{percent_change:+6.1f}%")
    
    # Summary statistics
    all_before = [before_tests[t]['avg_time_ms'] for t in common_tests]
    all_after = [after_tests[t]['avg_time_ms'] for t in common_tests]
    
    avg_before = sum(all_before) / len(all_before)
    avg_after = sum(all_after) / len(all_after)
    avg_change = avg_after - avg_before
    avg_percent = (avg_change / avg_before) * 100
    
    print("\n" + "="*50)
    print("SUMMARY")
    print("="*50)
    print(f"Average performance:")
    print(f"  Before: {avg_before:.2f}ms")
    print(f"  After:  {avg_after:.2f}ms")
    print(f"  Change: {avg_change:+.2f}ms ({avg_percent:+.1f}%)")
    
    if improvements:
        print(f"\n🚀 Improvements ({len(improvements)} tests):")
        for test_name, percent in sorted(improvements, key=lambda x: x[1]):
            print(f"  {test_name}: {percent:.1f}% faster")
    
    if regressions:
        print(f"\n⚠️  Regressions ({len(regressions)} tests):")
        for test_name, percent in sorted(regressions, key=lambda x: x[1], reverse=True):
            print(f"  {test_name}: {percent:.1f}% slower")
    
    if not improvements and not regressions:
        print("\n✅ No significant performance changes detected")
    
    # Overall verdict
    print(f"\n{'='*50}")
    if avg_percent < -5:
        print("🎉 OVERALL: Significant performance improvement!")
    elif avg_percent > 5:
        print("⚠️  OVERALL: Performance regression detected")
    elif avg_percent < -2:
        print("✅ OVERALL: Minor performance improvement")
    elif avg_percent > 2:
        print("⚠️  OVERALL: Minor performance regression")
    else:
        print("➡️  OVERALL: Performance unchanged")

def main():
    parser = argparse.ArgumentParser(description="Compare benchmark results")
    parser.add_argument("before", help="Before benchmark results (JSON)")
    parser.add_argument("after", help="After benchmark results (JSON)")
    
    args = parser.parse_args()
    
    compare_results(args.before, args.after)

if __name__ == "__main__":
    main()