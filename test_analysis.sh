#!/bin/bash

# Test analysis script to run all 101 test cases and categorize failures

echo "🔍 Comprehensive Test Analysis"
echo "=============================="

# Counters
total_tests=0
passed=0
failed=0

# Categories
power_chord_fails=0
basic_triad_fails=0
seventh_chord_fails=0
extended_chord_fails=0
slash_chord_fails=0
sus_chord_fails=0
aug_dim_fails=0

declare -a failures

echo "Running all test cases..."

while read -r line; do
    if [[ -z "$line" || "$line" =~ ^[[:space:]]*$ ]]; then
        continue
    fi
    
    # Parse format: "number→notes # expected"
    test_num=$(echo "$line" | sed 's/→.*//')
    test_data=$(echo "$line" | sed 's/.*→//')
    
    # Extract notes and expected chord
    notes=$(echo "$test_data" | cut -d'#' -f1 | xargs)
    expected=$(echo "$test_data" | cut -d'#' -f2 | xargs)
    
    if [[ -z "$notes" || -z "$expected" ]]; then
        continue
    fi
    
    ((total_tests++))
    
    # Run test
    result=$(./build/chordlock_test -N "$notes" 2>/dev/null | grep "Detected Chord:" | sed 's/Detected Chord: //' | sed 's/ (confidence:.*//')
    
    if [[ "$result" == "$expected" ]]; then
        ((passed++))
        echo "✅ Test $test_num: $expected"
    else
        ((failed++))
        failures+=("Test $test_num: Expected '$expected', Got '$result' (Notes: $notes)")
        echo "❌ Test $test_num: Expected '$expected', Got '$result'"
        
        # Categorize failure
        case "$expected" in
            *5) ((power_chord_fails++)) ;;
            *maj7|*7|*6) ((seventh_chord_fails++)) ;;
            *add9|*add11|*9|*11|*13) ((extended_chord_fails++)) ;;
            */*) ((slash_chord_fails++)) ;;
            *sus2|*sus4) ((sus_chord_fails++)) ;;
            *aug|*dim*) ((aug_dim_fails++)) ;;
            *) 
                if [[ "$expected" =~ ^[A-G][#b]?m?$ ]]; then
                    ((basic_triad_fails++))
                else
                    ((basic_triad_fails++))
                fi
                ;;
        esac
    fi
    
done < test_set.txt

echo ""
echo "📊 Test Results Summary"
echo "======================"
echo "Total tests: $total_tests"
if [[ $total_tests -gt 0 ]]; then
    echo "Passed: $passed ($((passed * 100 / total_tests))%)"
    echo "Failed: $failed ($((failed * 100 / total_tests))%)"
else
    echo "Passed: $passed (0%)"
    echo "Failed: $failed (0%)"
fi

echo ""
echo "📈 Failure Categories"
echo "===================="
echo "Power chords (C5, D5, etc.): $power_chord_fails"
echo "Basic triads (C, Dm, etc.): $basic_triad_fails"
echo "Seventh chords (Cmaj7, C7, etc.): $seventh_chord_fails"
echo "Extended chords (C9, C11, C13): $extended_chord_fails"
echo "Slash chords (C/E, etc.): $slash_chord_fails"
echo "Suspended chords (Csus2, Csus4): $sus_chord_fails"
echo "Augmented/Diminished: $aug_dim_fails"

echo ""
echo "❌ Detailed Failures"
echo "=================="
for failure in "${failures[@]}"; do
    echo "$failure"
done