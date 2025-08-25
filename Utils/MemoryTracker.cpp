#include "MemoryTracker.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace ChordLock {

// Static member definition
MemoryTracker* MemoryTracker::instance_ = nullptr;

MemorySnapshot MemoryTracker::getCurrentSnapshot() const {
    MemorySnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.resident_memory_kb = getProcessMemoryUsage() / 1024;
    snapshot.virtual_memory_kb = getProcessVirtualMemoryUsage() / 1024;
    snapshot.peak_memory_kb = getProcessPeakMemoryUsage() / 1024;
    return snapshot;
}

void MemoryTracker::takeSnapshot(const std::string& label) {
    if (!tracking_enabled_) return;
    
    (void)label; // Suppress unused parameter warning - label could be used for future enhanced logging
    auto snapshot = getCurrentSnapshot();
    snapshots_.push_back(snapshot);
    
    // Trim snapshots if we exceed the limit
    if (snapshots_.size() > max_snapshots_) {
        trimSnapshots();
    }
}

void MemoryTracker::registerComponent(const std::string& component_name) {
    if (!tracking_enabled_) return;
    
    if (component_memory_.find(component_name) == component_memory_.end()) {
        component_memory_[component_name] = ComponentMemoryInfo(component_name);
    }
}

void MemoryTracker::updateComponentMemory(const std::string& component_name, size_t size_bytes) {
    if (!tracking_enabled_) return;
    
    registerComponent(component_name);
    component_memory_[component_name].estimated_size_bytes = size_bytes;
    component_memory_[component_name].last_updated = std::chrono::system_clock::now();
}

void MemoryTracker::incrementAllocations(const std::string& component_name, size_t count) {
    if (!tracking_enabled_) return;
    
    registerComponent(component_name);
    component_memory_[component_name].allocation_count += count;
    component_memory_[component_name].last_updated = std::chrono::system_clock::now();
}

void MemoryTracker::incrementDeallocations(const std::string& component_name, size_t count) {
    if (!tracking_enabled_) return;
    
    registerComponent(component_name);
    component_memory_[component_name].deallocation_count += count;
    component_memory_[component_name].last_updated = std::chrono::system_clock::now();
}

ComponentMemoryInfo MemoryTracker::getComponentInfo(const std::string& component_name) const {
    auto it = component_memory_.find(component_name);
    if (it != component_memory_.end()) {
        return it->second;
    }
    return ComponentMemoryInfo(component_name);
}

std::vector<ComponentMemoryInfo> MemoryTracker::getAllComponents() const {
    std::vector<ComponentMemoryInfo> components;
    components.reserve(component_memory_.size());
    
    for (const auto& [name, info] : component_memory_) {
        components.push_back(info);
    }
    
    // Sort by estimated size (largest first)
    std::sort(components.begin(), components.end(), 
              [](const ComponentMemoryInfo& a, const ComponentMemoryInfo& b) {
                  return a.estimated_size_bytes > b.estimated_size_bytes;
              });
    
    return components;
}

size_t MemoryTracker::getTotalEstimatedMemory() const {
    size_t total = 0;
    for (const auto& [name, info] : component_memory_) {
        total += info.estimated_size_bytes;
    }
    return total;
}

std::string MemoryTracker::generateMemoryReport() const {
    std::ostringstream oss;
    
    oss << "=== ChordLock Memory Usage Report ===" << std::endl;
    
    // Current system memory usage
    auto current_snapshot = getCurrentSnapshot();
    oss << "System Memory Usage:" << std::endl;
    oss << "  Resident Memory: " << current_snapshot.resident_memory_kb << " KB" << std::endl;
    oss << "  Virtual Memory:  " << current_snapshot.virtual_memory_kb << " KB" << std::endl;
    oss << "  Peak Memory:     " << current_snapshot.peak_memory_kb << " KB" << std::endl;
    oss << std::endl;
    
    // Component breakdown
    auto components = getAllComponents();
    if (!components.empty()) {
        oss << "Component Memory Breakdown:" << std::endl;
        oss << std::left << std::setw(25) << "Component" 
            << std::setw(12) << "Size (KB)" 
            << std::setw(8) << "Allocs" 
            << std::setw(8) << "Deallocs" 
            << "Balance" << std::endl;
        oss << std::string(60, '-') << std::endl;
        
        size_t total_estimated = 0;
        for (const auto& component : components) {
            total_estimated += component.estimated_size_bytes;
            
            oss << std::left << std::setw(25) << component.component_name
                << std::setw(12) << (component.estimated_size_bytes / 1024)
                << std::setw(8) << component.allocation_count
                << std::setw(8) << component.deallocation_count
                << (static_cast<int>(component.allocation_count) - static_cast<int>(component.deallocation_count))
                << std::endl;
        }
        
        oss << std::string(60, '-') << std::endl;
        oss << "Total Estimated: " << (total_estimated / 1024) << " KB" << std::endl;
        oss << std::endl;
    }
    
    // Memory leak detection
    auto potential_leaks = detectPotentialLeaks();
    if (!potential_leaks.empty()) {
        oss << "Potential Memory Leaks:" << std::endl;
        for (const auto& leak : potential_leaks) {
            oss << "  - " << leak << std::endl;
        }
        oss << std::endl;
    }
    
    // Memory snapshots summary
    if (!snapshots_.empty()) {
        oss << "Memory Snapshots (" << snapshots_.size() << " total):" << std::endl;
        size_t min_memory = snapshots_[0].resident_memory_kb;
        size_t max_memory = snapshots_[0].resident_memory_kb;
        
        for (const auto& snapshot : snapshots_) {
            min_memory = std::min(min_memory, snapshot.resident_memory_kb);
            max_memory = std::max(max_memory, snapshot.resident_memory_kb);
        }
        
        oss << "  Minimum Memory: " << min_memory << " KB" << std::endl;
        oss << "  Maximum Memory: " << max_memory << " KB" << std::endl;
        oss << "  Memory Range:   " << (max_memory - min_memory) << " KB" << std::endl;
    }
    
    return oss.str();
}

void MemoryTracker::printMemoryReport() const {
    std::cout << generateMemoryReport() << std::endl;
}

std::vector<std::string> MemoryTracker::detectPotentialLeaks() const {
    std::vector<std::string> potential_leaks;
    
    for (const auto& [name, info] : component_memory_) {
        // Check for allocation/deallocation imbalance
        int balance = static_cast<int>(info.allocation_count) - static_cast<int>(info.deallocation_count);
        if (balance > 0 && info.allocation_count > 100) { // Only flag if significant activity
            potential_leaks.push_back(name + " (unbalanced alloc/dealloc: +" + std::to_string(balance) + ")");
        }
        
        // Check for unusually large memory usage
        if (info.estimated_size_bytes > 10 * 1024 * 1024) { // > 10MB
            potential_leaks.push_back(name + " (large memory usage: " + 
                                    std::to_string(info.estimated_size_bytes / 1024 / 1024) + " MB)");
        }
    }
    
    return potential_leaks;
}

void MemoryTracker::trimSnapshots() {
    if (snapshots_.size() <= max_snapshots_) return;
    
    // Keep the most recent snapshots
    size_t to_remove = snapshots_.size() - max_snapshots_;
    snapshots_.erase(snapshots_.begin(), snapshots_.begin() + to_remove);
}

// Platform-specific memory usage implementations
#ifdef __APPLE__
size_t MemoryTracker::getProcessMemoryUsage() const {
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
        return 0;
    }
    
    return info.resident_size;
}

size_t MemoryTracker::getProcessPeakMemoryUsage() const {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss; // On macOS, this is in bytes
    }
    return 0;
}

size_t MemoryTracker::getProcessVirtualMemoryUsage() const {
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
        return 0;
    }
    
    return info.virtual_size;
}

#elif __linux__
size_t MemoryTracker::getProcessMemoryUsage() const {
    std::ifstream status_file("/proc/self/status");
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stoull(value) * 1024; // Convert KB to bytes
        }
    }
    
    return 0;
}

size_t MemoryTracker::getProcessPeakMemoryUsage() const {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // On Linux, this is in KB, convert to bytes
    }
    return 0;
}

size_t MemoryTracker::getProcessVirtualMemoryUsage() const {
    std::ifstream status_file("/proc/self/status");
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmSize:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stoull(value) * 1024; // Convert KB to bytes
        }
    }
    
    return 0;
}

#elif _WIN32
size_t MemoryTracker::getProcessMemoryUsage() const {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

size_t MemoryTracker::getProcessPeakMemoryUsage() const {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize;
    }
    return 0;
}

size_t MemoryTracker::getProcessVirtualMemoryUsage() const {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PrivateUsage;
    }
    return 0;
}

#else
// Fallback implementations for unsupported platforms
size_t MemoryTracker::getProcessMemoryUsage() const { return 0; }
size_t MemoryTracker::getProcessPeakMemoryUsage() const { return 0; }
size_t MemoryTracker::getProcessVirtualMemoryUsage() const { return 0; }
#endif

} // namespace ChordLock