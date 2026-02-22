#ifndef HASHSET_H
#define HASHSET_H

#include <list>
#include <string>
#include <vector>

class HashSet {
private:
    static const int TABLE_SIZE = 10007;  // Same as HashMap – consistent!
    std::list<std::string> table[TABLE_SIZE];

    int hashFunc(const std::string& key) const {
        unsigned long long hash = 0;
        for (char c : key) {
            hash = (hash * 31 + static_cast<unsigned char>(c)) % TABLE_SIZE;
        }
        return static_cast<int>(hash);
    }

public:
    // Insert key if not already present
    void insert(const std::string& key) {
        int idx = hashFunc(key);
        for (const auto& k : table[idx]) {
            if (k == key) {
                return;  // Already exists
            }
        }
        table[idx].emplace_back(key);
    }

    // Check existence
    bool contains(const std::string& key) const {
        int idx = hashFunc(key);
        for (const auto& k : table[idx]) {
            if (k == key) {
                return true;
            }
        }
        return false;
    }

    // Return all elements (useful for debugging or displaying vocabulary)
    std::vector<std::string> getAll() const {
        std::vector<std::string> all;
        all.reserve(size());  // Optimization: avoid reallocations
        for (int i = 0; i < TABLE_SIZE; ++i) {
            for (const auto& str : table[i]) {
                all.push_back(str);
            }
        }
        return all;
    }

    // Number of unique elements
    size_t size() const {
        size_t count = 0;
        for (int i = 0; i < TABLE_SIZE; ++i) {
            count += table[i].size();
        }
        return count;
    }

    // Clear the entire set
    void clear() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            table[i].clear();
        }
    }

    // Optional: remove a key
    bool remove(const std::string& key) {
        int idx = hashFunc(key);
        auto& bucket = table[idx];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (*it == key) {
                bucket.erase(it);
                return true;
            }
        }
        return false;
    }
};

#endif