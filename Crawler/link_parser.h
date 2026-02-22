#ifndef LINK_PARSER_H
#define LINK_PARSER_H

#include <regex>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

// 1. FIXED RESOLVE URL: This strictly prevents doubling URLs
std::string resolveURL(const std::string& base, const std::string& link) {
    if (link.empty()) return "";

    // Trim whitespace
    std::string cleanLink = link;
    cleanLink.erase(0, cleanLink.find_first_not_of(" \t\r\n"));
    cleanLink.erase(cleanLink.find_last_not_of(" \t\r\n") + 1);

    if (cleanLink.empty()) return "";

    // THE CRITICAL FIX: If it already looks like a URL, don't touch it!
    if (cleanLink.find("://") != std::string::npos || cleanLink.find("http") == 0) {
        return cleanLink;
    }

    // Handle protocol-relative (//en.wikipedia.org)
    if (cleanLink.find("//") == 0) {
        return "https:" + cleanLink;
    }

    // Handle absolute paths (/wiki/Page)
    if (cleanLink[0] == '/') {
        size_t schemeEnd = base.find("://");
        if (schemeEnd == std::string::npos) return "";
        size_t domainEnd = base.find('/', schemeEnd + 3);
        std::string domain = (domainEnd != std::string::npos) ? base.substr(0, domainEnd) : base;
        return domain + cleanLink;
    }

    // Handle relative paths
    size_t lastSlash = base.rfind('/');
    if (lastSlash != std::string::npos && lastSlash >= 8) {
        return base.substr(0, lastSlash + 1) + cleanLink;
    }

    return base + "/" + cleanLink;
}

// 2. FIXED EXTRACT LINKS: Accepts two arguments to match your main.cpp
std::vector<std::string> extractLinks(const std::string& html, const std::string& baseURL) {
    std::vector<std::string> links;

    std::regex linkRegex(
        R"(\<a[^>]+href\s*=\s*(["']?)([^"'\s>]+)\1[^>]*\>)",
        std::regex_constants::icase | std::regex_constants::optimize
    );

    auto begin = std::sregex_iterator(html.begin(), html.end(), linkRegex);
    auto end = std::sregex_iterator();

    for (auto i = begin; i != end; ++i) {
        std::string link = (*i)[2].str();

        // Filter junk
        if (link.empty() || link[0] == '#' || link.find("javascript:") == 0 || 
            link.find("mailto:") == 0) continue;

        // Remove fragments
        size_t frag = link.find('#');
        if (frag != std::string::npos) link.erase(frag);

        // Resolve path properly
        std::string absolute = resolveURL(baseURL, link);
        if (!absolute.empty()) {
            links.push_back(absolute);
        }
    }

    // Unique links only
    std::sort(links.begin(), links.end());
    links.erase(std::unique(links.begin(), links.end()), links.end());

    return links;
}

// 3. OVERLOAD: Helps if you call it with only 1 argument elsewhere
std::vector<std::string> extractLinks(const std::string& html) {
    return extractLinks(html, "https://en.wikipedia.org");
}

#endif