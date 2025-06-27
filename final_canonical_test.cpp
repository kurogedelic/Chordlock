#include "src/Chordlock.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Chordlock chordlock;
    
    std::cout << "🎯 FINAL CANONICAL NAMING VERIFICATION\n";
    std::cout << "======================================\n\n";
    
    // Test cases exactly as the user requested
    struct TestCase {
        std::string input;
        std::string expectedCanonical;
        std::string description;
    };
    
    std::vector<TestCase> tests = {
        {"C+", "Caug", "C+ should return Caug (user requirement)"},
        {"Caug", "Caug", "Caug should return Caug (consistent)"},
        {"C°", "Cdim", "C° should return Cdim (user requirement)"},
        {"Cdim", "Cdim", "Cdim should return Cdim (consistent)"},
        {"CM", "C", "CM should return C (major simplification)"},
        {"Cmaj", "C", "Cmaj should return C (major simplification)"},
        {"CM7", "Cmaj7", "CM7 should return Cmaj7 (canonical major 7th)"},
        {"Cmaj7", "Cmaj7", "Cmaj7 should return Cmaj7 (consistent)"},
        {"Csus", "Csus4", "Generic sus should default to sus4"},
        {"C5", "C5", "Power chord should remain C5"}
    };
    
    int passed = 0;
    int total = tests.size();
    
    for (const auto& test : tests) {
        std::string jsonResult = chordlock.chordNameToNotesJSON(test.input, 4);
        
        // Parse JSON to extract chord name
        size_t chordPos = jsonResult.find("\"chord\":\"");
        std::string actualCanonical = "NOT_FOUND";
        
        if (chordPos != std::string::npos) {
            size_t start = chordPos + 9; // Skip "chord":"
            size_t end = jsonResult.find("\"", start);
            if (end != std::string::npos) {
                actualCanonical = jsonResult.substr(start, end - start);
            }
        }
        
        bool testPassed = (actualCanonical == test.expectedCanonical);
        std::cout << (testPassed ? "✅" : "❌") << " Input: " << test.input 
                  << " → Expected: " << test.expectedCanonical 
                  << " → Got: " << actualCanonical << "\n";
        std::cout << "   " << test.description << "\n";
        
        if (!testPassed) {
            std::cout << "   Full JSON: " << jsonResult << "\n";
        }
        std::cout << "\n";
        
        if (testPassed) passed++;
    }
    
    std::cout << "=== FINAL RESULT ===\n";
    std::cout << "Passed: " << passed << "/" << total << " tests\n";
    
    if (passed == total) {
        std::cout << "🎉 ALL TESTS PASSED! Canonical naming system is working correctly.\n";
        std::cout << "✅ C+ returns 'Caug' as requested\n";
        std::cout << "✅ C° returns 'Cdim' as requested\n";
        std::cout << "✅ System uses consistent canonical names\n";
        return 0;
    } else {
        std::cout << "❌ Some tests failed. Canonical naming needs fixes.\n";
        return 1;
    }
}