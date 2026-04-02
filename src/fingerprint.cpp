#include <ssrag/fingerprint.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <unordered_map>

namespace ssrag {

uint32_t fnv1a_hash(std::string_view s) {
    uint32_t hash = 2166136261u;
    for (char c : s) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

std::vector<std::string_view> tokenize(std::string_view text) {
    std::vector<std::string_view> tokens;
    size_t i = 0;
    while (i < text.size()) {
        // Skip non-alphanumeric.
        while (i < text.size() && !std::isalnum(static_cast<unsigned char>(text[i]))) ++i;
        size_t start = i;
        while (i < text.size() && std::isalnum(static_cast<unsigned char>(text[i]))) ++i;
        if (i > start) {
            tokens.push_back(text.substr(start, i - start));
        }
    }
    return tokens;
}

// Lowercase a token into a static thread-local buffer for hashing.
static std::string to_lower(std::string_view sv) {
    std::string s(sv);
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

cw::SparseVector fingerprint_text(std::string_view text) {
    auto tokens = tokenize(text);
    if (tokens.empty()) return {};

    // Term frequency: count occurrences per hashed bucket.
    std::unordered_map<uint32_t, double> tf;
    for (auto tok : tokens) {
        uint32_t dim = fnv1a_hash(to_lower(tok)) % BASE_DIMS;
        tf[dim] += 1.0;
    }

    // Normalize TF by total tokens, scale to int16.
    double n = static_cast<double>(tokens.size());
    std::vector<cw::SparseEntry> entries;
    entries.reserve(tf.size());
    for (auto& [dim, count] : tf) {
        double weight = (count / n);  // TF without IDF (no corpus stats)
        int16_t val = static_cast<int16_t>(std::clamp(weight * 32767.0, -32767.0, 32767.0));
        if (val != 0) {
            entries.push_back({dim, val});
        }
    }

    return cw::SparseVector(std::move(entries));
}

cw::SparseVector fingerprint_text(std::string_view text,
                                  const std::unordered_map<uint32_t, uint32_t>& df_table,
                                  uint32_t total_docs) {
    auto tokens = tokenize(text);
    if (tokens.empty()) return {};

    // Term frequency per hashed bucket.
    std::unordered_map<uint32_t, double> tf;
    for (auto tok : tokens) {
        uint32_t dim = fnv1a_hash(to_lower(tok)) % BASE_DIMS;
        tf[dim] += 1.0;
    }

    double n = static_cast<double>(tokens.size());
    double N = std::max(static_cast<double>(total_docs), 1.0);

    std::vector<cw::SparseEntry> entries;
    entries.reserve(tf.size());
    for (auto& [dim, count] : tf) {
        // TF-IDF: tf * log(N / (df + 1))
        double df = 1.0;  // default: 1 (smoothing)
        auto it = df_table.find(dim);
        if (it != df_table.end()) df = static_cast<double>(it->second) + 1.0;
        double tfidf = (count / n) * std::log(N / df);
        int16_t val = static_cast<int16_t>(std::clamp(tfidf * 16383.0, -32767.0, 32767.0));
        if (val != 0) {
            entries.push_back({dim, val});
        }
    }

    return cw::SparseVector(std::move(entries));
}

} // namespace ssrag
