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
    std::vector<KeyType> order;  // Vector to store keys in insertion order

public:
    ~OrderedMap() = default;

    OrderedMap() = default;

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

    ValueType& operator[](const KeyType& key) {
        // If the key does not exist, insert a default value
        if (map.find(key) == map.end()) {
            order.push_back(key);          // Maintain insertion order
            map[key] = StoredValueType{};  // Insert default value
        }
        return map[key];
    }

    const ValueType& operator[](const KeyType& key) const {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Key not found");
        }
    }

    auto find(const KeyType& key) const { return map.find(key); }

    // Concatenate another OrderedMap in O(n) time
    // This is a copy operation, so the other map will remain unchanged.
    // This is useful for merging two OrderedMaps while keeping the original maps intact.
    void Concatenate(const OrderedMap& other) {
        const auto& otherMap = other.GetMap();
        for (const auto& itemKey : other.GetOrder()) {
            if constexpr (std::is_reference_v<ValueType>) {
                Insert(itemKey, otherMap.get(itemKey).value().get());  // Unwrap the reference
            } else {
                Insert(itemKey, otherMap.at(itemKey));
            }
        }
    }

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

    const std::unordered_map<KeyType, ValueType>& GetMap() const { return map; }

    const std::vector<KeyType>& GetOrder() const { return order; }

    // --- Iterator support for range-based for ---
    class iterator {
    public:
        iterator(typename std::vector<KeyType>::iterator it, OrderedMap* owner) : it_(it), owner_(owner) {}

        std::pair<const KeyType&, ValueType&> operator*() { return {*it_, owner_->map[*it_]}; }

        iterator& operator++() {
            ++it_;
            return *this;
        }
        bool operator!=(const iterator& other) const { return it_ != other.it_; }
        bool operator==(const iterator& other) const { return it_ == other.it_; }

    private:
        typename std::vector<KeyType>::iterator it_;
        OrderedMap* owner_;
    };

    class const_iterator {
    public:
        const_iterator(typename std::vector<KeyType>::const_iterator it, const OrderedMap* owner)
            : it_(it), owner_(owner) {}

        // throws if something's ever out of sync
        std::pair<const KeyType&, const ValueType&> operator*() const { return {*it_, owner_->map.at(*it_)}; }

        const_iterator& operator++() {
            ++it_;
            return *this;
        }
        bool operator!=(const const_iterator& other) const { return it_ != other.it_; }
        bool operator==(const const_iterator& other) const { return it_ == other.it_; }

    private:
        typename std::vector<KeyType>::const_iterator it_;
        const OrderedMap* owner_;
    };

    iterator begin() { return iterator(order.begin(), this); }
    iterator end() { return iterator(order.end(), this); }

    const_iterator begin() const { return const_iterator(order.begin(), this); }
    const_iterator end() const { return const_iterator(order.end(), this); }
};
