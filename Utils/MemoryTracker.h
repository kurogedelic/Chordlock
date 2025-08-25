#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#elif __linux__
#include <sys/resource.h>
#include <fstream>
#elif _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace ChordLock {

struct MemorySnapshot {
    size_t virtual_memory_kb;      // Virtual memory usage in KB
    size_t resident_memory_kb;     // Resident (physical) memory usage in KB
    size_t peak_memory_kb;         // Peak memory usage in KB
    std::chrono::system_clock::time_point timestamp;
    
    MemorySnapshot() : virtual_memory_kb(0), resident_memory_kb(0), peak_memory_kb(0),
                       timestamp(std::chrono::system_clock::now()) {}
};

struct ComponentMemoryInfo {
    std::string component_name;
    size_t estimated_size_bytes;
    size_t allocation_count;
    size_t deallocation_count;
    std::chrono::system_clock::time_point last_updated;
    
    ComponentMemoryInfo(const std::string& name = "") 
        : component_name(name), estimated_size_bytes(0), allocation_count(0), 
          deallocation_count(0), last_updated(std::chrono::system_clock::now()) {}
};

class MemoryTracker {
private:
    static MemoryTracker* instance_;
    std::unordered_map<std::string, ComponentMemoryInfo> component_memory_;
    std::vector<MemorySnapshot> snapshots_;
    bool tracking_enabled_;
    size_t max_snapshots_;
    
    MemoryTracker() : tracking_enabled_(true), max_snapshots_(100) {}
    
public:
    static MemoryTracker& getInstance() {
        if (!instance_) {
            instance_ = new MemoryTracker();
        }
        return *instance_;
    }
    
    // System-level memory tracking
    MemorySnapshot getCurrentSnapshot() const;
    void takeSnapshot(const std::string& label = "");
    std::vector<MemorySnapshot> getSnapshots() const { return snapshots_; }
    void clearSnapshots() { snapshots_.clear(); }
    
    // Component-level memory tracking
    void registerComponent(const std::string& component_name);
    void updateComponentMemory(const std::string& component_name, size_t size_bytes);
    void incrementAllocations(const std::string& component_name, size_t count = 1);
    void incrementDeallocations(const std::string& component_name, size_t count = 1);
    ComponentMemoryInfo getComponentInfo(const std::string& component_name) const;
    std::vector<ComponentMemoryInfo> getAllComponents() const;
    
    // Analysis and reporting
    size_t getTotalEstimatedMemory() const;
    std::string generateMemoryReport() const;
    void printMemoryReport() const;
    
    // Memory leak detection
    std::vector<std::string> detectPotentialLeaks() const;
    
    // Configuration
    void setTrackingEnabled(bool enabled) { tracking_enabled_ = enabled; }
    bool isTrackingEnabled() const { return tracking_enabled_; }
    void setMaxSnapshots(size_t max_snapshots) { max_snapshots_ = max_snapshots; }
    
private:
    size_t getProcessMemoryUsage() const;
    size_t getProcessPeakMemoryUsage() const;
    size_t getProcessVirtualMemoryUsage() const;
    void trimSnapshots();
};

// RAII helper for automatic memory tracking
class ScopedMemoryTracker {
private:
    std::string component_name_;
    MemorySnapshot initial_snapshot_;
    
public:
    ScopedMemoryTracker(const std::string& component_name) 
        : component_name_(component_name) {
        auto& tracker = MemoryTracker::getInstance();
        tracker.registerComponent(component_name_);
        initial_snapshot_ = tracker.getCurrentSnapshot();
        tracker.takeSnapshot("Before " + component_name_);
    }
    
    ~ScopedMemoryTracker() {
        auto& tracker = MemoryTracker::getInstance();
        auto final_snapshot = tracker.getCurrentSnapshot();
        tracker.takeSnapshot("After " + component_name_);
        
        // Calculate memory delta
        size_t memory_delta = 0;
        if (final_snapshot.resident_memory_kb > initial_snapshot_.resident_memory_kb) {
            memory_delta = (final_snapshot.resident_memory_kb - initial_snapshot_.resident_memory_kb) * 1024;
        }
        
        tracker.updateComponentMemory(component_name_, memory_delta);
    }
};

// Memory estimation helpers for common STL containers
class MemoryEstimator {
public:
    template<typename T>
    static size_t estimateVectorMemory(const std::vector<T>& vec) {
        return sizeof(std::vector<T>) + (vec.capacity() * sizeof(T));
    }
    
    template<typename K, typename V>
    static size_t estimateUnorderedMapMemory(const std::unordered_map<K, V>& map) {
        size_t base_size = sizeof(std::unordered_map<K, V>);
        size_t entry_size = (sizeof(K) + sizeof(V) + sizeof(void*)) * map.size(); // Approximate bucket overhead
        return base_size + entry_size;
    }
    
    // Overload for unordered maps with custom hash functions
    template<typename K, typename V, typename Hash, typename KeyEqual, typename Alloc>
    static size_t estimateUnorderedMapMemory(const std::unordered_map<K, V, Hash, KeyEqual, Alloc>& map) {
        size_t base_size = sizeof(std::unordered_map<K, V, Hash, KeyEqual, Alloc>);
        size_t entry_size = (sizeof(K) + sizeof(V) + sizeof(void*)) * map.size(); // Approximate bucket overhead
        return base_size + entry_size;
    }
    
    static size_t estimateStringMemory(const std::string& str) {
        return sizeof(std::string) + str.capacity();
    }
    
    template<typename T>
    static size_t estimateArrayMemory(const T* array, size_t count) {
        (void)array; // Suppress unused parameter warning
        return count * sizeof(T);
    }
};

// Macros for convenient memory tracking
#define TRACK_MEMORY_SCOPE(name) ScopedMemoryTracker _memory_tracker(name)
#define TRACK_COMPONENT_MEMORY(component, size) \
    MemoryTracker::getInstance().updateComponentMemory(component, size)
#define TRACK_ALLOCATION(component) \
    MemoryTracker::getInstance().incrementAllocations(component)
#define TRACK_DEALLOCATION(component) \
    MemoryTracker::getInstance().incrementDeallocations(component)

} // namespace ChordLock