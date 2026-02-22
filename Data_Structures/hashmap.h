#ifndef HASHMAP_H
#define HASHMAP_H

#include "linkedlist.h"
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>

template <typename T>
class HashMap {
private:
    static const int TABLE_SIZE = 10007;
    LinkedList<std::pair<std::string, T>> table[TABLE_SIZE];

    int hashFunc(const std::string& key) const {
        unsigned long long hash = 0;
        for (char c : key) {
            hash = (hash * 31 + static_cast<unsigned char>(c)) % TABLE_SIZE;
        }
        return static_cast<int>(hash);
    }

public:
    void put(const std::string& key, const T& value) {
        int idx = hashFunc(key);
        for (auto& p : table[idx]) {
            if (p.first == key) { p.second = value; return; }
        }
        table[idx].push_back(std::make_pair(key, value));
    }

    bool contains(const std::string& key) const {
        int idx = hashFunc(key);
        for (const auto& p : table[idx]) {
            if (p.first == key) return true;
        }
        return false;
    }

    T& operator[](const std::string& key) {
        int idx = hashFunc(key);
        for (auto& p : table[idx]) {
            if (p.first == key) return p.second;
        }
        table[idx].push_back(std::make_pair(key, T()));
        return table[idx].back().second;
    }

    T get(const std::string& key) const {
        int idx = hashFunc(key);
        for (const auto& p : table[idx]) {
            if (p.first == key) return p.second;
        }
        return T(); // Return default value (e.g., 0.0 for double) if key missing
    }

    size_t size() const {
        size_t count = 0;
        for (int i = 0; i < TABLE_SIZE; ++i) count += table[i].size();
        return count;
    }

    std::vector<std::string> getKeys() const {
        std::vector<std::string> keys;
        for (int i = 0; i < TABLE_SIZE; ++i) {
            for (const auto& p : table[i]) keys.push_back(p.first);
        }
        return keys;
    }

    void clear() {
        for (int i = 0; i < TABLE_SIZE; ++i) table[i].clear();
    }
};

#endif