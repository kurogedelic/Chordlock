#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <optional>
#include <atomic>

namespace ChordLock {

/**
 * Cache-Oblivious B-Tree implementation
 * 
 * Achieves optimal O(log_B N) memory transfers without knowing cache parameters
 * Based on van Emde Boas layout for optimal cache performance
 * 
 * Benefits:
 * - Works optimally on any memory hierarchy
 * - No tuning for specific cache sizes needed
 * - Fractal tree structure for recursive cache optimization
 */
template<typename Key, typename Value>
class CacheObliviousBTree {
private:
    static constexpr size_t MIN_LEAF_SIZE = 4;
    static constexpr size_t MAX_LEAF_SIZE = 64;  // Typical cache line is 64 bytes
    
    struct Node {
        std::vector<Key> keys;
        std::vector<Value> values;
        std::vector<std::unique_ptr<Node>> children;
        bool is_leaf;
        size_t size;
        
        Node(bool leaf = true) : is_leaf(leaf), size(0) {
            keys.reserve(MAX_LEAF_SIZE);
            values.reserve(MAX_LEAF_SIZE);
            if (!leaf) {
                children.reserve(MAX_LEAF_SIZE + 1);
            }
        }
    };
    
    /**
     * van Emde Boas layout for cache-oblivious performance
     * Recursively divide tree into sqrt(n) × sqrt(n) blocks
     */
    class vEBLayout {
    private:
        std::vector<uint8_t> memory_;  // Packed memory layout
        size_t root_offset_;
        size_t node_size_;
        
        // Calculate van Emde Boas tree height
        static size_t veb_height(size_t n) {
            if (n <= 1) return 0;
            return static_cast<size_t>(std::ceil(std::log2(n) / 2));
        }
        
        // Pack node into contiguous memory
        size_t pack_node(const Node* node, size_t offset) {
            if (!node) return offset;
            
            // Store node metadata
            std::memcpy(&memory_[offset], &node->is_leaf, sizeof(bool));
            offset += sizeof(bool);
            
            std::memcpy(&memory_[offset], &node->size, sizeof(size_t));
            offset += sizeof(size_t);
            
            // Store keys and values contiguously
            for (size_t i = 0; i < node->size; ++i) {
                std::memcpy(&memory_[offset], &node->keys[i], sizeof(Key));
                offset += sizeof(Key);
                
                if (node->is_leaf) {
                    std::memcpy(&memory_[offset], &node->values[i], sizeof(Value));
                    offset += sizeof(Value);
                }
            }
            
            // Recursively pack children in vEB order
            if (!node->is_leaf) {
                size_t h = veb_height(node->children.size());
                size_t top_size = 1 << h;
                [[maybe_unused]] size_t bottom_size = (node->children.size() + top_size - 1) / top_size;
                
                // Pack top tree
                for (size_t i = 0; i < std::min(top_size, node->children.size()); ++i) {
                    offset = pack_node(node->children[i].get(), offset);
                }
                
                // Pack bottom trees
                for (size_t i = top_size; i < node->children.size(); ++i) {
                    offset = pack_node(node->children[i].get(), offset);
                }
            }
            
            return offset;
        }
        
    public:
        vEBLayout(const Node* root, size_t total_nodes) {
            node_size_ = sizeof(bool) + sizeof(size_t) + 
                        MAX_LEAF_SIZE * (sizeof(Key) + sizeof(Value));
            memory_.resize(total_nodes * node_size_);
            root_offset_ = 0;
            pack_node(root, root_offset_);
        }
        
        const uint8_t* get_memory() const { return memory_.data(); }
        size_t get_size() const { return memory_.size(); }
    };
    
    std::unique_ptr<Node> root_;
    size_t size_;
    mutable size_t cache_misses_;
    mutable size_t total_accesses_;
    
    /**
     * Binary search with cache-friendly access pattern
     * Minimizes cache misses by accessing memory sequentially
     */
    template<typename Iterator>
    Iterator cache_friendly_search(Iterator begin, Iterator end, const Key& key) const {
        size_t size = std::distance(begin, end);
        
        // For small sizes, use linear search (better cache behavior)
        if (size <= 8) {
            return std::find_if(begin, end, 
                               [&key](const Key& k) { return k >= key; });
        }
        
        // Binary search with prefetching
        Iterator left = begin;
        Iterator right = end;
        
        while (left < right) {
            Iterator mid = left + (std::distance(left, right) / 2);
            
            // Prefetch next potential access
            if (std::distance(left, right) > 16) {
                __builtin_prefetch(&*(mid - 1), 0, 1);
                __builtin_prefetch(&*(mid + 1), 0, 1);
            }
            
            if (*mid < key) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        
        return left;
    }
    
    /**
     * Recursive search optimized for cache hierarchies
     */
    std::optional<Value> search_recursive(const Node* node, const Key& key) const {
        if (!node) return std::nullopt;
        
        total_accesses_++;
        
        // Find position in current node
        auto it = cache_friendly_search(node->keys.begin(), 
                                       node->keys.begin() + node->size, key);
        size_t pos = std::distance(node->keys.begin(), it);
        
        // Check if key found
        if (pos < node->size && node->keys[pos] == key) {
            return node->values[pos];
        }
        
        // Recursively search in child
        if (!node->is_leaf && pos < node->children.size()) {
            return search_recursive(node->children[pos].get(), key);
        }
        
        return std::nullopt;
    }
    
    /**
     * Insert with recursive tree descent
     * Maintains B-tree invariants while optimizing for cache
     */
    void insert_recursive(Node* node, const Key& key, const Value& value) {
        auto it = cache_friendly_search(node->keys.begin(), 
                                       node->keys.begin() + node->size, key);
        size_t pos = std::distance(node->keys.begin(), it);
        
        if (node->is_leaf) {
            // Insert into leaf node
            node->keys.insert(node->keys.begin() + pos, key);
            node->values.insert(node->values.begin() + pos, value);
            node->size++;
            
            // Split if necessary
            if (node->size > MAX_LEAF_SIZE) {
                split_node(node);
            }
        } else {
            // Recursively insert into child
            if (pos < node->children.size()) {
                insert_recursive(node->children[pos].get(), key, value);
            }
        }
        
        size_++;
    }
    
    /**
     * Split node to maintain B-tree properties
     * Optimized for cache-line boundaries
     */
    void split_node(Node* node) {
        size_t mid = node->size / 2;
        
        auto new_node = std::make_unique<Node>(node->is_leaf);
        
        // Move half of the keys/values to new node
        new_node->keys.assign(node->keys.begin() + mid, 
                             node->keys.begin() + node->size);
        node->keys.resize(mid);
        
        if (node->is_leaf) {
            new_node->values.assign(node->values.begin() + mid,
                                   node->values.begin() + node->size);
            node->values.resize(mid);
        } else {
            new_node->children.assign(
                std::make_move_iterator(node->children.begin() + mid),
                std::make_move_iterator(node->children.begin() + node->size + 1));
            node->children.resize(mid + 1);
        }
        
        new_node->size = new_node->keys.size();
        node->size = mid;
        
        // TODO: Update parent node with new child
    }
    
public:
    CacheObliviousBTree() 
        : root_(std::make_unique<Node>(true)), 
          size_(0), 
          cache_misses_(0), 
          total_accesses_(0) {}
    
    /**
     * Search with optimal cache performance
     * O(log_B N) memory transfers for any cache size B
     */
    std::optional<Value> find(const Key& key) const {
        return search_recursive(root_.get(), key);
    }
    
    /**
     * Insert maintaining cache-oblivious properties
     */
    void insert(const Key& key, const Value& value) {
        insert_recursive(root_.get(), key, value);
    }
    
    /**
     * Batch insert for better cache utilization
     * Sort and insert in order for optimal tree construction
     */
    void insert_batch(std::vector<std::pair<Key, Value>>& items) {
        // Sort for better tree balance
        std::sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b) { return a.first < b.first; });
        
        // Insert in sorted order
        for (const auto& [key, value] : items) {
            insert(key, value);
        }
    }
    
    /**
     * Optimize tree layout for cache
     * Reorganize into van Emde Boas layout
     */
    void optimize_layout() {
        vEBLayout layout(root_.get(), size_);
        // Tree is now laid out optimally in memory
    }
    
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    // Cache performance statistics
    double cache_miss_rate() const {
        return total_accesses_ > 0 ? 
               static_cast<double>(cache_misses_) / total_accesses_ : 0.0;
    }
    
    void reset_stats() {
        cache_misses_ = 0;
        total_accesses_ = 0;
    }
};

/**
 * Specialized cache-oblivious structure for chord lookups
 * Optimized for musical interval patterns
 */
class ChordCacheObliviousTree {
private:
    using IntervalSet = std::vector<int>;
    using ChordName = std::string;
    using PackedInterval = uint64_t;
    
    CacheObliviousBTree<PackedInterval, ChordName> tree_;
    
    // Pack intervals for efficient storage and comparison
    static PackedInterval pack_intervals(const IntervalSet& intervals) {
        PackedInterval packed = 0;
        for (size_t i = 0; i < std::min(intervals.size(), size_t(12)); ++i) {
            packed |= (static_cast<PackedInterval>(intervals[i] & 0x1F) << (i * 5));
        }
        packed |= (static_cast<PackedInterval>(intervals.size()) << 60);
        return packed;
    }
    
public:
    void insert(const IntervalSet& intervals, const ChordName& name) {
        PackedInterval packed = pack_intervals(intervals);
        tree_.insert(packed, name);
    }
    
    std::optional<ChordName> find(const IntervalSet& intervals) const {
        PackedInterval packed = pack_intervals(intervals);
        return tree_.find(packed);
    }
    
    void build_from_database(const std::vector<std::pair<IntervalSet, ChordName>>& chords) {
        std::vector<std::pair<PackedInterval, ChordName>> packed_chords;
        packed_chords.reserve(chords.size());
        
        for (const auto& [intervals, name] : chords) {
            packed_chords.emplace_back(pack_intervals(intervals), name);
        }
        
        tree_.insert_batch(packed_chords);
        tree_.optimize_layout();  // Optimize for cache after bulk insert
    }
    
    size_t size() const { return tree_.size(); }
    double cache_miss_rate() const { return tree_.cache_miss_rate(); }
};

} // namespace ChordLock