# SSRAG Implementation Plan

## Dependency

`third_party/catwhisper/` — vendor as submodule. It provides:
- `iot_distance()` — IOT distance for sparse int16 vectors
- `IndexSparseIOT` — sparse ANN index with path signature pre-filter
- `SparseVector` — sparse int16 vector type
- `PathSignature` — binary signature + Hamming distance
- `adaptive_kappa()` — dimension-adaptive R:r scaling
- GPU Hamming batch shader

Everything else is built from scratch.

## Components

### 1. Document Fingerprint Extractor

`text → int16 sparse vector`

Base fingerprint (~256 dims):
- Token hashes: FNV-1a hash of each unique term, modulo 256, value = TF-IDF weight scaled to int16
- This is a hashing trick (like scikit-learn's HashingVectorizer) but outputting int16 directly

That's the minimum. Domain-specific extractors can replace or extend this with richer features. But hashed TF-IDF is enough to bootstrap.

### 2. Document Store

Stores the actual text so search results can be returned:
- Chunk ID → chunk text + metadata (source path, byte range, ingestion timestamp)
- Chunk ID → sparse fingerprint (base + folded)
- Chunk ID → node index (for lambda fold dimension assignment)

Persistence: binary snapshot. Adapt the journal snapshot pattern from SCN if useful, or just serialize/deserialize. Don't over-engineer this.

### 3. Lambda Fold Engine

On ingest of chunk D:
1. Compute base fingerprint of D
2. Search existing index for top-k similar chunks (using the base fingerprint only)
3. For each similar chunk E with similarity above threshold:
   - D's sparse vector gets dimension `base_dims + index(E)` = similarity score as int16
   - E's sparse vector gets dimension `base_dims + index(D)` = similarity score as int16
   - Queue E for index update
4. Insert D into IOT index
5. Batch-update queued entries in IOT index

Step 2 is bootstrapping: the first document has no folded dimensions. The second might share some n-grams with the first, getting one folded dim. By document 1000, the fold is rich.

### 4. MCP Server

Three tools:

**`ssrag_ingest`**
- Input: file path or directory path
- Chunks documents (fixed size or paragraph boundaries)
- Runs fingerprint extraction + lambda fold for each chunk
- Returns: count ingested, new effective dimensionality

**`ssrag_search`**
- Input: query string, k (default 10)
- Fingerprints the query (base dims only — query has no folded dims since it's not in the index)
- IOT ANN search against the index
- Returns: top-k chunks with text content and distance scores

**`ssrag_status`**
- Returns: total chunks indexed, effective dimensionality, estimated contrast ratio

### 5. Build System

CMake. Link catwhisper. No other dependencies besides standard C++20 and spdlog for logging.

## Build Order

1. Get catwhisper building as submodule
2. Write the fingerprint extractor + a test that fingerprints a text file
3. Write the document store + a test that round-trips chunk storage
4. Write the lambda fold engine + a test that ingests 100 documents and verifies dimensionality growth
5. Write the MCP server + manual test via Claude
6. Scale test: ingest 10k, 100k documents, measure contrast ratio vs corpus size

## Validation

The single most important test: **contrast ratio vs corpus size**.

```
Ingest N documents. Sample 100 random queries. For each query:
  - Record D_min (nearest neighbor distance)
  - Record D_max (farthest of top-100 distances)  
  - Compute C = (D_max - D_min) / D_min

Plot mean(C) vs N. 

Expected: C grows as approximately sqrt(base_dims + N).
If C is flat or declining: lambda fold is broken (edges too sparse, 
dimensions not populating, or IOT params wrong).
```

If the curve goes up: the product works.
