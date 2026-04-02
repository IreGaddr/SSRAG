#include <ssrag/fingerprint.hpp>
#include <gtest/gtest.h>

using namespace ssrag;

TEST(Fingerprint, TokenizeBasic) {
    auto tokens = tokenize("Hello, world! This is a test.");
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0], "Hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST(Fingerprint, TokenizeEmpty) {
    auto tokens = tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST(Fingerprint, FNV1aConsistent) {
    EXPECT_EQ(fnv1a_hash("hello"), fnv1a_hash("hello"));
    EXPECT_NE(fnv1a_hash("hello"), fnv1a_hash("world"));
}

TEST(Fingerprint, FingerprintNonEmpty) {
    auto fp = fingerprint_text("The quick brown fox jumps over the lazy dog.");
    EXPECT_GT(fp.nnz(), 0u);
    EXPECT_LE(fp.effective_dim(), BASE_DIMS);
}

TEST(Fingerprint, FingerprintEmpty) {
    auto fp = fingerprint_text("");
    EXPECT_EQ(fp.nnz(), 0u);
}

TEST(Fingerprint, SimilarTextsSimilarFingerprints) {
    auto fp1 = fingerprint_text("aspirin is a COX-2 inhibitor used as an anti-inflammatory drug");
    auto fp2 = fingerprint_text("ibuprofen is a COX-2 inhibitor used as an anti-inflammatory drug");
    auto fp3 = fingerprint_text("quantum chromodynamics describes the strong nuclear force");

    // fp1 and fp2 should be more similar than fp1 and fp3.
    int64_t dot12 = fp1.dot(fp2);
    int64_t dot13 = fp1.dot(fp3);
    EXPECT_GT(dot12, dot13);
}

TEST(Fingerprint, FingerprintWithIDF) {
    std::unordered_map<uint32_t, uint32_t> df_table;
    // Simulate some terms being very common.
    for (uint32_t i = 0; i < BASE_DIMS; ++i) df_table[i] = 100;

    auto fp = fingerprint_text("The quick brown fox", df_table, 1000);
    EXPECT_GT(fp.nnz(), 0u);
}

TEST(Fingerprint, AllDimsWithinBase) {
    auto fp = fingerprint_text("A somewhat longer text with many different words to ensure "
                               "we exercise the hashing vectorizer across multiple buckets "
                               "and verify that all dimensions stay within BASE_DIMS range.");
    for (const auto& e : fp.entries()) {
        EXPECT_LT(e.index, BASE_DIMS);
    }
}
