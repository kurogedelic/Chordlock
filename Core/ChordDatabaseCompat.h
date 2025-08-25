#pragma once

/**
 * Compatibility layer for ChordDatabase storage backends
 * Provides unified interface for different hash implementations
 */

namespace ChordLock {

// Helper macro to access chord map based on compilation flags
#ifdef USE_ROBIN_HOOD
    #define CHORD_MAP_INSERT(intervals, name) robin_hood_map_.insert(intervals, name)
    #define CHORD_MAP_FIND(intervals) robin_hood_map_.find(intervals)
    #define CHORD_MAP_EXISTS(intervals) robin_hood_map_.find(intervals).has_value()
    #define CHORD_MAP_ITERATE(var) \
        std::vector<std::pair<std::vector<int>, std::string>> var##_temp; \
        /* TODO: Add iteration support for Robin Hood */ \
        for (const auto& var : var##_temp)
#else
    #define CHORD_MAP_INSERT(intervals, name) main_chord_map[intervals] = name
    #define CHORD_MAP_FIND(intervals) \
        [this](const std::vector<int>& i) -> std::optional<std::string> { \
            auto it = main_chord_map.find(i); \
            return it != main_chord_map.end() ? std::optional(it->second) : std::nullopt; \
        }(intervals)
    #define CHORD_MAP_EXISTS(intervals) (main_chord_map.find(intervals) != main_chord_map.end())
    #define CHORD_MAP_ITERATE(var) for (const auto& var : main_chord_map)
#endif

} // namespace ChordLock