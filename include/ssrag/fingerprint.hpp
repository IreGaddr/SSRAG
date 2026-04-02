#ifndef SSRAG_FINGERPRINT_HPP
#define SSRAG_FINGERPRINT_HPP

#include <catwhisper/sparse_vector.hpp>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ssrag {

// Base fingerprint dimensionality — hashed TF-IDF features.
constexpr uint32_t BASE_DIMS = 256;

// FNV-1a hash of a string, reduced to a dimension index.
uint32_t fnv1a_hash(std::string_view s);

// Extract a base fingerprint from text.
// Tokenizes on whitespace/punctuation, computes TF-IDF weights hashed into
// BASE_DIMS buckets, outputs int16 sparse vector.
cw::SparseVector fingerprint_text(std::string_view text);

// Extract fingerprint from text given corpus-level document frequencies.
// df_table maps dimension index → number of documents containing that dim.
// total_docs is the corpus size.
cw::SparseVector fingerprint_text(std::string_view text,
                                  const std::unordered_map<uint32_t, uint32_t>& df_table,
                                  uint32_t total_docs);

// Tokenize text into lowercased terms.
std::vector<std::string_view> tokenize(std::string_view text);

} // namespace ssrag

#endif // SSRAG_FINGERPRINT_HPP
