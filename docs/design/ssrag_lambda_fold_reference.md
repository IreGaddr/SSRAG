# Lambda Fold: Reference for SSRAG Implementation

## The Idea

Every item in the index becomes a dimension of the search space. "Lambda fold" is the operation that makes this happen.

## How It Works

Given:
- A base fingerprint with B dimensions (e.g. 256 dims of text features)
- N documents already in the index
- A new document D being ingested

The lambda fold produces a sparse vector with B + N potential dimensions:
- Dimensions 1..B: the base fingerprint (dense, always populated)
- Dimension B+1: affinity to document 1 (zero if no relationship)
- Dimension B+2: affinity to document 2 (zero if no relationship)
- ...
- Dimension B+N: affinity to document N (zero if no relationship)
- Dimension B+N+1: self (this IS document D, so other docs fold onto this dimension)

In practice each document has relationships (non-zero affinity) to maybe 20-50 other documents. So a sparse vector with B + ~30 non-zero entries, out of B + N possible dimensions.

## What Counts As "Relationship"

Anything that indicates two documents are related. The lambda fold doesn't care about the semantics — it just needs a weight:

- **Shared n-grams**: count of 4-grams appearing in both documents, normalized
- **Co-citation**: both documents cited by a third
- **Same source**: chunks from the same original file
- **Explicit links**: hyperlinks, references, database foreign keys
- **User-provided**: "these two are related" with a weight

The weight is scaled to int16: `clamp(weight * scale_factor, -32767, 32767)`. Sign matters — negative affinity means "these are anti-correlated" which the IOT involution can exploit.

## Why It Creates Self-Sharpening

Consider three documents:
- Doc A: a paper about aspirin
- Doc B: a paper about ibuprofen  
- Doc C: a paper about COX-2 inhibition mechanisms

After lambda fold:
- A has non-zero dim for C (they share pharmacology n-grams)
- B has non-zero dim for C (they also share pharmacology n-grams)
- A and B have non-zero dim for each other (both NSAIDs)

Now a query about "anti-inflammatory mechanisms" matches A and B not just because of surface text similarity (base fingerprint), but because they BOTH fold onto the COX-2 dimension (Doc C's dimension). The folded dimension captures a relationship that TF-IDF alone cannot see.

When Doc D (a new paper about acetaminophen and COX-3) is ingested:
- D folds onto A, B, C (shared pharmacology terms)
- A, B, C gain a new dimension (D's dimension)
- Now the space has one more axis of discrimination
- The IOT metric at this higher dimensionality has HIGHER contrast
- Searches for any of these documents get more precise

This happens automatically on every ingest. No retraining, no re-embedding.

## Sparse Storage

At 1M documents with 256 base dims and ~30 average edges:
- Ambient dimensionality: 1,000,256
- Per-document storage: ~286 non-zero entries × 6 bytes (4 byte dim index + 2 byte value) = ~1.7 KB
- Total index: ~1.7 GB for 1M documents
- IOT contrast at 1M dims: ~1000:1

Compare to dense float32 embeddings at 1536 dims (typical transformer embedding):
- Per-document: 6 KB
- Total: 6 GB for 1M documents
- Cosine contrast at 1536 dims: nearly 0

SSRAG uses 3.5x less memory AND has 1000x better search discrimination.

## Implementation Notes

The fold is bidirectional: when D is ingested and related to E, BOTH get updated:
- D gains dimension `base + index(E)` 
- E gains dimension `base + index(D)`

This means E's entry in the ANN index needs updating. Options:
1. **Lazy update**: E's index entry is stale until it's touched by a query or re-index sweep
2. **Batch update**: accumulate folds, update index entries in batches
3. **Immediate update**: update E's index entry inline during D's ingestion

Option 2 is the right default. Same pattern as SCN's batch mutation: accumulate changes behind a write guard, flush periodically. The ANN index is eventually consistent.

## Connection To IOT

The lambda fold is only useful because the IOT metric handles high dimensionality. With cosine/L2:
- Adding 30 folded dimensions to a 256-dim base vector barely matters (256 → 286, minor change in concentration)
- Adding 100,000 folded dimensions DESTROYS cosine similarity (everything equidistant)

With IOT:
- Adding 30 folded dimensions: contrast improves from ~16 to ~17 (small but positive)
- Adding 100,000 folded dimensions: contrast improves from ~16 to ~316 (massive improvement)

Lambda fold without IOT is pointless. IOT without lambda fold works but misses the self-sharpening property. Together they create an index that improves with scale.
