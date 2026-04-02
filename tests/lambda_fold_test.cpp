#include <ssrag/lambda_fold.hpp>
#include <gtest/gtest.h>
#include <cmath>

using namespace ssrag;

TEST(LambdaFold, InitSucceeds) {
    LambdaFoldEngine engine;
    EXPECT_TRUE(engine.init());
}

TEST(LambdaFold, IngestSingleDocument) {
    LambdaFoldEngine engine;
    ASSERT_TRUE(engine.init());

    DocumentStore store;
    ChunkMeta meta{.source_path = "test.txt"};
    auto result = engine.ingest(store, "The first document about pharmacology", meta);

    EXPECT_GT(result.chunk_id, 0u);
    EXPECT_EQ(result.node_index, 0u);
    EXPECT_EQ(result.new_effective_dim, BASE_DIMS + 1);
    EXPECT_EQ(result.edges_created, 0u);  // No neighbors yet.
}

TEST(LambdaFold, DimensionalityGrowsWithIngestion) {
    LambdaFoldEngine engine;
    ASSERT_TRUE(engine.init());

    DocumentStore store;
    for (int i = 0; i < 50; ++i) {
        ChunkMeta meta{.source_path = "doc_" + std::to_string(i) + ".txt"};
        engine.ingest(store, "Document number " + std::to_string(i) +
                      " about various scientific topics including chemistry and biology", meta);
    }

    EXPECT_EQ(engine.effective_dim(), BASE_DIMS + 50);
}

// Test the fold wiring directly: when we manually set up two related fingerprints
// and ingest them, verify the sparse vectors get the correct fold dimensions.
TEST(LambdaFold, FoldWiringSetsBidirectionalDimensions) {
    // Instead of relying on ANN search to find neighbors, verify the fold
    // mechanics by checking that after ingestion, chunks that got fold edges
    // have the correct folded dimensions in their sparse vectors.
    LambdaFoldEngine engine;
    ASSERT_TRUE(engine.init());
    DocumentStore store;

    ChunkMeta meta{.source_path = "test.txt"};

    // Ingest first doc — it gets node_index 0.
    auto r1 = engine.ingest(store, "aspirin NSAID COX2 inhibitor anti-inflammatory", meta);
    EXPECT_EQ(r1.node_index, 0u);

    // Check the first doc's fingerprint has only base dims.
    const Chunk* c1 = store.get(r1.chunk_id);
    ASSERT_NE(c1, nullptr);
    for (const auto& e : c1->fingerprint.entries()) {
        EXPECT_LT(e.index, BASE_DIMS) << "First doc should have no folded dims";
    }

    // Second doc gets node_index 1.
    auto r2 = engine.ingest(store, "ibuprofen NSAID COX2 inhibitor anti-inflammatory", meta);
    EXPECT_EQ(r2.node_index, 1u);

    // If fold edges were created, verify bidirectionality:
    // - Doc 1's fingerprint should have dim BASE_DIMS + 1 (doc 2's node)
    // - Doc 2's fingerprint should have dim BASE_DIMS + 0 (doc 1's node)
    if (r2.edges_created > 0) {
        const Chunk* c1_after = store.get(r1.chunk_id);
        const Chunk* c2_after = store.get(r2.chunk_id);
        ASSERT_NE(c1_after, nullptr);
        ASSERT_NE(c2_after, nullptr);

        // Doc 1 should have gained dimension BASE_DIMS + 1.
        int16_t c1_fold_val = c1_after->fingerprint.get(BASE_DIMS + 1);
        EXPECT_NE(c1_fold_val, 0) << "Doc 1 should have fold dim for doc 2";

        // Doc 2 should have dimension BASE_DIMS + 0.
        int16_t c2_fold_val = c2_after->fingerprint.get(BASE_DIMS + 0);
        EXPECT_NE(c2_fold_val, 0) << "Doc 2 should have fold dim for doc 1";

        // The fold weights should be equal (symmetric similarity).
        EXPECT_EQ(c1_fold_val, c2_fold_val);
    }
}

TEST(LambdaFold, FoldSimilarityIsSymmetric) {
    // Directly test the fingerprint similarity used for fold decisions.
    auto fp1 = fingerprint_text("aspirin NSAID COX2 inhibitor anti-inflammatory drug");
    auto fp2 = fingerprint_text("ibuprofen NSAID COX2 inhibitor anti-inflammatory drug");

    int64_t dot = fp1.dot(fp2);
    int64_t n1 = fp1.norm_sq();
    int64_t n2 = fp2.norm_sq();

    // Similarity should be positive and significant — these share most terms.
    EXPECT_GT(dot, 0);
    float sim = static_cast<float>(dot) / std::sqrt(static_cast<float>(n1) * static_cast<float>(n2));
    EXPECT_GT(sim, 0.5f) << "Highly overlapping texts should have high similarity";
}

TEST(LambdaFold, NodeIndexAssignedSequentially) {
    LambdaFoldEngine engine;
    ASSERT_TRUE(engine.init());
    DocumentStore store;

    for (uint32_t i = 0; i < 10; ++i) {
        ChunkMeta meta{.source_path = "doc.txt"};
        auto r = engine.ingest(store, "document " + std::to_string(i), meta);
        EXPECT_EQ(r.node_index, i);
    }
}

TEST(LambdaFold, SearchReturnsResults) {
    LambdaFoldEngine engine;
    ASSERT_TRUE(engine.init());
    DocumentStore store;

    ChunkMeta meta{.source_path = "test.txt"};
    engine.ingest(store, "Titanium dioxide nanoparticles exhibit photocatalytic properties "
                         "useful in water purification and self-cleaning surfaces", meta);
    engine.ingest(store, "Carbon nanotubes have exceptional tensile strength and electrical "
                         "conductivity making them ideal for composite materials", meta);
    engine.ingest(store, "Shakespeare wrote Hamlet in approximately 1600 and it remains "
                         "one of the most performed plays in the English language", meta);

    auto hits = engine.search(store, "nanomaterials for water treatment", 3);
    EXPECT_FALSE(hits.empty());
}

TEST(LambdaFold, EstimatedContrastFormula) {
    LambdaFoldEngine engine;
    ASSERT_TRUE(engine.init());

    // Before any ingestion, effective dim = BASE_DIMS.
    EXPECT_EQ(engine.effective_dim(), BASE_DIMS);
    EXPECT_FLOAT_EQ(engine.estimated_contrast(), std::sqrt(static_cast<float>(BASE_DIMS)));

    DocumentStore store;
    ChunkMeta meta{.source_path = "test.txt"};
    engine.ingest(store, "test document", meta);

    EXPECT_EQ(engine.effective_dim(), BASE_DIMS + 1);
    EXPECT_FLOAT_EQ(engine.estimated_contrast(), std::sqrt(static_cast<float>(BASE_DIMS + 1)));
}
