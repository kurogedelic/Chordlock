#include "ChordDatabase.h"
#ifndef USE_COMPILED_TABLES
#include <yaml-cpp/yaml.h>
#endif
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <set>
#include <algorithm>

namespace ChordLock {

ChordDatabase::ChordDatabase() {
#ifdef USE_COMPILED_TABLES
    loadCompiledChords();
#endif
    buildAliasResolution();
    calculateQualityScores();
}

bool ChordDatabase::loadFromYaml(const std::string& filepath) {
#ifdef USE_COMPILED_TABLES
    // When using compiled tables, just return success
    (void)filepath; // Suppress unused parameter warning
    return true;
#else
    try {
        YAML::Node yaml_data = YAML::LoadFile(filepath);
        
        for (auto it = yaml_data.begin(); it != yaml_data.end(); ++it) {
            std::string intervals_str = it->first.as<std::string>();
            std::string chord_name = it->second.as<std::string>();
            
            ChordInfo chord_info = parseChordEntry(intervals_str, chord_name);
            if (!chord_info.intervals.empty()) {
                chord_info_map[chord_name] = chord_info;
                main_chord_map[chord_info.intervals] = chord_name;
                
                // Add to bloom filter
                known_intervals_filter.add(chord_info.intervals);
            }
        }
        
        buildInversionTables();
        populateBloomFilter();
        return true;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error loading chord database: " << e.what() << std::endl;
        return false;
    }
#endif
}

bool ChordDatabase::loadFromYaml(const std::string& chord_dict_path, const std::string& aliases_path) {
#ifdef USE_COMPILED_TABLES
    // When using compiled tables, ignore file paths
    (void)chord_dict_path;
    (void)aliases_path;
    return true;
#else
    // Load main chord dictionary
    if (!loadFromYaml(chord_dict_path)) {
        return false;
    }
    
    // Load aliases if provided
    if (!aliases_path.empty()) {
        try {
            YAML::Node aliases_data = YAML::LoadFile(aliases_path);
            
            for (auto it = aliases_data.begin(); it != aliases_data.end(); ++it) {
                std::string canonical = it->first.as<std::string>();
                auto aliases = it->second.as<std::vector<std::string>>();
                
                for (const auto& alias : aliases) {
                    addAlias(canonical, alias);
                }
            }
        } catch (const YAML::Exception& e) {
            std::cerr << "Aliases YAML parsing error: " << e.what() << std::endl;
            // Continue without aliases
        }
    }
    
    return true;
#endif
}

ChordInfo ChordDatabase::parseChordEntry(const std::string& intervals_str, const std::string& chord_name) {
    ChordInfo info;
    info.name = chord_name;
    
    // Parse interval string like "[0,4,7]"
    info.intervals = parseIntervalString(intervals_str);
    
    // Extract category from chord name
    if (chord_name.find("triad") != std::string::npos) {
        info.category = "triad";
    } else if (chord_name.find("seventh") != std::string::npos) {
        info.category = "seventh";
    } else if (chord_name.find("ninth") != std::string::npos || 
               chord_name.find("eleventh") != std::string::npos ||
               chord_name.find("thirteenth") != std::string::npos) {
        info.category = "extended";
    } else if (chord_name.find("scale") != std::string::npos) {
        info.category = "scale";
    } else {
        info.category = "other";
    }
    
    return info;
}

std::vector<int> ChordDatabase::parseIntervalString(const std::string& intervals_str) {
    std::vector<int> intervals;
    
    // Remove brackets and parse comma-separated integers
    std::regex bracket_regex(R"(\[([^\]]+)\])");
    std::smatch match;
    
    if (std::regex_search(intervals_str, match, bracket_regex)) {
        std::string content = match[1].str();
        std::stringstream ss(content);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            
            try {
                int interval = std::stoi(item);
                if (interval >= 0 && interval < 24) { // Allow up to 2 octaves
                    intervals.push_back(interval % 12); // Normalize to single octave
                }
            } catch (const std::exception&) {
                // Skip invalid intervals
            }
        }
    }
    
    // Sort and remove duplicates
    std::sort(intervals.begin(), intervals.end());
    intervals.erase(std::unique(intervals.begin(), intervals.end()), intervals.end());
    
    return intervals;
}

void ChordDatabase::addChord(const std::string& name, const std::vector<int>& intervals) {
    ChordInfo info(name, intervals);
    chord_info_map[name] = info;
    main_chord_map[intervals] = name;
    known_intervals_filter.add(intervals);
}

void ChordDatabase::addAlias(const std::string& canonical_name, const std::string& alias) {
    alias_to_canonical[alias] = canonical_name;
    canonical_to_aliases[canonical_name].push_back(alias);
    
    // Update chord info
    if (chord_info_map.find(canonical_name) != chord_info_map.end()) {
        chord_info_map[canonical_name].aliases.push_back(alias);
    }
}

std::optional<ChordMatch> ChordDatabase::findExactMatch(const std::vector<int>& intervals) const {
    // Try exact match with extended intervals first
    auto exact_match = findExactMatchInternal(intervals);
    if (exact_match) {
        return exact_match;
    }
    
    // Try with basic intervals (for wide voicings)
    auto basic_intervals = createBasicIntervals(intervals);
    if (basic_intervals != intervals) {
        auto basic_match = findExactMatchInternal(basic_intervals);
        if (basic_match) {
            basic_match->confidence *= 0.95f; // Slight penalty for wide voicing
            return basic_match;
        }
    }
    
    return std::nullopt;
}

std::optional<ChordMatch> ChordDatabase::findExactMatchInternal(const std::vector<int>& intervals) const {
    // Quick bloom filter check
    if (!known_intervals_filter.mayContain(intervals)) {
        return std::nullopt;
    }
    
    // Check cache first
    std::string* cached = lookup_cache.get(intervals);
    if (cached && !cached->empty()) {
        ChordMatch match;
        match.chord_info = chord_info_map.at(*cached);
        match.confidence = 1.0f;
        return match;
    }
    
    // Fast common chord lookup
    auto fast_result = fastCommonLookup(intervals);
    if (fast_result) {
        ChordMatch match;
        match.chord_info.name = *fast_result;
        match.chord_info.intervals = intervals;
        match.confidence = 1.0f;
        return match;
    }
    
    // Main lookup
    auto it = main_chord_map.find(intervals);
    if (it != main_chord_map.end()) {
        ChordMatch match;
        auto info_it = chord_info_map.find(it->second);
        if (info_it != chord_info_map.end()) {
            match.chord_info = info_it->second;
            match.confidence = 1.0f;
            
            // Cache the result
            lookup_cache.put(intervals, it->second);
            
            return match;
        }
    }
    
    return std::nullopt;
}

float ChordDatabase::calculateChordPriority(const std::string& chord_name, size_t interval_count) const {
    // Base confidence is 1.0
    float confidence = 1.0f;
    
    // Prioritize more specific chord names for larger interval sets
    if (interval_count >= 5) {
        // For 5+ note chords, prioritize specific extended chords
        if (chord_name.find("six-nine") != std::string::npos) {
            confidence = 1.1f; // Prefer 6/9 over major9
        } else if (chord_name.find("sus4-add9") != std::string::npos) {
            confidence = 1.1f; // Prefer sus4add9 over generic patterns
        } else if (chord_name.find("flat9") != std::string::npos && chord_name.find("sharp11") != std::string::npos) {
            confidence = 1.05f; // Complex altered chords get priority
        } else if (chord_name.find("major-ninth") != std::string::npos && interval_count == 5) {
            confidence = 0.9f; // Slight penalty for M9 with exactly 5 notes (might be 6/9)
        }
    }
    
    // Penalty for overly generic matches
    if (chord_name == "major-triad" && interval_count > 3) {
        confidence = 0.8f; // Penalize basic triad match for complex intervals
    }
    
    return confidence;
}

std::vector<ChordMatch> ChordDatabase::findMatches(const std::vector<int>& intervals, bool include_inversions) const {
    std::vector<ChordMatch> matches;
    
    // Try exact match first
    auto exact_match = findExactMatch(intervals);
    if (exact_match) {
        // Apply priority weighting for more specific chord types
        exact_match->confidence = calculateChordPriority(exact_match->chord_info.name, intervals.size());
        matches.push_back(*exact_match);
    }
    
    // Try inversions if requested and no exact match
    if (include_inversions && matches.empty()) {
        auto inversion_match = findWithInversion(intervals);
        if (inversion_match) {
            matches.push_back(*inversion_match);
        }
    }
    
    // Try fuzzy matching as fallback
    if (matches.empty()) {
        auto fuzzy_matches = findFuzzyMatches(intervals);
        matches.insert(matches.end(), fuzzy_matches.begin(), fuzzy_matches.end());
    }
    
    return matches;
}

std::vector<ChordMatch> ChordDatabase::findBestMatches(const std::vector<int>& intervals, size_t max_results) const {
    auto all_matches = findMatches(intervals, true);
    
    // Sort by confidence
    std::sort(all_matches.begin(), all_matches.end(), 
              [](const ChordMatch& a, const ChordMatch& b) {
                  return a.confidence > b.confidence;
              });
    
    // Return top matches
    if (all_matches.size() > max_results) {
        all_matches.resize(max_results);
    }
    
    return all_matches;
}

std::optional<ChordMatch> ChordDatabase::findWithInversion(const std::vector<int>& intervals) const {
    if (intervals.empty()) return std::nullopt;
    
    // Generate all possible rotations
    for (size_t rotation = 1; rotation < intervals.size(); ++rotation) {
        std::vector<int> rotated = intervals;
        
        // Rotate and normalize
        std::rotate(rotated.begin(), rotated.begin() + rotation, rotated.end());
        
        // Normalize to start with 0
        if (!rotated.empty() && rotated[0] != 0) {
            int offset = rotated[0];
            for (int& interval : rotated) {
                interval = (interval - offset + 12) % 12;
            }
            std::sort(rotated.begin(), rotated.end());
        }
        
        // Check for match
        auto match = findExactMatch(rotated);
        if (match) {
            match->is_inversion = true;
            match->bass_interval = intervals[0]; // Original bass
            match->confidence *= 0.9f; // Slight penalty for inversion
            return match;
        }
    }
    
    return std::nullopt;
}

std::vector<ChordMatch> ChordDatabase::findFuzzyMatches(const std::vector<int>& intervals, float min_confidence) const {
    std::vector<ChordMatch> matches;
    
    for (const auto& [chord_intervals, chord_name] : main_chord_map) {
        float similarity = calculateSimilarity(intervals, chord_intervals);
        
        if (similarity >= min_confidence) {
            ChordMatch match;
            auto info_it = chord_info_map.find(chord_name);
            if (info_it != chord_info_map.end()) {
                match.chord_info = info_it->second;
                match.confidence = similarity;
                
                // Calculate missing and extra notes
                std::set<int> input_set(intervals.begin(), intervals.end());
                std::set<int> chord_set(chord_intervals.begin(), chord_intervals.end());
                
                std::set_difference(chord_set.begin(), chord_set.end(),
                                  input_set.begin(), input_set.end(),
                                  std::back_inserter(match.missing_notes));
                
                std::set_difference(input_set.begin(), input_set.end(),
                                  chord_set.begin(), chord_set.end(),
                                  std::back_inserter(match.extra_notes));
                
                matches.push_back(match);
            }
        }
    }
    
    return matches;
}

float ChordDatabase::calculateSimilarity(const std::vector<int>& a, const std::vector<int>& b) const {
    if (a.empty() && b.empty()) return 1.0f;
    if (a.empty() || b.empty()) return 0.0f;
    
    std::set<int> set_a(a.begin(), a.end());
    std::set<int> set_b(b.begin(), b.end());
    
    // Calculate Jaccard similarity
    std::vector<int> intersection;
    std::vector<int> union_set;
    
    std::set_intersection(set_a.begin(), set_a.end(),
                         set_b.begin(), set_b.end(),
                         std::back_inserter(intersection));
    
    std::set_union(set_a.begin(), set_a.end(),
                   set_b.begin(), set_b.end(),
                   std::back_inserter(union_set));
    
    if (union_set.empty()) return 0.0f;
    
    return static_cast<float>(intersection.size()) / static_cast<float>(union_set.size());
}

std::string ChordDatabase::resolveAlias(const std::string& chord_name) const {
    auto it = alias_to_canonical.find(chord_name);
    return (it != alias_to_canonical.end()) ? it->second : chord_name;
}

std::vector<std::string> ChordDatabase::getAliases(const std::string& canonical_name) const {
    auto it = canonical_to_aliases.find(canonical_name);
    return (it != canonical_to_aliases.end()) ? it->second : std::vector<std::string>{};
}

void ChordDatabase::buildInversionTables() {
    for (const auto& [intervals, chord_name] : main_chord_map) {
        // Generate all inversions for this chord
        for (size_t i = 1; i < intervals.size(); ++i) {
            std::vector<int> inversion = intervals;
            std::rotate(inversion.begin(), inversion.begin() + i, inversion.end());
            
            // Normalize
            if (!inversion.empty() && inversion[0] != 0) {
                int offset = inversion[0];
                for (int& interval : inversion) {
                    interval = (interval - offset + 12) % 12;
                }
                std::sort(inversion.begin(), inversion.end());
            }
            
            inversion_to_root[inversion] = chord_name;
        }
    }
}

void ChordDatabase::buildAliasResolution() {
    // Common chord aliases
    addAlias("major-triad", "M");
    addAlias("major-triad", "maj");
    addAlias("major-triad", "");
    
    addAlias("minor-triad", "m");
    addAlias("minor-triad", "min");
    addAlias("minor-triad", "-");
    
    addAlias("dominant-seventh", "7");
    addAlias("major-seventh", "M7");
    addAlias("major-seventh", "maj7");
    addAlias("major-seventh", "Δ7");
    
    addAlias("minor-seventh", "m7");
    addAlias("minor-seventh", "min7");
    addAlias("minor-seventh", "-7");
    
    addAlias("diminished-triad", "dim");
    addAlias("diminished-triad", "°");
    
    addAlias("augmented-triad", "aug");
    addAlias("augmented-triad", "+");
}

void ChordDatabase::calculateQualityScores() {
    // Assign quality scores based on chord commonality
    std::unordered_map<std::string, float> scores = {
        {"major-triad", 1.0f},
        {"minor-triad", 1.0f},
        {"dominant-seventh", 0.9f},
        {"major-seventh", 0.8f},
        {"minor-seventh", 0.8f},
        {"diminished-triad", 0.7f},
        {"augmented-triad", 0.6f},
        {"sus4-triad", 0.7f},
        {"sus2-triad", 0.6f}
    };
    
    for (auto& [name, info] : chord_info_map) {
        auto it = scores.find(name);
        info.quality_score = (it != scores.end()) ? it->second : 0.5f;
    }
}

void ChordDatabase::populateBloomFilter() {
    for (const auto& [intervals, chord_name] : main_chord_map) {
        known_intervals_filter.add(intervals);
    }
}

bool ChordDatabase::validateDatabase() const {
    bool valid = true;
    
    // Check for empty intervals
    for (const auto& [intervals, chord_name] : main_chord_map) {
        if (intervals.empty()) {
            std::cerr << "Warning: Empty intervals for chord " << chord_name << std::endl;
            valid = false;
        }
    }
    
    // Check for missing chord info
    for (const auto& [intervals, chord_name] : main_chord_map) {
        if (chord_info_map.find(chord_name) == chord_info_map.end()) {
            std::cerr << "Warning: Missing chord info for " << chord_name << std::endl;
            valid = false;
        }
    }
    
    return valid;
}

void ChordDatabase::clearCaches() {
    lookup_cache = LRUChordCache<1024>{};
}

void ChordDatabase::warmupCache(const std::vector<std::vector<int>>& common_patterns) {
    for (const auto& pattern : common_patterns) {
        findExactMatch(pattern);
    }
}

std::vector<ChordMatch> ChordDatabase::findWithTensions(const std::vector<int>& intervals) const {
    // Simplified implementation - can be enhanced
    return findFuzzyMatches(intervals, 0.5f);
}

std::vector<ChordMatch> ChordDatabase::findWithOmissions(const std::vector<int>& intervals) const {
    // Simplified implementation - can be enhanced  
    return findFuzzyMatches(intervals, 0.4f);
}

std::vector<std::string> ChordDatabase::getAllChordNames() const {
    std::vector<std::string> names;
    names.reserve(chord_info_map.size());
    
    for (const auto& [name, info] : chord_info_map) {
        names.push_back(name);
    }
    
    return names;
}

float ChordDatabase::getChordQuality(const std::string& chord_name) const {
    auto it = chord_info_map.find(chord_name);
    return (it != chord_info_map.end()) ? it->second.quality_score : 0.0f;
}

std::vector<int> ChordDatabase::createBasicIntervals(const std::vector<int>& extended_intervals) const {
    std::vector<int> basic_intervals;
    std::set<int> unique_classes;
    
    for (int interval : extended_intervals) {
        // Reduce to basic interval class (0-11)
        int basic = interval % 12;
        unique_classes.insert(basic);
    }
    
    basic_intervals.assign(unique_classes.begin(), unique_classes.end());
    return basic_intervals;
}

#ifdef USE_COMPILED_TABLES
void ChordDatabase::loadCompiledChords() {
    // Load all compiled chords
    for (size_t i = 0; i < COMPILED_CHORDS_COUNT; ++i) {
        const auto& entry = COMPILED_CHORDS[i];
        
        ChordInfo info;
        info.name = entry.second;
        info.intervals = entry.first;
        
        // Determine category from name
        std::string name = info.name;
        if (name.find("triad") != std::string::npos) {
            info.category = "triad";
        } else if (name.find("seventh") != std::string::npos) {
            info.category = "seventh";
        } else if (name.find("ninth") != std::string::npos || 
                   name.find("eleventh") != std::string::npos ||
                   name.find("thirteenth") != std::string::npos) {
            info.category = "extended";
        } else if (name.find("scale") != std::string::npos) {
            info.category = "scale";
        } else {
            info.category = "other";
        }
        
        chord_info_map[info.name] = info;
        main_chord_map[info.intervals] = info.name;
        known_intervals_filter.add(info.intervals);
    }
    
    buildInversionTables();
}
#endif

} // namespace ChordLock