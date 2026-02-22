#ifndef RANKER_H
#define RANKER_H

#include "Data_Structures/graph.h"
#include "Indexer/inverted_index.h"
#include "Data_Structures/hashmap.h"
#include "Data_Structures/heap.h"     
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cctype>     
#include <functional> 

class Ranker {
private:
    static long long computeTotalDocLength(const InvertedIndex& index) {
        long long sum = 0;
        auto urls = index.getAllDocuments();
        for (const auto& url : urls) {
            sum += index.getDocLength(url);
        }
        return sum;
    }

public:
    static HashMap<double> computePageRank(const Graph& graph,
                                           int iterations = 30,
                                           double damping = 0.85) {
        auto nodes = graph.getAllNodes();
        size_t N = nodes.size();
        if (N == 0) return HashMap<double>{};

        HashMap<double> ranks;
        double initialRank = 1.0 / static_cast<double>(N);
        for (const auto& node : nodes) {
            ranks[node] = initialRank;
        }

        HashMap<double> temp;

        for (int iter = 0; iter < iterations; ++iter) {
            temp.clear();
            double danglingMass = 0.0;

            for (const auto& node : nodes) {
                double rank = ranks.get(node);
                const auto& outgoing = graph.getNeighbors(node);

                if (outgoing.empty()) {
                    danglingMass += rank;
                } else {
                    double contrib = rank / static_cast<double>(outgoing.size());
                    for (const auto& target : outgoing) {
                        temp[target] += contrib;
                    }
                }
            }

            double leak = damping * danglingMass / static_cast<double>(N);
            double base = (1.0 - damping) / static_cast<double>(N);

            for (const auto& node : nodes) {
                double incoming = temp.get(node);
                temp[node] = base + leak + damping * incoming;
            }

            ranks = std::move(temp);
        }

        return ranks;
    }

    static double computeTFIDF(const InvertedIndex& index,
                               const std::string& term,
                               const std::string& docUrl) {
        int tf = index.getPostings(term).get(docUrl);
        if (tf == 0) return 0.0;

        size_t N = index.getDocCount();
        size_t df = index.getDocumentFrequency(term);
        if (df == 0) return 0.0;

        int docLen = index.getDocLength(docUrl);
        long long totalLen = computeTotalDocLength(index);
        double avgDocLen = N > 0 ? static_cast<double>(totalLen) / N : 1.0;

        double k1 = 1.2;
        double b = 0.75;
        double tfComponent = tf / (tf + k1 * (1 - b + b * docLen / avgDocLen));

        double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);

        return tfComponent * idf;
    }

    static double computeTitleBoost(const std::string& html, const std::vector<std::string>& terms) {
        size_t start = html.find("<title>");
        if (start == std::string::npos) return 0.0;
        start += 7;

        size_t end = html.find("</title>", start);
        if (end == std::string::npos) return 0.0;

        std::string title = html.substr(start, end - start);
        std::transform(title.begin(), title.end(), title.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        double bonus = 0.0;
        for (const auto& term : terms) {
            if (title.find(term) != std::string::npos) {
                bonus += 4.0;
            }
        }
        return std::min(bonus, 20.0);
    }

    static double computeFinalScore(double tfidfSum,
                                    double pageRank,
                                    double titleBonus) {
        return (tfidfSum * 1.5) +
               (pageRank * 1000.0) +
               (titleBonus * 1.0);
    }

    struct ScoredDoc {
        double score;
        std::string url;
        ScoredDoc(double s = 0, const std::string& u = "") : score(s), url(u) {}
    };

    // Helper comparator for the Heap to keep Top K (Min-Heap behavior)
    struct MinScoreComp {
        bool operator()(const ScoredDoc& a, const ScoredDoc& b) const {
            return a.score > b.score; // Root will be the SMALLEST of the top K
        }
    };

    // ====================== UPDATED: TOP-K RESULTS USING CUSTOM HEAP ======================
    static std::vector<ScoredDoc> getTopK(const std::vector<ScoredDoc>& candidates, int k) {
        if (candidates.empty() || k <= 0) return {};

        // custom MaxHeap with a Min-comparator to maintain the K largest elements
        MaxHeap<ScoredDoc, MinScoreComp> kHeap;

        for (const auto& doc : candidates) {
            if (kHeap.size() < static_cast<size_t>(k)) {
                kHeap.push(doc);
            } else if (doc.score > kHeap.top().score) {
                kHeap.pop();
                kHeap.push(doc);
            }
        }

        // Extract and sort results (highest score first)
        std::vector<ScoredDoc> result;
        while (!kHeap.empty()) {
            result.push_back(kHeap.pop());
        }
        std::reverse(result.begin(), result.end());
        return result;
    }
};

#endif