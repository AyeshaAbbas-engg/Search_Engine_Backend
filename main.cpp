#include "Data_Structures/thread_safe_queue.h"
#include "Data_Structures/hashset.h"
#include "Crawler/html_downloader.h"
#include "Crawler/link_parser.h"
#include "Data_Structures/trie.h"
#include "Data_Structures/graph.h"
#include "Indexer/inverted_index.h"
#include "Data_Structures/hashmap.h"
#include "Ranker/ranker.h"
#include "libs/crow_all.h"
#include "Scraper/scraper.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <set>

// ────────────────────────────────────────────────
//  HELPER FUNCTIONS 
// ────────────────────────────────────────────────

std::string getDomain(const std::string& url) {
    size_t pos = url.find("/", 8);
    return (pos == std::string::npos) ? url : url.substr(0, pos);
}

bool isHTMLPage(const std::string& url) {
    std::string lower = url;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find(".css")  == std::string::npos &&
           lower.find(".js")   == std::string::npos &&
           lower.find(".png")  == std::string::npos &&
           lower.find(".jpg")  == std::string::npos &&
           lower.find(".jpeg") == std::string::npos &&
           lower.find(".ico")  == std::string::npos &&
           lower.find(".svg")  == std::string::npos &&
           lower.find(".gif")  == std::string::npos &&
           lower.find(".pdf")  == std::string::npos;
}


// ────────────────────────────────────────────────
//  ROBOTS.TXT HANDLING
// ────────────────────────────────────────────────

HashMap<std::vector<std::string>> robotsRules;

void fetchRobots(const std::string& domain) {
    std::string robotsURL = domain + "/robots.txt";
    std::string content = HTMLDownloader::fetchHTML(robotsURL);

    std::vector<std::string> disallowed;
    if (!content.empty()) {
        std::cout << "Fetched robots.txt from " << domain << "\n";
        std::stringstream ss(content);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.find("Disallow:") == 0) {
                std::string path = line.substr(9);
                path.erase(0, path.find_first_not_of(" \t"));
                path.erase(path.find_last_not_of(" \t\r\n") + 1);
                if (!path.empty() && path != "/") {
                    disallowed.push_back(path);
                }
            }
        }
    } else {
        std::cout << "No robots.txt or failed to fetch — allowing all paths for " << domain << "\n";
    }
    robotsRules[domain] = disallowed;
}

bool allowedByRobots(const std::string& url) {
    std::string domain = getDomain(url);
    if (!robotsRules.contains(domain)) return true;

    std::string path = url.substr(domain.length());
    if (path.empty()) path = "/";

    const auto& rules = robotsRules.get(domain);
    for (const auto& rule : rules) {
        if (path.rfind(rule, 0) == 0) {
            std::cout << "Blocked by robots.txt: " << url << " (rule: " << rule << ")\n";
            return false;
        }
    }
    return true;
}

// ────────────────────────────────────────────────
//  MAIN
// ────────────────────────────────────────────────

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    ThreadSafeQueue urlQueue;
    HashSet visitedURLs;
    HashSet robotsFetchedDomains;

    Trie wordTrie;
    Graph linkGraph;
    InvertedIndex invIndex;
    HashMap<double> pageRanks;

    std::mutex ioMutex;
    std::atomic<bool> crawling{true};
    std::atomic<int> processedCount{0};

   // Change this in your main()
const std::string seedURL = "https://en.wikipedia.org/wiki/Computer_science";

// BEFORE pushing to the queue, sanitize it just in case
 std::string cleanSeed = seedURL;
 size_t s_pos = cleanSeed.find("https://", 8);
     if (s_pos != std::string::npos) cleanSeed = cleanSeed.substr(s_pos);

    urlQueue.push(cleanSeed);
    const int MAX_PAGES = 70;
    const int NUM_WORKERS = 2;

    bool loadedFromDisk = false;

    // Load existing index
    {
        std::ifstream in("Indexer/inverted_index.txt");
        if (in.good()) {
            std::cout << "Loading existing index from disk...\n";
            std::string line;
            while (std::getline(in, line)) {
                size_t sep = line.find('|');
                if (sep == std::string::npos) continue;
                std::string word = line.substr(0, sep);
                std::string urlsPart = line.substr(sep + 1);
                std::stringstream ss(urlsPart);
                std::string urlSeg;
                while (std::getline(ss, urlSeg, ';')) {
                    if (!urlSeg.empty()) {
                        invIndex.add(word, urlSeg);
                        wordTrie.insert(word);
                    }
                }
            }
            loadedFromDisk = true;
            std::cout << "Index loaded successfully.\n";
        }
    }

    std::ofstream visitedOut("Indexer/visited_pages.txt", std::ios::app);

    if (!loadedFromDisk) {

        urlQueue.push(seedURL);


        std::cout << "Starting multi-threaded crawl with " << NUM_WORKERS << " workers...\n";
        std::cout << "Crawling up to " << MAX_PAGES << " pages from " << seedURL << "\n\n";
     auto worker = [&]() {
     while (crawling) {
        std::string url;
        if (!urlQueue.try_pop(url)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        size_t secondProtocol = url.find("https://", 8);
        if (secondProtocol != std::string::npos) {
            url = url.substr(secondProtocol);
        }

        {
            std::lock_guard<std::mutex> lock(ioMutex);
            if (visitedURLs.contains(url) || processedCount >= MAX_PAGES) continue;
            std::cout << "[Worker] Processing: " << url << "\n";
        }

        std::string html = HTMLDownloader::fetchHTML(url);
        if (html.empty()) continue;

        // Wikipedia 404 check
        if (html.find("Wikipedia does not have an article with this exact name") != std::string::npos) {
            std::lock_guard<std::mutex> lock(ioMutex);
            std::cout << "[Skipping] Broken Wikipedia link: " << url << "\n";
            continue; 
        }

        {
            std::lock_guard<std::mutex> lock(ioMutex);

            if (visitedURLs.contains(url)) continue;
            if (processedCount >= MAX_PAGES) {
                crawling = false;
                return;
            }

            visitedURLs.insert(url);
            visitedOut << url << "\n";
            processedCount++;

            auto links = extractLinks(html, url);
            for (auto& link : links) {
                // Sanitize child links
                size_t secondChildProto = link.find("https://", 8);
                if (secondChildProto != std::string::npos) {
                    link = link.substr(secondChildProto);
                }

                if (!isHTMLPage(link)) continue;
                if (link.find("https://en.wikipedia.org/wiki/") != 0) continue;
                
                if (link.find("/wiki/Wikipedia:") != std::string::npos ||
                    link.find("/wiki/Help:")      != std::string::npos ||
                    link.find("/wiki/Talk:")      != std::string::npos ||
                    link.find("?")                != std::string::npos) continue;

                if (!visitedURLs.contains(link)) {
                    linkGraph.addEdge(url, link); 
                    urlQueue.push(link);
                }
            }

            // Inverted Indexing - uses the clean 'url'
            std::string cleanText = Scraper::extractText(html);
            std::vector<std::string> words = Scraper::tokenize(cleanText);

            for (const auto& w : words) {
                invIndex.add(w, url); 
                wordTrie.insert(w);   
            }
            
            std::cout << "[SUCCESS] Processed (" << processedCount << "/" << MAX_PAGES << "): " << url << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

        // Launch workers
        std::vector<std::thread> workers;
        for (int i = 0; i < NUM_WORKERS; ++i) {
            workers.emplace_back(worker);
        }

        // Clean status updates (every 3 seconds)
        while (crawling && processedCount < MAX_PAGES) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
       {
        std::lock_guard<std::mutex> lock(ioMutex);
        std::cout << "[Status] Processed: " << processedCount << " / " << MAX_PAGES << "\n";
       }
}
        crawling = false;

        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }

        std::cout << "\n=== CRAWLING COMPLETE ===\n";
        std::cout << "Successfully crawled and indexed " << processedCount << " pages.\n";

        std::cout << "Computing PageRank...\n";
        pageRanks = Ranker::computePageRank(linkGraph, 40, 0.85);

        std::cout << "Saving inverted index to disk...\n";
        {
            std::ofstream out("Indexer/inverted_index.txt");
            auto allWords = invIndex.getAllWords();
            for (const auto& word : allWords) {
                const auto& postings = invIndex.getPostings(word);
                if (postings.size() == 0) continue;
                out << word << "|";
                auto urls = postings.getKeys();
                for (size_t i = 0; i < urls.size(); ++i) {
                    out << urls[i];
                    if (i + 1 < urls.size()) out << ";";
                }
                out << "\n";
            }
        }
        std::cout << "Index saved!\n";
    }

    visitedOut.close();

    // ────────────────────────────────────────────────
    //  WEB SERVER
    // ────────────────────────────────────────────────

    std::cout << "\n" << std::string(60, '=') << "\n";
std::cout << " ATMX SEARCH ENGINE READY!\n";
std::cout << " Indexed " << invIndex.getDocCount() << " pages\n";
std::cout << " Visit: http://localhost:8080\n";
std::cout << std::string(60, '=') << "\n\n";

// Enable CORS globally via middleware
   // Enable CORS globally
crow::App<crow::CORSHandler> app;

// Configure CORS properly
auto& cors = app.get_middleware<crow::CORSHandler>();

cors.global()
    .origin("http://localhost:3000")                  // Allow only your Next.js dev server
    .methods("GET"_method, "OPTIONS"_method)         // GET for your requests, OPTIONS for preflight
    .headers("Content-Type", "Accept")                // Allow these common headers
    .max_age(3600);                                   // Cache preflight for 1 hour (optional but good)

    CROW_ROUTE(app, "/api/search")
    ([&invIndex, &wordTrie, &pageRanks](const crow::request& req) {
        crow::response res;

        // No need to handle OPTIONS manually anymore
        // No need to add Access-Control-Allow-Origin headers manually
        // The CORS middleware does all of this for you!

        auto q_param = req.url_params.get("q");
        auto suggest_param = req.url_params.get("suggest");

        crow::json::wvalue result;

        if (suggest_param) {
    std::string full_input = suggest_param;
    std::transform(full_input.begin(), full_input.end(), full_input.begin(), ::tolower);

    // 1. Find the last word in the input
    size_t lastSpace = full_input.find_last_of(" ");
    std::string prefix;
    std::string leadText = "";

    if (lastSpace == std::string::npos) {
        // Only one word typed so far
        prefix = full_input;
    } else {
        // Multiple words: "history tog" -> leadText is "history ", prefix is "tog"
        leadText = full_input.substr(0, lastSpace + 1);
        prefix = full_input.substr(lastSpace + 1);
    }

    crow::json::wvalue::list list;
    if (!prefix.empty()) {
        auto suggestions = wordTrie.getSuggestions(prefix, 10);
        for (const auto& s : suggestions) {
            // Prepend the leadText so the search bar shows the full phrase
            list.emplace_back(leadText + s);
        }
    }
    result["suggestions"] = std::move(list);
}
        else if (q_param) {
            std::string query = q_param;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);

            std::vector<std::string> terms;
            std::stringstream qss(query);
            std::string term;
            while (qss >> term) {
                terms.push_back(term);
            }

            HashSet candidates;
            for (const auto& t : terms) {
                auto postings = invIndex.getPostings(t);
                for (const auto& u : postings.getKeys()) {
                    candidates.insert(u);
                }
            }

            std::vector<Ranker::ScoredDoc> scored;
            for (const auto& url : candidates.getAll()) {
                double tfidfSum = 0.0;
                for (const auto& t : terms) {
                    tfidfSum += Ranker::computeTFIDF(invIndex, t, url);
                }
                double pr = pageRanks.get(url);
                double score = Ranker::computeFinalScore(tfidfSum, pr, 0.0);
                if (score > 0.001) {
                    scored.emplace_back(score, url);
                }
            }

            auto top = Ranker::getTopK(scored, 20);

            crow::json::wvalue::list results;
for (const auto& r : top) {
    crow::json::wvalue item;
    
    // --- START SANITIZER FIX ---
    std::string finalUrl = r.url;
    
    // Check if "https://" appears a second time (starting search after index 8)
    size_t secondProtocol = finalUrl.find("https://", 8);
    if (secondProtocol != std::string::npos) {
        // If doubled, keep only the second part
        finalUrl = finalUrl.substr(secondProtocol);
    }
    // --- END SANITIZER FIX ---

    item["url"] = finalUrl; // Use the cleaned URL
    item["score"] = r.score;

    // Generate title from the CLEANED URL
    size_t last = finalUrl.find_last_of('/');
    std::string title = (last != std::string::npos) ? finalUrl.substr(last + 1) : finalUrl;
    
    // Clean up title formatting
    size_t dotPos = title.find('.');
    if (dotPos != std::string::npos) title = title.substr(0, dotPos);
    
    std::replace(title.begin(), title.end(), '-', ' ');
    std::replace(title.begin(), title.end(), '_', ' ');
    if (title.empty()) title = "Untitled Page";

    item["title"] = title;
    results.emplace_back(std::move(item));
}
         result["results"] = std::move(results);
        }
        else {
            result = crow::json::wvalue();  // empty object
        }

        res.set_header("Content-Type", "application/json");
        res.body = result.dump();

        return res;
    });  // Note: removed .methods(...) — not needed when middleware handles OPTIONS

    // Optional: reduce log noise (hide INFO level like favicon requests)
    app.loglevel(crow::LogLevel::Warning);


// Start the server
app.port(8080).multithreaded().run();

curl_global_cleanup();
return 0;
}