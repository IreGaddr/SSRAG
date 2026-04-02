# SSRAG: Self-Sharpening RAG

## One Sentence

A document retrieval index where adding more documents makes search better, not worse, because each document creates a new dimension in the search space and the IOT metric reverses the curse of dimensionality.

## The Problem

Every RAG system uses cosine similarity or L2 distance on embeddings. These are Euclidean metrics on hyperspheres. Beyer et al. (1999) proved that on hyperspheres, as dimensionality grows, all points become equidistant:

```
(D_max - D_min) / D_min → 0   as d → ∞
```

This means: the bigger your corpus, the more features you need to discriminate, the WORSE your search gets. Every RAG system fights this — dimensionality reduction, chunking strategies, re-ranking, hybrid search — all because the underlying distance function breaks at scale.

## The Solution

Replace cosine/L2 with the Involuted Oblate Toroid (IOT) metric. On the IOT manifold, the curse reverses:

```
Contrast C(d) ≈ √d   (grows with dimension)
```

At 1,000 dimensions: contrast ~31:1. At 1,000,000 dimensions: contrast ~1000:1. Search gets MORE discriminative as dimensionality grows.

## The Mechanism: Lambda Fold

Every document ingested into the index becomes a dimension in the search space:

1. Document D enters the index with a base fingerprint (n-gram hashes, TF-IDF, whatever)
2. Document D is assigned dimension index `base_dims + D_index`
3. Every OTHER document that shares content overlap with D gets a non-zero value in dimension `base_dims + D_index`, proportional to overlap strength
4. D gets non-zero values in the dimensions of documents it overlaps with

This is the "lambda fold" — the index's own contents become axes of the similarity space. Two documents that share no surface-level text features but both relate to the same third document will be nearby in the folded dimensions.

At 100,000 documents the effective search space is 100,000+ dimensions. Cosine similarity at 100,000 dims returns random noise. IOT at 100,000 dims has contrast ~316:1.

## The Self-Sharpening Property

"Self-sharpening" means: ingesting document N+1 doesn't just add one more searchable item — it creates dimension N+1, which makes ALL existing searches across ALL N documents more precise. The index literally gets smarter by getting bigger.

No retraining. No embedding model updates. No re-indexing. The dimensionality growth IS the improvement.

## What Needs To Be Built (from scratch)

1. IOT distance function for sparse integer vectors
2. Sparse vector ANN index using IOT distance with path signature pre-filter
3. Document fingerprint extraction (text → int16 sparse vector)
4. Lambda fold wiring (document co-occurrence → folded dimensions)
5. MCP server exposing `search` and `ingest` tools
6. Persistence (snapshot/restore the index)

## Reference Material

The math behind IOT cursebreaking is proven in this repo:
- `docs/iot_volume_expansion_proof.md` — full proof with critical scaling law
- `docs/cursebreaking/` — LaTeX paper with theorems

Key results to carry forward:
- Critical R:r scaling: `r/R = ln(d)/(2d)` separates curse from anti-curse
- Dimension-adaptive formula: `R(d) = d/(c*ln(d))*r` with c=2 stays at 2x critical
- Integer-native arithmetic: fingerprints are int16 by construction, not quantized floats
- Path signatures: per-dimension binary choice (direct vs involuted path) enables Hamming pre-filter for sub-linear search
