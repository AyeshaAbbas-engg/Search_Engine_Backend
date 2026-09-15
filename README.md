# ATMX Search Engine — C++ Backend

An educational search engine backend exploring web crawling, inverted indexing, ranked retrieval, and autocomplete through custom C++ data structures. A Crow HTTP API exposes search and suggestion results for a separate frontend.

**Stack:** C++17, Crow (bundled header), libcurl, Boost, Docker.

**Status:** Student prototype. The repository includes a saved index for local demonstrations. Public deployment and frontend integration are not verified by this README.

## Features

- Queue-based, two-worker crawler seeded with Wikipedia's Computer Science article and filtered to selected English Wikipedia article URLs.
- HTML tag stripping, lowercase tokenization, and a small English stop-word filter.
- Inverted index mapping terms to documents, with occurrence counts maintained during fresh crawling.
- Multi-keyword retrieval: candidates may match **any** query term; scores combine contributions from the matching terms.
- BM25-style lexical scoring, a PageRank implementation, and heap-based selection of up to 20 results.
- Trie autocomplete returning up to 10 suggestions, including completion of the final word in a multiword input.
- File-based index loading and saving.

The uploaded snapshot contains 35,986 index lines and 70 distinct URL strings in its postings. These describe the saved data, not a measured crawl result or quality benchmark. The current fresh-crawl limit in source is 25 pages.

## Architecture

There are two startup paths. If `Indexer/inverted_index.txt` can be opened, the server reconstructs the index and trie from it and skips crawling. Otherwise, it attempts a fresh crawl before starting the API.

| Component | Role | Source |
|---|---|---|
| Application | Startup, crawl workers, persistence, API routes | `main.cpp` |
| Downloader and parser | Fetch HTML and extract links | `Crawler/` |
| Scraper | Extract text and tokenize it | `Scraper/scraper.h` |
| Inverted index | Store term-to-document postings and document lengths | `Indexer/inverted_index.h` |
| Ranker | Lexical scoring, PageRank, and top-k selection | `Ranker/ranker.h` |
| Trie and sorter | Prefix traversal and frequency-based suggestion sorting | `Data_Structures/trie.h`, `Sorter/sorter.h` |
| Custom structures | Hash map, hash set, linked list, queue, graph, heap | `Data_Structures/` |
| HTTP framework | Bundled Crow implementation | `libs/crow_all.h` |

The crawler uses a shared FIFO queue; parallel downloads mean completion order is not strict breadth-first order. Recursive depth-first traversal is used inside the autocomplete trie. The code does not expose a DFS web-crawling mode.

## Local setup

Run commands from the repository root so relative index paths resolve correctly. Keep the bundled index in place for an initial demonstration; see the fresh-crawl limitations below.

### Build from source on Ubuntu

The dependencies below follow the existing Dockerfile:

```bash
sudo apt-get update
sudo apt-get install -y g++ libcurl4-openssl-dev libboost-all-dev libssl-dev zlib1g-dev

g++ -std=c++17 -O2 -I. -o server main.cpp \
  -lcurl -lpthread -lboost_system -lssl -lcrypto

./server
```

The API defaults to port 8080. To choose another port:

```bash
PORT=8081 ./server
```

The application binds to `0.0.0.0`. There is no homepage route: use `/api/search` to interact with this backend.

### Existing Docker configuration

With Docker installed, the repository provides this build path:

```bash
docker build -t atmx-backend .
docker run --rm -p 127.0.0.1:8080:8080 atmx-backend
```

This maps the container API to the local machine only. The Dockerfile compiles the source into `server`; the included `atmx` executable is not needed for this build. Container filesystem changes are not retained after removal unless persistence is configured.

**Verification note:** These commands are derived from the source and existing Dockerfile. A review-environment compilation attempt stopped because Boost headers were unavailable. A successful build, Docker run, and end-to-end frontend session have not been verified in that review.

## API

| Method | Request | Response fields |
|---|---|---|
| GET | `/api/search?q=computer%20science` | `results`: array of objects containing `url`, `score`, and `title` |
| GET | `/api/search?suggest=comp` | `suggestions`: array of strings |
| GET | `/api/search?suggest=computer%20sci` | Suggestions completing the final word and preserving the preceding words in lowercase |

Try the API after the server reports readiness:

```bash
curl --get 'http://localhost:8080/api/search' \
  --data-urlencode 'q=computer science'

curl --get 'http://localhost:8080/api/search' \
  --data-urlencode 'suggest=comp'
```

Search terms are lowercased and split on whitespace. Search results are ranked in descending score order, limited to 20, and filtered by a score threshold of 0.001. Titles are derived from URL paths rather than fetched page titles. If both parameters are supplied, `suggest` takes precedence. Query normalization does not currently use the same tokenizer as document indexing, so punctuation can affect matches.

## Ranking and persistence

The function named `computeTFIDF` implements a BM25-style term contribution using document length normalization (`k1=1.2`, `b=0.75`) and a smoothed inverse-document-frequency term. It is not a conventional TF-IDF implementation.

The final scoring function combines lexical score, PageRank, and title bonus. In the active search route, title bonus is always zero. PageRank is computed after a fresh crawl with 40 iterations and damping 0.85; it is not restored when loading the saved index.

The saved index format is:

```text
term|url1;url2;url3
```

It preserves term-to-URL membership but does not preserve term occurrence counts, original document lengths, the link graph, or PageRank. Reloading adds each stored posting once. Consequently, ranking and autocomplete frequencies can differ between a fresh crawl and a restart. Full ranking-state persistence is a planned improvement, not a completed feature.

## Demonstrating the project

1. Start the backend using the bundled index.
2. Show one search request and its returned URLs, titles, and scores.
3. Show autocomplete for a prefix and for the final word in a phrase.
4. Connect the separate frontend to the API and record a short search walkthrough after verifying integration.
5. Link the frontend repository and recorded demo here once available.

The frontend source is maintained separately and was not included in this backend review. Its startup command, configuration variable names, and repository URL should be documented after inspecting that source.

## Known limitations and next steps

- **Crawl termination:** Workers keep polling when the queue is empty. If fewer than 25 pages succeed and no work remains, startup can wait indefinitely. Add explicit queue-exhaustion and in-flight-work termination logic.
- **Robots handling:** Helper functions exist but are not invoked by the crawl workers. The parser also ignores `Disallow: /` and does not implement user-agent rule grouping. Correct and integrate robots handling before fresh public-web crawling.
- **TLS verification:** The downloader currently disables peer and hostname verification. Restore certificate verification before using fresh crawling beyond controlled development.
- **Ranking persistence:** Save and reload term frequencies, document lengths, and graph/rank information to preserve results across restarts.
- **Resource usage:** Each custom hash map contains 10,007 linked-list buckets, including every per-term postings map. Nested maps and copied postings can consume substantial memory and query time. Measure these before making scalability or speed claims.
- **HTML parsing:** Tag stripping is a lightweight character scan rather than a full HTML parser; script/style content and entities need more robust handling.
- **Deployment configuration:** CORS currently permits all origins; the Dockerfile grants broad write permissions to `Indexer/` and runs under its default user. Tighten these settings before public hosting and add request/resource limits.
- **Evaluation:** The archive does not include a benchmark suite or automated test suite. Retrieval-quality, latency, and memory claims require measurement.

## Scope

This project demonstrates data structures and information-retrieval concepts in a small search system. It does not claim web-scale coverage, production readiness, or experimentally established ranking quality. A local demonstration and reproducible source build are the immediate presentation goals.
