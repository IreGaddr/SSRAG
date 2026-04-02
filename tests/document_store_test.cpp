#include <ssrag/document_store.hpp>
#include <ssrag/fingerprint.hpp>
#include <gtest/gtest.h>
#include <filesystem>

using namespace ssrag;

TEST(DocumentStore, InsertAndRetrieve) {
    DocumentStore store;
    auto fp = fingerprint_text("test document");
    ChunkMeta meta{.source_path = "test.txt", .byte_offset = 0, .byte_length = 13};

    ChunkId id = store.insert("test document", meta, fp);
    EXPECT_GT(id, 0u);
    EXPECT_EQ(store.size(), 1u);

    const Chunk* chunk = store.get(id);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->text, "test document");
    EXPECT_EQ(chunk->meta.source_path, "test.txt");
    EXPECT_EQ(chunk->node_index, 0u);
}

TEST(DocumentStore, MultipleInserts) {
    DocumentStore store;
    for (int i = 0; i < 100; ++i) {
        auto fp = fingerprint_text("document " + std::to_string(i));
        ChunkMeta meta{.source_path = "test.txt"};
        store.insert("document " + std::to_string(i), meta, fp);
    }
    EXPECT_EQ(store.size(), 100u);
    EXPECT_EQ(store.next_node_index(), 100u);
}

TEST(DocumentStore, UpdateFingerprint) {
    DocumentStore store;
    auto fp = fingerprint_text("original text");
    ChunkMeta meta{.source_path = "test.txt"};
    ChunkId id = store.insert("original text", meta, fp);

    auto new_fp = fingerprint_text("modified fingerprint with extra terms");
    store.update_fingerprint(id, new_fp);

    const Chunk* chunk = store.get(id);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->fingerprint.nnz(), new_fp.nnz());
}

TEST(DocumentStore, Persistence) {
    auto tmp_path = std::filesystem::temp_directory_path() / "ssrag_test_store.bin";

    {
        DocumentStore store;
        for (int i = 0; i < 10; ++i) {
            auto fp = fingerprint_text("document " + std::to_string(i));
            ChunkMeta meta{.source_path = "file_" + std::to_string(i) + ".txt"};
            store.insert("document " + std::to_string(i), meta, fp);
        }
        ASSERT_TRUE(store.save(tmp_path));
    }

    {
        DocumentStore store;
        ASSERT_TRUE(store.load(tmp_path));
        EXPECT_EQ(store.size(), 10u);

        // Verify a chunk.
        const Chunk* chunk = store.get(1);
        ASSERT_NE(chunk, nullptr);
        EXPECT_EQ(chunk->text, "document 0");
    }

    std::filesystem::remove(tmp_path);
}

TEST(DocumentStore, DFTableUpdated) {
    DocumentStore store;
    auto fp = fingerprint_text("hello world");
    ChunkMeta meta{.source_path = "test.txt"};
    store.insert("hello world", meta, fp);

    EXPECT_FALSE(store.df_table().empty());
}
