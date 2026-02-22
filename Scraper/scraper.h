#ifndef SCRAPER_H
#define SCRAPER_H

#include <string>
#include <vector>
#include <set>
#include <cctype>

class Scraper {
private:
    inline static const std::set<std::string> stopWords = {
        "a", "an", "the", "in", "on", "at", "to", "for", "of", "with",
        "is", "are", "was", "were", "be", "been", "have", "has", "had",
        "and", "or", "but", "not", "this", "that", "these", "those"
    };

public:
    // 1. Cleans the HTML
    static std::string extractText(const std::string& html) {
        std::string text;
        bool inTag = false;
        for (char c : html) {
            if (c == '<') {
                inTag = true;
                text += ' '; 
            }
            else if (c == '>') {
                inTag = false;
                text += ' '; 
            }
            else if (!inTag) {
                if (std::isspace(static_cast<unsigned char>(c))) {
                    text += ' '; 
                } else if (std::isprint(static_cast<unsigned char>(c))) {
                    text += std::tolower(static_cast<unsigned char>(c));
                } else {
                    text += ' '; 
                }
            }
        }
        return text;
    }

    // 2. Breaks text into clean words (using the stopWords above)
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> words;
        std::string word;
        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                word += std::tolower(static_cast<unsigned char>(c));
            } else {
                // Now uses the class-level stopWords
                if (!word.empty() && word.length() >= 2 && stopWords.find(word) == stopWords.end()) {
                    words.push_back(word);
                }
                word.clear();
            }
        }
        if (!word.empty() && word.length() >= 2 && stopWords.find(word) == stopWords.end()) {
            words.push_back(word);
        }
        return words;
    }
};

#endif