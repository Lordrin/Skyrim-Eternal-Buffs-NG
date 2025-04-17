#pragma once

#include <iostream>
#include <list>
#include <unordered_map>

// Insertion: O(1) average time (amortized, due to unordered_map's constant time insert).
// Lookup: O(1) average time (due to unordered_map).
// Iteration: O(n) where n is the number of elements, because we iterate through the list of keys.
// Keeps the order of insertion.
template <typename KeyType, typename ValueType>
class OrderedMap {
private:
    using StoredValueType =
        std::conditional_t<std::is_reference_v<ValueType>,
                           std::optional<std::reference_wrapper<std::remove_reference_t<ValueType>>>, ValueType>;
    std::unordered_map<KeyType, StoredValueType> map;
    std::vector<KeyType> order;  // List to store keys in insertion order

public:
    ~OrderedMap() = default;

    OrderedMap() = default;

    // Constructor that accepts an initializer list
    OrderedMap(std::initializer_list<std::pair<const KeyType, ValueType>> initList) {
        map.reserve(initList.size());    // Reserve space in the map to avoid rehashing
        order.reserve(initList.size());  // Reserve space in the vector to avoid reallocations
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

    // void Concatenate(const OrderedMap& other) {
    //     for (const auto& item : other.map) {
    //         Insert(item.first, item.second);  // Use the existing Insert method to maintain order
    //     }
    // }

    void Concatenate(const OrderedMap& other) {
        const auto& otherMap = other.GetMap();
        for (const auto& itemKey : other.GetOrder()) {
            if constexpr (std::is_reference_v<ValueType>) {
                Insert(itemKey, otherMap.get(itemKey).value().get());  // Unwrap the reference
                // Insert(itemkey, item.second.value().get());  // Unwrap the reference
            } else {
                Insert(itemKey, otherMap.at(itemKey));
            }
        }
    }

    // void Concatenate(const OrderedMap other) {
    //     for (const auto& item : other.map) {
    //         Insert(item.first, item.second);  // Use the existing Insert method to maintain order
    //     }
    // }

    // Efficiently concatenate another OrderedMap in O(1) time
    // This is a move operation, so the other map will be empty after this operation.
    // This is useful for merging two OrderedMaps without copying the elements.
    void Concatenate_fast(OrderedMap&& other) {
        // Move the keys from the other map's order vector
        order.insert(order.end(), std::make_move_iterator(other.order.begin()),
                     std::make_move_iterator(other.order.end()));

        // Move the elements from the other map's unordered_map
        map.insert(std::make_move_iterator(other.map.begin()), std::make_move_iterator(other.map.end()));

        // Clear the other map to avoid dangling references
        other.order.clear();
        other.map.clear();
    }

    // // Lookup a value by key (O(1) lookup)
    // ValueType Lookup(const KeyType& key) const {
    //     auto it = map.find(key);
    //     if (it != map.end()) {
    //         return it->second;
    //     } else {
    //         throw std::runtime_error("Key not found");
    //     }
    // }

    // Lookup a value by key (O(1) lookup)
    ValueType Lookup(const KeyType& key) const {
        auto it = map.find(key);
        if (it != map.end()) {
            if constexpr (std::is_reference_v<ValueType>) {
                return it->second.value().get();  // Unwrap reference_wrapper for reference types
            } else {
                return it->second;
            }
        } else {
            throw std::runtime_error("Key not found");
        }
    }

    // Get map
    const std::unordered_map<KeyType, ValueType>& GetMap() const { return map; }

    // Get order
    const std::vector<KeyType>& GetOrder() const { return order; }
};
