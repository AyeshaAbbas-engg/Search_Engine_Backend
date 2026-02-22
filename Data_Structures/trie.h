#ifndef TRIE_H
#define TRIE_H

#include "../Sorter/sorter.h"
#include <string>
#include <vector>
#include <utility> 
#include <algorithm>

struct TrieNode {
    static const int ALPHA_SIZE = 36;  // 0-9 then a-z
    TrieNode* children[ALPHA_SIZE];
    bool isEndOfWord;
    int frequency; 

    TrieNode() : isEndOfWord(false), frequency(0) {
        for (int i = 0; i < ALPHA_SIZE; ++i) {
            children[i] = nullptr;
        }
    }
};

class Trie {
private:
    TrieNode* root;

    int getIndex(char c) const {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'z') return 10 + (c - 'a');
        return -1; 
    }

    char getChar(int idx) const {
        if (idx < 10) return '0' + idx;
        return 'a' + (idx - 10);
    }

    void deleteTrie(TrieNode* node) {
        if (!node) return;
        for (int i = 0; i < TrieNode::ALPHA_SIZE; ++i) {
            deleteTrie(node->children[i]);
        }
        delete node;
    }

    // Traverses the Trie to find all completed words after a prefix
    void collectWords(TrieNode* node, std::string current,
                      std::vector<std::pair<std::string, int>>& results) const {
        if (!node) return;
        if (node->isEndOfWord) {
            results.emplace_back(current, node->frequency);
        }
        for (int i = 0; i < TrieNode::ALPHA_SIZE; ++i) {
            if (node->children[i]) {
                collectWords(node->children[i], current + getChar(i), results);
            }
        }
    }

public:
    Trie() { root = new TrieNode(); }
    ~Trie() { deleteTrie(root); }

    void insert(const std::string& word) {
        if (word.empty()) return;
        TrieNode* current = root;
        for (char c : word) {
            int idx = getIndex(c);
            if (idx == -1) continue; 
            if (!current->children[idx]) {
                current->children[idx] = new TrieNode();
            }
            current = current->children[idx];
        }
        current->isEndOfWord = true;
        current->frequency++;
    }

    std::vector<std::string> getSuggestions(const std::string& prefix,
                                            size_t maxResults = 10) const {
        std::vector<std::pair<std::string, int>> candidates;

        // 1. Navigate to the end of the prefix
        TrieNode* current = root;
        for (char c : prefix) {
            int idx = getIndex(c);
            if (idx == -1 || !current->children[idx]) return {}; 
            current = current->children[idx];
        }

        // 2. DFS: Collect all words starting with this prefix
        collectWords(current, prefix, candidates);

        // 3. CUSTOM SORT: Using your QuickSort instead of std::sort
        if (!candidates.empty()) {
            Sorter::quickSort(candidates, 0, candidates.size() - 1);
        }

        // 4. Extraction
        std::vector<std::string> results;
        for (size_t i = 0; i < candidates.size() && i < maxResults; ++i) {
            results.push_back(candidates[i].first);
        }

        return results;
    }
};

#endif