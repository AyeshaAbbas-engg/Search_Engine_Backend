#ifndef HTML_DOWNLOADER_H
#define HTML_DOWNLOADER_H

#include <string>
#include <curl/curl.h>
#include <iostream>

class HTMLDownloader {
private:
    //  appends received data to string buffer
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::string* buffer = static_cast<std::string*>(userp);
        buffer->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

public:
    static std::string fetchHTML(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[CURL ERROR] Failed to initialize curl for: " << url << "\n";
            return "";
        }

        std::string buffer;

        // Realistic browser User-Agent (Chrome on Windows - updated for late 2025)
        const char* user_agent = 
       "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
       "AppleWebKit/537.36 (KHTML, like Gecko) "
       "Chrome/143.0.0.0 Safari/537.36";
        // Additional headers to mimic a real browser
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8");
        headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
        headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        headers = curl_slist_append(headers, "Sec-Fetch-Dest: document");
        headers = curl_slist_append(headers, "Sec-Fetch-Mode: navigate");
        headers = curl_slist_append(headers, "Sec-Fetch-Site: none");
        headers = curl_slist_append(headers, "Sec-Fetch-User: ?1");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);          // Follow redirects
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br"); // Auto decompression
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 12L);                // Increased slightly
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);                // Thread-safe

        // WARNING: Only for testing/dev! Re-enable in production!
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);


        CURLcode res = curl_easy_perform(curl);

        // Clean up headers
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            std::cerr << "[CURL ERROR] " << curl_easy_strerror(res) << " for: " << url << "\n";
            curl_easy_cleanup(curl);
            return "";
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);

        if (http_code != 200) {
            std::cerr << "[HTTP STATUS] " << http_code << " for: " << url << "\n";
            if (http_code == 403 || http_code == 429 || http_code == 503) {
                std::cerr << "[BLOCKED?] Possible anti-bot detection (403/429/503)\n";
            }
        }

        if (buffer.empty()) {
            std::cerr << "[EMPTY RESPONSE] No data received from: " << url << "\n";
            return "";
        }

        std::cout << "[DOWNLOAD SUCCESS] " << buffer.length() << " bytes from: " << url << "\n";
        return std::move(buffer);
    }
};

#endif 