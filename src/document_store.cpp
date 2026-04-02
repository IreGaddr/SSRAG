#include <ssrag/document_store.hpp>
#include <ssrag/fingerprint.hpp>
#include <chrono>
#include <fstream>
#include <mutex>

namespace ssrag {

ChunkId DocumentStore::insert(std::string text, ChunkMeta meta, cw::SparseVector fingerprint) {
    std::unique_lock lock(mu_);

    ChunkId id = next_id_++;
    uint32_t node_idx = static_cast<uint32_t>(chunks_.size());

    // Update document frequency table for base dims.
    for (const auto& e : fingerprint.entries()) {
        if (e.index < BASE_DIMS) {
            df_table_[e.index]++;
        }
    }

    if (meta.ingestion_ts == 0) {
        meta.ingestion_ts = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }

    Chunk chunk;
    chunk.id = id;
    chunk.text = std::move(text);
    chunk.meta = std::move(meta);
    chunk.fingerprint = std::move(fingerprint);
    chunk.node_index = node_idx;

    chunks_.emplace(id, std::move(chunk));
    return id;
}

const Chunk* DocumentStore::get(ChunkId id) const {
    std::shared_lock lock(mu_);
    auto it = chunks_.find(id);
    return it != chunks_.end() ? &it->second : nullptr;
}

void DocumentStore::update_fingerprint(ChunkId id, cw::SparseVector fingerprint) {
    std::unique_lock lock(mu_);
    auto it = chunks_.find(id);
    if (it != chunks_.end()) {
        it->second.fingerprint = std::move(fingerprint);
    }
}

uint64_t DocumentStore::size() const {
    std::shared_lock lock(mu_);
    return chunks_.size();
}

uint32_t DocumentStore::next_node_index() const {
    std::shared_lock lock(mu_);
    return static_cast<uint32_t>(chunks_.size());
}

bool DocumentStore::save(const std::filesystem::path& path) const {
    std::shared_lock lock(mu_);
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    // Format: [chunk_count][chunks...]
    uint64_t count = chunks_.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    out.write(reinterpret_cast<const char*>(&next_id_), sizeof(next_id_));

    for (const auto& [id, chunk] : chunks_) {
        // ID + node_index
        out.write(reinterpret_cast<const char*>(&chunk.id), sizeof(chunk.id));
        out.write(reinterpret_cast<const char*>(&chunk.node_index), sizeof(chunk.node_index));

        // Text
        uint64_t text_len = chunk.text.size();
        out.write(reinterpret_cast<const char*>(&text_len), sizeof(text_len));
        out.write(chunk.text.data(), static_cast<std::streamsize>(text_len));

        // Meta
        uint64_t path_len = chunk.meta.source_path.size();
        out.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
        out.write(chunk.meta.source_path.data(), static_cast<std::streamsize>(path_len));
        out.write(reinterpret_cast<const char*>(&chunk.meta.byte_offset), sizeof(chunk.meta.byte_offset));
        out.write(reinterpret_cast<const char*>(&chunk.meta.byte_length), sizeof(chunk.meta.byte_length));
        out.write(reinterpret_cast<const char*>(&chunk.meta.ingestion_ts), sizeof(chunk.meta.ingestion_ts));

        // Fingerprint (sparse entries)
        uint32_t nnz = chunk.fingerprint.nnz();
        out.write(reinterpret_cast<const char*>(&nnz), sizeof(nnz));
        for (const auto& e : chunk.fingerprint.entries()) {
            out.write(reinterpret_cast<const char*>(&e.index), sizeof(e.index));
            out.write(reinterpret_cast<const char*>(&e.value), sizeof(e.value));
        }
    }

    // DF table
    uint64_t df_count = df_table_.size();
    out.write(reinterpret_cast<const char*>(&df_count), sizeof(df_count));
    for (const auto& [dim, count_val] : df_table_) {
        out.write(reinterpret_cast<const char*>(&dim), sizeof(dim));
        out.write(reinterpret_cast<const char*>(&count_val), sizeof(count_val));
    }

    return out.good();
}

bool DocumentStore::load(const std::filesystem::path& path) {
    std::unique_lock lock(mu_);
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    chunks_.clear();
    df_table_.clear();

    uint64_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    in.read(reinterpret_cast<char*>(&next_id_), sizeof(next_id_));

    for (uint64_t i = 0; i < count; ++i) {
        Chunk chunk;
        in.read(reinterpret_cast<char*>(&chunk.id), sizeof(chunk.id));
        in.read(reinterpret_cast<char*>(&chunk.node_index), sizeof(chunk.node_index));

        uint64_t text_len;
        in.read(reinterpret_cast<char*>(&text_len), sizeof(text_len));
        chunk.text.resize(text_len);
        in.read(chunk.text.data(), static_cast<std::streamsize>(text_len));

        uint64_t path_len;
        in.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));
        chunk.meta.source_path.resize(path_len);
        in.read(chunk.meta.source_path.data(), static_cast<std::streamsize>(path_len));
        in.read(reinterpret_cast<char*>(&chunk.meta.byte_offset), sizeof(chunk.meta.byte_offset));
        in.read(reinterpret_cast<char*>(&chunk.meta.byte_length), sizeof(chunk.meta.byte_length));
        in.read(reinterpret_cast<char*>(&chunk.meta.ingestion_ts), sizeof(chunk.meta.ingestion_ts));

        uint32_t nnz;
        in.read(reinterpret_cast<char*>(&nnz), sizeof(nnz));
        std::vector<cw::SparseEntry> entries(nnz);
        for (uint32_t j = 0; j < nnz; ++j) {
            in.read(reinterpret_cast<char*>(&entries[j].index), sizeof(entries[j].index));
            in.read(reinterpret_cast<char*>(&entries[j].value), sizeof(entries[j].value));
        }
        chunk.fingerprint = cw::SparseVector(std::move(entries));

        chunks_.emplace(chunk.id, std::move(chunk));
    }

    uint64_t df_count;
    in.read(reinterpret_cast<char*>(&df_count), sizeof(df_count));
    for (uint64_t i = 0; i < df_count; ++i) {
        uint32_t dim, cnt;
        in.read(reinterpret_cast<char*>(&dim), sizeof(dim));
        in.read(reinterpret_cast<char*>(&cnt), sizeof(cnt));
        df_table_[dim] = cnt;
    }

    return in.good();
}

} // namespace ssrag
