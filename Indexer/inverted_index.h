#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include "Data_Structures/hashmap.h"
#include <string>
#include <vector>

class InvertedIndex {
private:
    HashMap<HashMap<int>> index;
    HashMap<int> docLengths;

public:
    void add(const std::string& word, const std::string& url) {
        index[word][url]++;
        docLengths[url]++;
    }

    // Changed: returns by value (HashMap<int>)
    HashMap<int> getPostings(const std::string& word) const {
        if (index.contains(word)) {
            return index.get(word); 
        }
        return HashMap<int>(); 
    }

    int getDocLength(const std::string& url) const {
        return docLengths.contains(url) ? docLengths.get(url) : 0;
    }

    size_t getDocCount() const {
        return docLengths.size();
    }

    size_t getDocumentFrequency(const std::string& word) const {
        // This is safe because it calls size() on a temporary copy
        return getPostings(word).size();
    }

    std::vector<std::string> getAllWords() const {
        return index.getKeys();
    }

    std::vector<std::string> getAllDocuments() const {
        return docLengths.getKeys();
    }

    void clear() {
        index.clear();
        docLengths.clear();
    }
};

#endif