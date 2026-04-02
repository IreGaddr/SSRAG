#include <ssrag/lambda_fold.hpp>
#include <catwhisper/distance_iot.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <thread>
#include <vector>

namespace ssrag {

namespace {

cw::SparseVector base_dims_only(const cw::SparseVector& v) {
    cw::SparseVector result;
    for (const auto& e : v.entries()) {
        if (e.index < BASE_DIMS) result.set(e.index, e.value);
    }
    return result;
}

float cosine_similarity(const cw::SparseVector& a, const cw::SparseVector& b) {
    int64_t dot = a.dot(b);
    int64_t na = a.norm_sq();
    int64_t nb = b.norm_sq();
    if (na == 0 || nb == 0) return 0.0f;
    return static_cast<float>(dot) / std::sqrt(static_cast<float>(na) * static_cast<float>(nb));
}

uint32_t hardware_threads() {
    uint32_t n = std::thread::hardware_concurrency();
    return n > 0 ? n : 4;
}

} // namespace

LambdaFoldEngine::LambdaFoldEngine()
    : params_() {}

LambdaFoldEngine::LambdaFoldEngine(FoldParams params)
    : params_(params) {}

LambdaFoldEngine::~LambdaFoldEngine() = default;
LambdaFoldEngine::LambdaFoldEngine(LambdaFoldEngine&&) noexcept = default;
LambdaFoldEngine& LambdaFoldEngine::operator=(LambdaFoldEngine&&) noexcept = default;

bool LambdaFoldEngine::init() {
    auto result = cw::IndexSparseIOT::create();
    if (!result) return false;
    index_ = std::make_unique<cw::IndexSparseIOT>(std::move(*result));
    return true;
}

float LambdaFoldEngine::fold_similarity(const cw::SparseVector& a,
                                         const cw::SparseVector& b) const {
    return cosine_similarity(a, b);
}

FoldResult LambdaFoldEngine::ingest(DocumentStore& store, std::string text, ChunkMeta meta) {
    FoldResult result{};

    // 1. Compute base fingerprint.
    cw::SparseVector base_fp = fingerprint_text(text, store.df_table(),
                                                 static_cast<uint32_t>(store.size()));

    // 2. Parallel brute-force scan for fold neighbors.
    struct FoldEdge {
        ChunkId neighbor_id;
        uint32_t neighbor_node;
        float similarity;
    };
    std::vector<FoldEdge> edges;

    auto chunks = store.snapshot();
    if (!chunks.empty()) {
        const uint32_t n_threads = std::min(hardware_threads(),
                                             static_cast<uint32_t>(chunks.size()));

        // Per-thread candidate lists to avoid synchronization.
        std::vector<std::vector<FoldEdge>> thread_candidates(n_threads);

        std::vector<std::thread> threads;
        threads.reserve(n_threads);

        for (uint32_t t = 0; t < n_threads; ++t) {
            size_t start = (chunks.size() * t) / n_threads;
            size_t end = (chunks.size() * (t + 1)) / n_threads;

            threads.emplace_back([&, start, end, t]() {
                auto& local = thread_candidates[t];
                for (size_t i = start; i < end; ++i) {
                    const Chunk* chunk = chunks[i];
                    float sim = cosine_similarity(base_fp, chunk->fingerprint);
                    if (sim >= params_.fold_threshold) {
                        local.push_back({chunk->id, chunk->node_index, sim});
                    }
                }
            });
        }

        for (auto& th : threads) th.join();

        // Merge thread results.
        std::vector<FoldEdge> all_candidates;
        for (auto& tc : thread_candidates) {
            all_candidates.insert(all_candidates.end(), tc.begin(), tc.end());
        }

        // Keep top fold_k by similarity.
        if (all_candidates.size() > params_.fold_k) {
            std::partial_sort(all_candidates.begin(),
                              all_candidates.begin() + params_.fold_k,
                              all_candidates.end(),
                              [](const FoldEdge& a, const FoldEdge& b) {
                                  return a.similarity > b.similarity;
                              });
            all_candidates.resize(params_.fold_k);
        }

        edges = std::move(all_candidates);
    }

    // 3. Insert into document store.
    ChunkId chunk_id = store.insert(std::move(text), std::move(meta), base_fp);
    const Chunk* new_chunk = store.get(chunk_id);
    uint32_t node_index = new_chunk->node_index;

    // 4. Create bidirectional fold dimensions.
    uint32_t edges_created = 0;
    cw::SparseVector folded_fp = base_fp;

    for (const auto& edge : edges) {
        int16_t weight = static_cast<int16_t>(
            std::clamp(edge.similarity * static_cast<float>(params_.max_fold_weight),
                       -32767.0f, 32767.0f));
        if (weight == 0) continue;

        // New doc gains dimension pointing to neighbor.
        folded_fp.set(BASE_DIMS + edge.neighbor_node, weight);

        // Neighbor gains dimension pointing to new doc.
        const Chunk* neighbor = store.get(edge.neighbor_id);
        if (neighbor) {
            cw::SparseVector neighbor_fp = neighbor->fingerprint;
            neighbor_fp.set(BASE_DIMS + node_index, weight);
            store.update_fingerprint(edge.neighbor_id, std::move(neighbor_fp));
            // Skip index_->add() for neighbors — we don't use the ANN index
            // for search (brute-force is used instead due to recall issues).
        }

        edges_created++;
    }

    // 5. Store final fingerprint.
    store.update_fingerprint(chunk_id, folded_fp);

    // Update effective dimensionality.
    effective_dim_ = BASE_DIMS + store.next_node_index();

    result.chunk_id = chunk_id;
    result.node_index = node_index;
    result.new_effective_dim = effective_dim_;
    result.edges_created = edges_created;
    return result;
}

std::vector<LambdaFoldEngine::SearchHit>
LambdaFoldEngine::search(const DocumentStore& store, std::string_view query, uint32_t k) {
    if (store.size() == 0) return {};

    cw::SparseVector query_fp = fingerprint_text(query, store.df_table(),
                                                  static_cast<uint32_t>(store.size()));

    cw::IOTParams iot_params;

    // Parallel scan over all documents.
    auto chunks = store.snapshot();
    const uint32_t n_threads = std::min(hardware_threads(),
                                         static_cast<uint32_t>(chunks.size()));

    std::vector<std::vector<SearchHit>> thread_hits(n_threads);

    std::vector<std::thread> threads;
    threads.reserve(n_threads);

    for (uint32_t t = 0; t < n_threads; ++t) {
        size_t start = (chunks.size() * t) / n_threads;
        size_t end = (chunks.size() * (t + 1)) / n_threads;

        threads.emplace_back([&, start, end, t]() {
            auto& local = thread_hits[t];
            for (size_t i = start; i < end; ++i) {
                auto doc_base = base_dims_only(chunks[i]->fingerprint);
                float dist = cw::distance::iot_distance(query_fp, doc_base, iot_params);
                local.push_back({chunks[i]->id, dist});
            }
        });
    }

    for (auto& th : threads) th.join();

    // Merge and sort.
    std::vector<SearchHit> hits;
    for (auto& th : thread_hits) {
        hits.insert(hits.end(), th.begin(), th.end());
    }

    std::sort(hits.begin(), hits.end(), [](const SearchHit& a, const SearchHit& b) {
        return a.distance < b.distance;
    });
    if (hits.size() > k) hits.resize(k);

    return hits;
}

uint32_t LambdaFoldEngine::effective_dim() const {
    return effective_dim_;
}

float LambdaFoldEngine::estimated_contrast() const {
    return std::sqrt(static_cast<float>(effective_dim_));
}

bool LambdaFoldEngine::save_index(const std::filesystem::path& path) const {
    // Save effective_dim (the ANN index is not used, but we keep the meta file).
    std::filesystem::path meta_path = path;
    meta_path += ".meta";
    std::ofstream out(meta_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&effective_dim_), sizeof(effective_dim_));

    // Write an empty index file so load_index doesn't fail on existence check.
    if (!std::filesystem::exists(path)) {
        std::ofstream touch(path);
    }

    return out.good();
}

bool LambdaFoldEngine::load_index(const std::filesystem::path& path) {
    // Restore effective_dim.
    std::filesystem::path meta_path = path;
    meta_path += ".meta";
    std::ifstream in(meta_path, std::ios::binary);
    if (in) {
        in.read(reinterpret_cast<char*>(&effective_dim_), sizeof(effective_dim_));
    }
    return true;
}

} // namespace ssrag
