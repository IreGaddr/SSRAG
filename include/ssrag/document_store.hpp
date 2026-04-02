#ifndef SSRAG_DOCUMENT_STORE_HPP
#define SSRAG_DOCUMENT_STORE_HPP

#include <catwhisper/sparse_vector.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ssrag {

using ChunkId = uint64_t;

struct ChunkMeta {
    std::string source_path;
    uint64_t byte_offset = 0;
    uint64_t byte_length = 0;
    uint64_t ingestion_ts = 0;  // unix millis
};

struct Chunk {
    ChunkId id = 0;
    std::string text;
    ChunkMeta meta;
    cw::SparseVector fingerprint;  // base + folded
    uint32_t node_index = 0;       // lambda fold dimension assignment
};

class DocumentStore {
public:
    DocumentStore() = default;

    // Insert a chunk, returns assigned ChunkId.
    ChunkId insert(std::string text, ChunkMeta meta, cw::SparseVector fingerprint);

    // Get chunk by ID.
    const Chunk* get(ChunkId id) const;

    // Update a chunk's fingerprint (for lambda fold updates).
    void update_fingerprint(ChunkId id, cw::SparseVector fingerprint);

    // Number of stored chunks.
    uint64_t size() const;

    // Next node index (= current size, used for lambda fold dimension assignment).
    uint32_t next_node_index() const;

    // Iterate all chunks (read-only).
    template <typename Fn>
    void for_each(Fn&& fn) const {
        std::shared_lock lock(mu_);
        for (const auto& [id, chunk] : chunks_) {
            fn(chunk);
        }
    }

    // Snapshot of all chunk pointers for parallel iteration (no lock held after return).
    std::vector<const Chunk*> snapshot() const {
        std::shared_lock lock(mu_);
        std::vector<const Chunk*> out;
        out.reserve(chunks_.size());
        for (const auto& [id, chunk] : chunks_) {
            out.push_back(&chunk);
        }
        return out;
    }

    // Document frequency table: dim index → count of chunks with non-zero value.
    const std::unordered_map<uint32_t, uint32_t>& df_table() const { return df_table_; }

    // Persistence.
    bool save(const std::filesystem::path& path) const;
    bool load(const std::filesystem::path& path);

private:
    mutable std::shared_mutex mu_;
    std::unordered_map<ChunkId, Chunk> chunks_;
    std::unordered_map<uint32_t, uint32_t> df_table_;  // base dim → doc count
    ChunkId next_id_ = 1;
};

} // namespace ssrag

#endif // SSRAG_DOCUMENT_STORE_HPP
