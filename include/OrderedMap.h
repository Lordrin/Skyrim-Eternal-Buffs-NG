#pragma once

#include <list>
#include <iostream>
#include <unordered_map>

// Insertion: O(1) average time (amortized, due to unordered_map's constant time insert).
// Lookup: O(1) average time (due to unordered_map).
// Iteration: O(n) where n is the number of elements, because we iterate through the list of keys.
// Keeps the order of insertion.
template <typename KeyType, typename ValueType>
class OrderedMap {
private:
    std::unordered_map<KeyType, ValueType> map;
    std::vector<KeyType> order;  // List to store keys in insertion order

public:
    // Default constructor
    OrderedMap() = default;

    // Constructor that accepts an initializer list
    OrderedMap(std::initializer_list<std::pair<const KeyType, ValueType>> initList) {
        for (const auto& item : initList) {
            Insert(item.first, item.second);
        }
    }
    // Insert a value with its associated key
    void Insert(const KeyType& key, const ValueType& value) {
        if (map.find(key) == map.end()) {
            order.push_back(key);  // Maintain the order of insertion
        }
        map[key] = value;  // Insert or update the value in the unordered_map
    }

    // Lookup a value by key (O(1) lookup)
    ValueType Lookup(const KeyType& key) const {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Key not found");
        }
    }

    // Get map
    const std::unordered_map<KeyType, ValueType>& GetMap() const {
        return map;
    }

    // Get order
    const std::vector<KeyType>& GetOrder() const {
        return order;
    }
};
