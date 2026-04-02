// Interactive query tool for the SSRAG protein index.
//
// Usage: ssrag-query [--data-dir <ssrag-data>] [--k <num>] [query...]
//
// Query modes:
//   text query:           ssrag-query "mitochondrial complex I"
//   protein neighborhood: ssrag-query "@EGFR"
//   multi-target overlap: ssrag-query "@EGFR @BRAF @MAPK1"

#include <ssrag/lambda_fold.hpp>
#include <ssrag/document_store.hpp>
#include <ssrag/fingerprint.hpp>
#include <catwhisper/distance_iot.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static std::string extract_protein_name(const ssrag::Chunk& chunk) {
    auto sp = chunk.text.find(' ');
    if (sp == std::string::npos) return chunk.text.substr(0, 20);
    return chunk.text.substr(0, sp);
}

static std::string extract_first_line(const ssrag::Chunk& chunk) {
    auto nl = chunk.text.find('\n');
    if (nl != std::string::npos) return chunk.text.substr(0, nl);
    return chunk.text.substr(0, 80);
}

static std::string extract_annotation(const ssrag::Chunk& chunk, size_t max_len = 300) {
    auto para_start = chunk.text.find("\n\n");
    if (para_start == std::string::npos) return {};
    para_start += 2;
    auto para_end = chunk.text.find("\n\n", para_start);
    if (para_end == std::string::npos) para_end = chunk.text.size();
    std::string annotation = chunk.text.substr(para_start,
        std::min(para_end - para_start, max_len));
    if (para_end - para_start > max_len) annotation += "...";
    return annotation;
}

static std::unordered_map<std::string, ssrag::ChunkId>
build_name_index(const ssrag::DocumentStore& store) {
    std::unordered_map<std::string, ssrag::ChunkId> index;
    store.for_each([&](const ssrag::Chunk& chunk) {
        std::string name = extract_protein_name(chunk);
        index[name] = chunk.id;
    });
    return index;
}

static cw::SparseVector base_dims_only(const cw::SparseVector& v) {
    cw::SparseVector result;
    for (const auto& e : v.entries()) {
        if (e.index < ssrag::BASE_DIMS) result.set(e.index, e.value);
    }
    return result;
}

struct Hit {
    ssrag::ChunkId id;
    float score;  // lower is better
};

static float fold_cosine(const cw::SparseVector& a, const cw::SparseVector& b) {
    int64_t dot = 0, na = 0, nb = 0;
    auto ae = a.entries();
    auto be = b.entries();
    size_t ai = 0, bi = 0;
    while (ai < ae.size() && ae[ai].index < ssrag::BASE_DIMS) ++ai;
    while (bi < be.size() && be[bi].index < ssrag::BASE_DIMS) ++bi;

    while (ai < ae.size() && bi < be.size()) {
        if (ae[ai].index < be[bi].index) {
            na += static_cast<int64_t>(ae[ai].value) * ae[ai].value;
            ++ai;
        } else if (ae[ai].index > be[bi].index) {
            nb += static_cast<int64_t>(be[bi].value) * be[bi].value;
            ++bi;
        } else {
            dot += static_cast<int64_t>(ae[ai].value) * be[bi].value;
            na += static_cast<int64_t>(ae[ai].value) * ae[ai].value;
            nb += static_cast<int64_t>(be[bi].value) * be[bi].value;
            ++ai; ++bi;
        }
    }
    while (ai < ae.size()) { na += static_cast<int64_t>(ae[ai].value) * ae[ai].value; ++ai; }
    while (bi < be.size()) { nb += static_cast<int64_t>(be[bi].value) * be[bi].value; ++bi; }

    if (na == 0 || nb == 0) return 0.0f;
    return static_cast<float>(dot) / std::sqrt(static_cast<float>(na) * static_cast<float>(nb));
}

// Parallel scan: applies scorer to each chunk, returns top-k hits by lowest score.
template <typename ScorerFn>
static std::vector<Hit> parallel_scan(const std::vector<const ssrag::Chunk*>& chunks,
                                       ScorerFn scorer, uint32_t k) {
    const uint32_t n_threads = std::min(
        std::max(1u, std::thread::hardware_concurrency()),
        static_cast<uint32_t>(chunks.size()));

    std::vector<std::vector<Hit>> thread_hits(n_threads);

    std::vector<std::thread> threads;
    threads.reserve(n_threads);

    for (uint32_t t = 0; t < n_threads; ++t) {
        size_t start = (chunks.size() * t) / n_threads;
        size_t end = (chunks.size() * (t + 1)) / n_threads;

        threads.emplace_back([&, start, end, t]() {
            auto& local = thread_hits[t];
            for (size_t i = start; i < end; ++i) {
                float s = scorer(*chunks[i]);
                if (!std::isnan(s)) {
                    local.push_back({chunks[i]->id, s});
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    // Merge.
    std::vector<Hit> hits;
    size_t total = 0;
    for (const auto& th : thread_hits) total += th.size();
    hits.reserve(total);
    for (auto& th : thread_hits) hits.insert(hits.end(), th.begin(), th.end());

    std::partial_sort(hits.begin(),
                      hits.begin() + std::min(static_cast<size_t>(k), hits.size()),
                      hits.end(),
                      [](const Hit& a, const Hit& b) { return a.score < b.score; });
    if (hits.size() > k) hits.resize(k);

    return hits;
}

int main(int argc, char* argv[]) {
    fs::path data_dir = "ssrag_data";
    uint32_t k = 10;
    std::string cli_query;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (arg == "--k" && i + 1 < argc) {
            k = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else {
            if (!cli_query.empty()) cli_query += ' ';
            cli_query += arg;
        }
    }

    ssrag::DocumentStore store;
    auto store_path = data_dir / "store.bin";
    auto index_path = data_dir / "index.bin";

    if (!fs::exists(store_path)) {
        std::cerr << "No index found in " << data_dir << "\n"
                  << "Run ssrag-ingest-string first.\n";
        return 1;
    }

    std::cerr << "Loading index from " << data_dir << "...\n";
    store.load(store_path);

    ssrag::LambdaFoldEngine engine;
    engine.load_index(index_path);

    uint32_t eff_dim = engine.effective_dim();
    std::cerr << "Loaded " << store.size() << " documents, "
              << "dim=" << eff_dim
              << ", contrast=" << engine.estimated_contrast() << ":1\n";

    auto name_index = build_name_index(store);
    std::cerr << "Built name index: " << name_index.size() << " proteins\n";

    // Take a snapshot once for all queries (index doesn't change at query time).
    auto chunks = store.snapshot();

    cw::IOTParams iot_params;

    // ---- Text query ----
    auto run_text_query = [&](const std::string& query) {
        auto query_fp = ssrag::fingerprint_text(query, store.df_table(),
                                                 static_cast<uint32_t>(store.size()));

        auto hits = parallel_scan(chunks, [&](const ssrag::Chunk& chunk) -> float {
            auto doc_base = base_dims_only(chunk.fingerprint);
            return cw::distance::iot_distance(query_fp, doc_base, iot_params);
        }, k);

        std::cout << "\n" << hits.size() << " results for: \"" << query << "\"\n";
        std::cout << std::string(60, '-') << "\n\n";

        for (size_t i = 0; i < hits.size(); ++i) {
            const ssrag::Chunk* chunk = store.get(hits[i].id);
            if (!chunk) continue;
            std::cout << "[" << (i + 1) << "] iot_dist=" << hits[i].score
                      << "  " << extract_first_line(*chunk) << "\n";
            std::string ann = extract_annotation(*chunk);
            if (!ann.empty()) std::cout << "    " << ann << "\n";
            std::cout << "\n";
        }
    };

    // ---- Protein neighborhood ----
    auto run_neighborhood_query = [&](const std::string& protein_name) {
        auto it = name_index.find(protein_name);
        if (it == name_index.end()) {
            std::cerr << "Protein not found: " << protein_name << "\n";
            std::string upper = protein_name;
            for (auto& c : upper) c = static_cast<char>(std::toupper(c));
            for (const auto& [name, id] : name_index) {
                std::string nu = name;
                for (auto& c : nu) c = static_cast<char>(std::toupper(c));
                if (nu == upper) {
                    it = name_index.find(name);
                    std::cerr << "  Found as: " << name << "\n";
                    break;
                }
            }
            if (it == name_index.end()) return std::vector<Hit>{};
        }

        const ssrag::Chunk* target = store.get(it->second);
        if (!target) return std::vector<Hit>{};

        std::cout << "\nNeighborhood of " << protein_name
                  << " (nnz=" << target->fingerprint.nnz()
                  << ", fold_dims=" << (target->fingerprint.nnz() -
                     base_dims_only(target->fingerprint).nnz()) << ")\n";
        std::cout << std::string(60, '-') << "\n\n";

        ssrag::ChunkId target_id = target->id;
        const auto& target_fp = target->fingerprint;

        auto hits = parallel_scan(chunks, [&](const ssrag::Chunk& chunk) -> float {
            if (chunk.id == target_id) return std::numeric_limits<float>::quiet_NaN();
            return -fold_cosine(target_fp, chunk.fingerprint);
        }, k);

        for (size_t i = 0; i < hits.size(); ++i) {
            const ssrag::Chunk* chunk = store.get(hits[i].id);
            if (!chunk) continue;
            std::cout << "[" << (i + 1) << "] fold_sim=" << -hits[i].score
                      << "  " << extract_first_line(*chunk) << "\n";
            std::string ann = extract_annotation(*chunk);
            if (!ann.empty()) std::cout << "    " << ann << "\n";
            std::cout << "\n";
        }

        return hits;
    };

    // ---- Multi-target overlap ----
    auto run_multi_target = [&](const std::vector<std::string>& targets) {
        std::cout << "\nMulti-target neighborhood overlap (" << targets.size() << " targets)\n";
        std::cout << std::string(60, '=') << "\n";

        uint32_t neighborhood_k = std::max(k * 10, 200u);

        std::vector<std::pair<std::string, std::unordered_map<ssrag::ChunkId, float>>> neighborhoods;

        for (const auto& target_name : targets) {
            auto it = name_index.find(target_name);
            if (it == name_index.end()) {
                std::cerr << "Target not found: " << target_name << "\n";
                continue;
            }
            const ssrag::Chunk* target = store.get(it->second);
            if (!target) continue;

            std::cout << "\nTarget: " << target_name
                      << " (nnz=" << target->fingerprint.nnz() << ")\n";

            ssrag::ChunkId target_id = target->id;
            const auto& target_fp = target->fingerprint;

            auto hits = parallel_scan(chunks, [&](const ssrag::Chunk& chunk) -> float {
                if (chunk.id == target_id) return std::numeric_limits<float>::quiet_NaN();
                return -fold_cosine(target_fp, chunk.fingerprint);
            }, neighborhood_k);

            std::unordered_map<ssrag::ChunkId, float> hood;
            for (const auto& h : hits) hood[h.id] = h.score;
            neighborhoods.push_back({target_name, std::move(hood)});

            std::cout << "  Top 5 neighbors:\n";
            for (size_t i = 0; i < std::min(size_t(5), hits.size()); ++i) {
                const ssrag::Chunk* chunk = store.get(hits[i].id);
                if (!chunk) continue;
                std::cout << "    " << extract_protein_name(*chunk)
                          << " (fold_sim=" << -hits[i].score << ")\n";
            }
        }

        if (neighborhoods.size() < 2) {
            std::cerr << "Need at least 2 valid targets for overlap analysis.\n";
            return;
        }

        struct OverlapHit {
            ssrag::ChunkId id;
            uint32_t n_targets;
            float avg_sim;
            std::vector<std::pair<std::string, float>> target_sims;
        };

        std::unordered_map<ssrag::ChunkId, OverlapHit> overlaps;
        for (const auto& [target_name, hood] : neighborhoods) {
            for (const auto& [cid, score] : hood) {
                auto& oh = overlaps[cid];
                oh.id = cid;
                oh.target_sims.push_back({target_name, -score});  // back to similarity
            }
        }

        std::vector<OverlapHit> overlap_results;
        for (auto& [cid, oh] : overlaps) {
            oh.n_targets = static_cast<uint32_t>(oh.target_sims.size());
            if (oh.n_targets < 2) continue;
            float sum = 0.0f;
            for (const auto& [_, s] : oh.target_sims) sum += s;
            oh.avg_sim = sum / static_cast<float>(oh.n_targets);
            overlap_results.push_back(std::move(oh));
        }

        std::sort(overlap_results.begin(), overlap_results.end(),
                  [](const OverlapHit& a, const OverlapHit& b) {
            if (a.n_targets != b.n_targets) return a.n_targets > b.n_targets;
            return a.avg_sim > b.avg_sim;
        });

        if (overlap_results.size() > k) overlap_results.resize(k);

        std::cout << "\n\nOVERLAPPING NEIGHBORHOODS (" << overlap_results.size()
                  << " proteins in 2+ target neighborhoods)\n";
        std::cout << std::string(60, '=') << "\n\n";

        for (size_t i = 0; i < overlap_results.size(); ++i) {
            const auto& oh = overlap_results[i];
            const ssrag::Chunk* chunk = store.get(oh.id);
            if (!chunk) continue;

            std::cout << "[" << (i + 1) << "] " << extract_protein_name(*chunk)
                      << "  (in " << oh.n_targets << "/" << neighborhoods.size()
                      << " neighborhoods, avg_sim=" << oh.avg_sim << ")\n";
            std::cout << "    " << extract_first_line(*chunk) << "\n";

            for (const auto& [tname, sim] : oh.target_sims) {
                std::cout << "      <- " << tname << " sim=" << sim << "\n";
            }

            std::string ann = extract_annotation(*chunk, 200);
            if (!ann.empty()) std::cout << "    " << ann << "\n";
            std::cout << "\n";
        }
    };

    // ---- Dispatch ----
    auto dispatch_query = [&](const std::string& query) {
        std::vector<std::string> targets;
        std::istringstream qs(query);
        std::string token;
        std::string text_query;

        while (qs >> token) {
            if (token[0] == '@' && token.size() > 1) {
                targets.push_back(token.substr(1));
            } else {
                if (!text_query.empty()) text_query += ' ';
                text_query += token;
            }
        }

        if (targets.size() >= 2) {
            run_multi_target(targets);
        } else if (targets.size() == 1) {
            run_neighborhood_query(targets[0]);
        } else {
            run_text_query(text_query);
        }
    };

    if (!cli_query.empty()) {
        dispatch_query(cli_query);
        return 0;
    }

    std::cerr << "\nQuery modes:\n"
              << "  text search:         mitochondrial complex I\n"
              << "  protein neighborhood: @EGFR\n"
              << "  multi-target overlap: @EGFR @BRAF @MAPK1\n"
              << "  set k:               k=20\n"
              << "  quit:                quit\n\n";

    std::string line;
    while (true) {
        std::cout << "ssrag> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

        if (line.substr(0, 2) == "k=") {
            k = static_cast<uint32_t>(std::stoul(line.substr(2)));
            std::cout << "Set k=" << k << "\n";
            continue;
        }

        dispatch_query(line);
    }

    return 0;
}
