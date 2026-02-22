#ifndef GRAPH_H
#define GRAPH_H

#include "hashmap.h"
#include "linkedlist.h"
#include <string>
#include <vector>

class Graph {
private:
    HashMap<LinkedList<std::string>> adjList;
    HashMap<LinkedList<std::string>> reverseAdjList;

public:
    void addEdge(const std::string& from, const std::string& to) {
        adjList[from].push_back(to);
        reverseAdjList[to].push_back(from);
    }

    // Returns by value (LinkedList<std::string>)
    LinkedList<std::string> getNeighbors(const std::string& url) const {
        if (adjList.contains(url)) {
            return adjList.get(url); // Returns a move-ready copy
        }
        return LinkedList<std::string>(); // Return empty list
    }

    // Returns by value (LinkedList<std::string>)
    LinkedList<std::string> getIncoming(const std::string& url) const {
        if (reverseAdjList.contains(url)) {
            return reverseAdjList.get(url);
        }
        return LinkedList<std::string>();
    }

    std::vector<std::string> getAllNodes() const {
        std::vector<std::string> nodes = adjList.getKeys();
        std::vector<std::string> incoming = reverseAdjList.getKeys();

        for (auto &n : incoming) {
            bool found = false;
            for (auto &m : nodes) {
                if (m == n) { found = true; break; }
            }
            if (!found) nodes.push_back(n);
        }
        return nodes;
    }

    size_t size() const {
        return adjList.size();
    }
};

#endif