#ifndef SSRAG_LAMBDA_FOLD_HPP
#define SSRAG_LAMBDA_FOLD_HPP

#include <ssrag/document_store.hpp>
#include <ssrag/fingerprint.hpp>
#include <catwhisper/index_sparse_iot.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace ssrag {

struct FoldParams {
    uint32_t fold_k = 50;            // top-k neighbors to fold onto
    float fold_threshold = 0.1f;     // minimum similarity to create fold edge
    int16_t max_fold_weight = 16000; // max int16 weight for fold dimensions
};

struct FoldResult {
    ChunkId chunk_id;
    uint32_t node_index;
    uint32_t new_effective_dim;
    uint32_t edges_created;  // number of bidirectional fold edges
};

class LambdaFoldEngine {
public:
    LambdaFoldEngine();
    explicit LambdaFoldEngine(FoldParams params);
    ~LambdaFoldEngine();
    LambdaFoldEngine(LambdaFoldEngine&&) noexcept;
    LambdaFoldEngine& operator=(LambdaFoldEngine&&) noexcept;

    // Initialize the IOT index.
    bool init();

    // Ingest a single chunk of text.
    // 1. Computes base fingerprint
    // 2. Searches index for related chunks
    // 3. Creates bidirectional fold dimensions
    // 4. Inserts into IOT index
    // Returns fold result with stats.
    FoldResult ingest(DocumentStore& store, std::string text, ChunkMeta meta);

    // Search the index with a query string.
    // Fingerprints the query (base dims only) and searches.
    struct SearchHit {
        ChunkId id;
        float distance;
    };
    std::vector<SearchHit> search(const DocumentStore& store,
                                  std::string_view query, uint32_t k = 10);

    // Current effective dimensionality.
    uint32_t effective_dim() const;

    // Estimated contrast ratio: sqrt(effective_dim).
    float estimated_contrast() const;

    // Persistence — saves/loads the IOT index.
    bool save_index(const std::filesystem::path& path) const;
    bool load_index(const std::filesystem::path& path);

private:
    FoldParams params_;
    std::unique_ptr<cw::IndexSparseIOT> index_;
    uint32_t effective_dim_ = BASE_DIMS;

    // Compute fold similarity between two base fingerprints.
    // Returns a normalized score in [0, 1].
    float fold_similarity(const cw::SparseVector& a, const cw::SparseVector& b) const;
};

} // namespace ssrag

#endif // SSRAG_LAMBDA_FOLD_HPP
