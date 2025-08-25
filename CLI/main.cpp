#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <getopt.h>
#include <chrono>
#include "Core/ChordIdentifier.h"
#include "Core/ProgressionAnalyzer.h"
#include "Utils/NoteConverter.h"
#include "Utils/OutputFormatter.h"

using namespace ChordLock;

struct CLIOptions {
    std::vector<int> notes;
    std::string key = "C";
    std::string output_format = "standard";
    std::string chord_dict_path = "interval_dict.yaml";
    std::string aliases_path = "";
    bool verbose = false;
    bool analyze = false;
    bool batch_mode = false;
    bool benchmark = false;
    bool voicing_analysis = false;
    bool progression_analysis = false;
    bool chord_suggestions = false;
    bool real_time_midi = false;
    bool web_api_mode = false;
    int transpose_semitones = 0;
    std::string scale_context = "";
    std::string chord_progression = "";
    std::string export_format = "";
    std::string input_file = "";
    std::string output_file = "";
    IdentificationMode mode = IdentificationMode::STANDARD;
    
    CLIOptions() = default;
};

void printUsage(const char* program_name) {
    std::cout << "ChordLock - Advanced High-Performance Chord Identification & Music Analysis\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Basic Options:\n";
    std::cout << "  -n, --notes NOTES         Comma-separated MIDI note numbers (required)\n";
    std::cout << "  -k, --key KEY             Key context for accidentals (default: C)\n";
    std::cout << "  -f, --format FORMAT       Output format: standard, jazz, minimal, json, xml (default: standard)\n";
    std::cout << "  -m, --mode MODE           Identification mode: fast, standard, comprehensive, analytical (default: standard)\n";
    std::cout << "  -v, --verbose             Verbose output with analysis details\n";
    std::cout << "  -h, --help                Show this help message\n\n";
    
    std::cout << "Advanced Analysis:\n";
    std::cout << "  --analyze                 Detailed chord analysis with theory information\n";
    std::cout << "  --voicing-analysis        Analyze chord voicings and inversions\n";
    std::cout << "  --progression-analysis    Analyze chord progressions and suggest next chords\n";
    std::cout << "  --chord-suggestions       Enable AI-powered chord suggestions\n";
    std::cout << "  --scale-context SCALE     Analyze chords within specific scale context\n\n";
    
    std::cout << "Transposition & Key Analysis:\n";
    std::cout << "  -t, --transpose SEMITONES Transpose chord by specified semitones\n";
    std::cout << "  --key-analysis            Determine most likely key from chord progression\n";
    std::cout << "  --roman-numerals          Show Roman numeral analysis in key context\n\n";
    
    std::cout << "Input/Output:\n";
    std::cout << "  -i, --input FILE          Read notes from MIDI file or text file\n";
    std::cout << "  -o, --output FILE         Save analysis results to file\n";
    std::cout << "  --export FORMAT           Export format: midi, musicxml, lilypond, json\n";
    std::cout << "  --batch                   Batch mode: read from stdin\n";
    std::cout << "  --real-time-midi          Real-time MIDI input mode\n\n";
    
    std::cout << "Performance & Testing:\n";
    std::cout << "  --benchmark               Run performance benchmark\n";
    std::cout << "  --web-api                 Start in web API server mode\n";
    std::cout << "  -d, --dict PATH           Path to chord dictionary YAML (default: interval_dict.yaml)\n";
    std::cout << "  -a, --aliases PATH        Path to aliases YAML file\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  Basic Usage:\n";
    std::cout << "    " << program_name << " --notes \"60,64,67\"                         # C major triad\n";
    std::cout << "    " << program_name << " --notes \"60,63,67\" --key F                  # C minor in F context\n";
    std::cout << "    " << program_name << " --notes \"60,64,67,70\" --analyze             # C7 with detailed analysis\n\n";
    
    std::cout << "  Advanced Analysis:\n";
    std::cout << "    " << program_name << " --notes \"60,64,67\" --voicing-analysis       # Analyze voicing structure\n";
    std::cout << "    " << program_name << " --notes \"60,64,67\" --roman-numerals --key C # Roman numeral analysis\n";
    std::cout << "    " << program_name << " --notes \"60,64,67\" --transpose 7            # Transpose up perfect 5th\n\n";
    
    std::cout << "  Progression Analysis:\n";
    std::cout << "    echo \"60,64,67\\n67,71,74\\n65,69,72\" | " << program_name << " --batch --progression-analysis\n";
    std::cout << "    " << program_name << " --notes \"60,64,67\" --chord-suggestions      # Get next chord suggestions\n\n";
    
    std::cout << "  Export & Integration:\n";
    std::cout << "    " << program_name << " --notes \"60,64,67\" --format json --output chords.json\n";
    std::cout << "    " << program_name << " --input song.mid --export musicxml --output song.xml\n";
    std::cout << "    " << program_name << " --real-time-midi --chord-suggestions         # Live MIDI analysis\n";
}

std::pair<std::vector<int>, std::vector<std::string>> parseNotesWithWarnings(const std::string& notes_str) {
    std::vector<int> notes;
    std::vector<std::string> warnings;
    std::stringstream ss(notes_str);
    std::string note;
    
    while (std::getline(ss, note, ',')) {
        // Trim whitespace
        note.erase(0, note.find_first_not_of(" \t"));
        note.erase(note.find_last_not_of(" \t") + 1);
        
        try {
            int midi_note = std::stoi(note);
            if (midi_note >= 0 && midi_note <= 127) {
                notes.push_back(midi_note);
            } else {
                warnings.push_back("Warning: MIDI note " + std::to_string(midi_note) + " out of range (0-127)");
            }
        } catch (const std::exception& e) {
            warnings.push_back("Warning: Invalid note '" + note + "': " + e.what());
        }
    }
    
    return {notes, warnings};
}

std::vector<int> parseNotes(const std::string& notes_str) {
    auto [notes, warnings] = parseNotesWithWarnings(notes_str);
    
    // Output warnings immediately
    for (const auto& warning : warnings) {
        std::cerr << warning << "\n";
    }
    
    return notes;
}

IdentificationMode parseMode(const std::string& mode_str) {
    if (mode_str == "fast") return IdentificationMode::FAST;
    if (mode_str == "standard") return IdentificationMode::STANDARD;
    if (mode_str == "comprehensive") return IdentificationMode::COMPREHENSIVE;
    if (mode_str == "analytical") return IdentificationMode::ANALYTICAL;
    
    std::cerr << "Warning: Unknown mode '" << mode_str << "', using standard\n";
    return IdentificationMode::STANDARD;
}

void printChordResult(const ChordIdentificationResult& result, const CLIOptions& options) {
    if (options.output_format == "minimal") {
        // For minimal output, prefer full_display_name if available, otherwise use chord_name
        std::string display_name = !result.full_display_name.empty() ? result.full_display_name : result.chord_name;
        std::cout << display_name << std::endl;
        return;
    }
    
    // Use full_display_name if available (includes slash notation), otherwise use theoretical_name or chord_name
    std::string display_name;
    if (!result.full_display_name.empty()) {
        display_name = result.full_display_name;
    } else if (!result.theoretical_name.empty()) {
        display_name = result.theoretical_name;
    } else {
        display_name = result.chord_name;
    }
    
    std::cout << "Chord: " << display_name;
    
    if (result.is_slash_chord && !result.bass_note_name.empty()) {
        std::cout << " (slash chord)";
    }
    
    if (result.is_inversion) {
        std::cout << " (inversion)";
    }
    
    std::cout << "\n";
    
    if (options.verbose || options.analyze) {
        std::cout << "Confidence: " << (result.confidence * 100.0f) << "%\n";
        std::cout << "Category: " << result.chord_category << "\n";
        std::cout << "Quality: " << result.chord_quality << "\n";
        
        // Print enhanced chord analysis
        if (!result.root_note.empty()) {
            std::cout << "Root: " << result.root_note << "\n";
        }
        if (!result.chord_symbol.empty()) {
            std::cout << "Symbol: " << result.chord_symbol << "\n";
        }
        if (!result.theoretical_name.empty()) {
            std::cout << "Theoretical name: " << result.theoretical_name << "\n";
        }
        if (!result.full_display_name.empty()) {
            std::cout << "Full display name: " << result.full_display_name << "\n";
        }
        std::cout << "Is slash chord: " << (result.is_slash_chord ? "true" : "false") << "\n";
        std::cout << "Is inversion: " << (result.is_inversion ? "true" : "false") << "\n";
        if (!result.bass_note_name.empty()) {
            std::cout << "Bass note: " << result.bass_note_name << "\n";
        }
        if (result.inversion_type > 0) {
            std::cout << "Inversion: " << result.inversion_type;
            if (result.inversion_type == 1) std::cout << "st";
            else if (result.inversion_type == 2) std::cout << "nd";
            else if (result.inversion_type == 3) std::cout << "rd";
            else std::cout << "th";
            std::cout << "\n";
        }
        
        if (!result.identified_intervals.empty()) {
            std::cout << "Intervals: [";
            for (size_t i = 0; i < result.identified_intervals.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << result.identified_intervals[i];
            }
            std::cout << "]\n";
        }
        
        if (!result.note_names.empty()) {
            std::cout << "Notes: ";
            for (size_t i = 0; i < result.note_names.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << result.note_names[i];
            }
            std::cout << "\n";
        }
        
        if (!result.alternative_names.empty()) {
            std::cout << "Alternatives: ";
            for (size_t i = 0; i < result.alternative_names.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << result.alternative_names[i];
            }
            std::cout << "\n";
        }
        
        std::cout << "Processing time: " << result.processing_time.count() << " μs\n";
    }
}

void runBenchmark(ChordIdentifier& identifier) {
    std::cout << "Running ChordLock performance benchmark...\n\n";
    
    // Test patterns
    std::vector<std::pair<std::string, std::vector<int>>> test_cases = {
        {"C major", {60, 64, 67}},
        {"C minor", {60, 63, 67}},
        {"C7", {60, 64, 67, 70}},
        {"Cmaj7", {60, 64, 67, 71}},
        {"C/E (1st inv)", {64, 67, 72}},
        {"Complex chord", {60, 64, 67, 70, 74, 77, 81}}
    };
    
    for (const auto& [name, notes] : test_cases) {
        // Warm up
        for (int i = 0; i < 100; ++i) {
            identifier.identify(notes);
        }
        
        // Measure
        auto start = std::chrono::high_resolution_clock::now();
        
        const int iterations = 10000;
        for (int i = 0; i < iterations; ++i) {
            auto result = identifier.identify(notes);
            (void)result; // Suppress unused warning
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_time = total_time.count() / double(iterations);
        
        std::cout << name << ": " << avg_time << " μs/chord\n";
    }
    
    // Performance statistics
    auto stats = identifier.getPerformanceStats();
    std::cout << "\nOverall Statistics:\n";
    std::cout << "Total identifications: " << stats.total_identifications << "\n";
    std::cout << "Cache hit rate: " << (stats.cache_hit_rate * 100.0f) << "%\n";
    std::cout << "Average time: " << stats.average_processing_time.count() << " μs\n";
}

void runBatchMode(ChordIdentifier& identifier, const CLIOptions& options) {
    std::string line;
    int count = 0;
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        auto notes = parseNotes(line);
        if (notes.empty()) {
            std::cerr << "Warning: No valid notes in line: " << line << "\n";
            continue;
        }
        
        auto result = identifier.identify(notes, options.mode);
        
        if (options.verbose) {
            std::cout << "Input " << (++count) << ": ";
        }
        
        printChordResult(result, options);
        
        if (options.verbose) {
            std::cout << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    CLIOptions options;
    
    static struct option long_options[] = {
        {"notes",               required_argument, 0, 'n'},
        {"key",                 required_argument, 0, 'k'},
        {"format",              required_argument, 0, 'f'},
        {"mode",                required_argument, 0, 'm'},
        {"transpose",           required_argument, 0, 't'},
        {"input",               required_argument, 0, 'i'},
        {"output",              required_argument, 0, 'o'},
        {"dict",                required_argument, 0, 'd'},
        {"aliases",             required_argument, 0, 'a'},
        {"export",              required_argument, 0, 2000},
        {"scale-context",       required_argument, 0, 2001},
        {"verbose",             no_argument,       0, 'v'},
        {"analyze",             no_argument,       0, 1000},
        {"voicing-analysis",    no_argument,       0, 1001},
        {"progression-analysis", no_argument,      0, 1002},
        {"chord-suggestions",   no_argument,       0, 1003},
        {"key-analysis",        no_argument,       0, 1004},
        {"roman-numerals",      no_argument,       0, 1005},
        {"real-time-midi",      no_argument,       0, 1006},
        {"web-api",             no_argument,       0, 1007},
        {"batch",               no_argument,       0, 1008},
        {"benchmark",           no_argument,       0, 1009},
        {"help",                no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "n:k:f:m:t:i:o:d:a:vh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'n':
                options.notes = parseNotes(optarg);
                break;
            case 'k':
                options.key = optarg;
                break;
            case 'f':
                options.output_format = optarg;
                break;
            case 'm':
                options.mode = parseMode(optarg);
                break;
            case 't':
                options.transpose_semitones = std::stoi(optarg);
                break;
            case 'i':
                options.input_file = optarg;
                break;
            case 'o':
                options.output_file = optarg;
                break;
            case 'd':
                options.chord_dict_path = optarg;
                break;
            case 'a':
                options.aliases_path = optarg;
                break;
            case 'v':
                options.verbose = true;
                break;
            case 1000:
                options.analyze = true;
                break;
            case 1001:
                options.voicing_analysis = true;
                break;
            case 1002:
                options.progression_analysis = true;
                break;
            case 1003:
                options.chord_suggestions = true;
                break;
            case 1004:
                options.progression_analysis = true; // Key analysis is part of progression analysis
                break;
            case 1005:
                options.progression_analysis = true; // Roman numerals need progression analysis
                break;
            case 1006:
                options.real_time_midi = true;
                break;
            case 1007:
                options.web_api_mode = true;
                break;
            case 1008:
                options.batch_mode = true;
                break;
            case 1009:
                options.benchmark = true;
                break;
            case 2000:
                options.export_format = optarg;
                break;
            case 2001:
                options.scale_context = optarg;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case '?':
                std::cerr << "Use --help for usage information.\n";
                return 1;
            default:
                break;
        }
    }
    
    // Initialize chord identifier
    ChordIdentifier identifier(options.mode);
    
    if (!identifier.initialize(options.chord_dict_path, options.aliases_path)) {
        std::cerr << "Error: Failed to initialize ChordLock with dictionary: " 
                  << options.chord_dict_path << "\n";
        return 1;
    }
    
    if (options.verbose) {
        std::cout << "ChordLock initialized successfully\n";
        std::cout << "Dictionary: " << options.chord_dict_path << "\n";
        std::cout << "Mode: " << static_cast<int>(options.mode) << "\n\n";
    }
    
    // Handle different modes
    if (options.benchmark) {
        runBenchmark(identifier);
        return 0;
    }
    
    if (options.batch_mode) {
        runBatchMode(identifier, options);
        return 0;
    }
    
    // Single chord identification
    if (options.notes.empty()) {
        std::cerr << "Error: No notes specified. Use --notes or --help.\n";
        return 1;
    }
    
    // Apply transposition if requested
    auto notes_to_analyze = options.notes;
    if (options.transpose_semitones != 0) {
        for (auto& note : notes_to_analyze) {
            note += options.transpose_semitones;
            // Clamp to valid MIDI range
            note = std::max(0, std::min(127, note));
        }
    }
    
    auto result = identifier.identify(notes_to_analyze, options.mode);
    
    // Initialize output formatter
    OutputFormat format = OutputFormatter::detectFormat(options.output_format);
    OutputFormatter formatter(format);
    
    // Handle progression analysis if requested
    if (options.progression_analysis || options.chord_suggestions) {
        ProgressionAnalyzer analyzer;
        std::vector<std::vector<int>> chord_sequence = {notes_to_analyze};
        
        if (options.progression_analysis) {
            auto progression_analysis = analyzer.analyzeProgression(chord_sequence);
            auto formatted_output = formatter.formatCompleteAnalysis(
                result, notes_to_analyze, 
                progression_analysis.chord_functions.empty() ? "" : progression_analysis.chord_functions[0].roman_numeral,
                progression_analysis.suggestions
            );
            
            if (!options.output_file.empty()) {
                formatter.saveToFile(formatted_output, options.output_file);
                std::cout << "Analysis saved to: " << options.output_file << std::endl;
            } else {
                std::cout << formatted_output.content << std::endl;
            }
        } else if (options.chord_suggestions) {
            auto suggestions = analyzer.suggestNextChords(chord_sequence);
            auto formatted_output = formatter.formatCompleteAnalysis(
                result, notes_to_analyze, "", suggestions
            );
            
            if (!options.output_file.empty()) {
                formatter.saveToFile(formatted_output, options.output_file);
                std::cout << "Analysis saved to: " << options.output_file << std::endl;
            } else {
                std::cout << formatted_output.content << std::endl;
            }
        }
    } else {
        // Standard chord identification output
        auto formatted_output = formatter.formatChord(result);
        
        if (!options.output_file.empty()) {
            formatter.saveToFile(formatted_output, options.output_file);
            std::cout << "Result saved to: " << options.output_file << std::endl;
        } else {
            if (format == OutputFormat::STANDARD || format == OutputFormat::JAZZ || format == OutputFormat::MINIMAL) {
                // Use existing printChordResult for standard formats
                printChordResult(result, options);
            } else {
                // Use formatter for JSON/XML
                std::cout << formatted_output.content << std::endl;
            }
        }
    }
    
    return 0;
}