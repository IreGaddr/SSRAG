# SSRAG — Self-Sharpening Retrieval-Augmented Generation

A retrieval system that improves with scale. SSRAG uses the **Involuted Oblate Toroid (IOT)
distance metric** via [CatWhisper](https://github.com/IreGaddr/catwhisper) and a **lambda fold**
graph enrichment algorithm to build a self-reinforcing semantic index over structured data.

As more documents are ingested, each document's fingerprint is enriched with fold dimensions
pointing to its neighbors — creating a graph structure embedded directly in the vector space.
The effective dimensionality grows with corpus size, increasing the contrast ratio and making
retrieval sharper as the index grows.

## What's in this repo

**Core system** (C++23):
- `src/lambda_fold.cpp` — Lambda fold engine: fingerprinting, parallel neighbor discovery, bidirectional fold edge creation
- `src/document_store.cpp` — Thread-safe chunk storage with persistence
- `src/fingerprint.cpp` — FNV-1a hashed TF-IDF sparse vector fingerprinting
- `src/ingest_string.cpp` — STRING protein database ingestion pipeline
- `src/query_string.cpp` — Interactive query tool: text search, protein neighborhoods, multi-target overlap analysis
- `src/mcp_server.cpp` / `src/main.cpp` — MCP server interface

**Research findings** (`docs/findings/`):
- Polypharmacy convergence node analysis on the human protein interactome
- FDA FAERS adverse event validation
- Four polypharmacy syndrome analyses (cardiac, nephrotoxicity, GI bleeding, anticholinergic burden, falls)

---

## Replicating the Research Results

The research uses SSRAG to analyze the human protein interactome from [STRING v12.0](https://string-db.org/)
(19,699 proteins, 11.9M interactions) to identify convergence nodes where multiple drug target
neighborhoods overlap — potential mediators of adverse polypharmacy interactions.

### Prerequisites

**System requirements:**
- C++23 compiler (GCC 13+ or Clang 17+)
- CMake 3.20+
- Vulkan SDK (for CatWhisper GPU acceleration)
- ~4GB RAM for ingestion
- ~2GB disk for STRING data files

**Install dependencies (Ubuntu/Debian):**
```bash
sudo apt install build-essential cmake libvulkan-dev vulkan-tools
```

**Install dependencies (Fedora):**
```bash
sudo dnf install gcc-c++ cmake vulkan-loader-devel vulkan-tools
```

**Install dependencies (Arch):**
```bash
sudo pacman -S base-devel cmake vulkan-icd-loader vulkan-headers
```

### Step 1: Clone and build

```bash
git clone https://github.com/IreGaddr/SSRAG.git
cd SSRAG

# Clone CatWhisper into the project root
git clone https://github.com/IreGaddr/catwhisper.git

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

This produces three binaries:
- `ssrag-ingest-string` — Ingests STRING database files into an SSRAG index
- `ssrag-query` — Interactive query tool
- `ssrag-mcp` — MCP server (for IDE/agent integration)

### Step 2: Download STRING v12.0 human data

Download three files from [STRING](https://string-db.org/cgi/download):

```bash
mkdir -p data/string_human && cd data/string_human

# Protein info (names, sizes, annotations)
wget https://stringdb-downloads.org/download/protein.info.v12.0/9606.protein.info.v12.0.txt.gz

# Detailed interaction links (all evidence channels)
wget https://stringdb-downloads.org/download/protein.links.detailed.v12.0/9606.protein.links.detailed.v12.0.txt.gz

# Functional enrichment terms (GO, KEGG, Reactome, etc.)
wget https://stringdb-downloads.org/download/protein.enrichment.terms.v12.0/9606.protein.enrichment.terms.v12.0.txt.gz

cd ../..
```

The ingestion pipeline will decompress these automatically on first run. Uncompressed total
is ~1.2GB.

### Step 3: Ingest the human interactome

```bash
build/ssrag-ingest-string \
    --data-dir data/string_human \
    --out-dir data/ssrag_human \
    --organism 9606 \
    --top-partners 50
```

This takes approximately **65 seconds** on a 16-thread CPU. You'll see progress output:

```
Parsing protein info...
  19699 proteins
Parsing interactions (keeping top 50 per protein)...
Parsing enrichment terms...
Building protein documents...
Ingesting 19699 protein documents...
  [19699/19699] dim=19955 edges=3421130 contrast=141.262:1 (303 docs/sec)

Ingestion complete.
  Documents: 19699
  Effective dimensionality: 19955
  Estimated contrast: 141.262:1
  Fold edges: 3421130
  Time: 65s
```

**Key parameters** (hardcoded in `src/ingest_string.cpp`):
- `fold_k = 200` — Each protein connects to up to 200 fold neighbors
- `fold_threshold = 0.02` — Minimum cosine similarity to create a fold edge
- `max_fold_weight = 16000` — Maximum int16 weight for fold dimensions

The output is stored in `data/ssrag_human/`:
- `store.bin` (~217MB) — All protein documents and fingerprints
- `index.bin.meta` (4B) — Effective dimensionality metadata

### Step 4: Run the polypharmacy queries

The query tool supports three modes:

```bash
# Interactive mode
build/ssrag-query --data-dir data/ssrag_human --k 30

# Query modes (enter at the ssrag> prompt):
#   Text search:            mitochondrial complex I
#   Protein neighborhood:   @EGFR
#   Multi-target overlap:   @EGFR @BRAF @MAPK1
#   Change result count:    k=50
#   Exit:                   quit
```

#### Cardiac convergence (Finding 1)

The original cardiac analysis queried drug targets for the 6 most commonly co-prescribed
drug classes in elderly patients:

| Drug class | Representative | Primary target |
|------------|---------------|----------------|
| CCB | Amlodipine | CACNA1C |
| SSRI | Sertraline | SLC6A4 |
| PPI | Omeprazole | ATP4A |
| Gabapentinoid | Gabapentin | CACNA2D1 |
| Benzodiazepine | Lorazepam | GABRA1 |
| ACE inhibitor | Lisinopril | ACE |

```bash
# Run the 6-target overlap query
echo "k=30
@CACNA1C @SLC6A4 @ATP4A @CACNA2D1 @GABRA1 @ACE" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

**Expected result**: Convergence nodes including HCN4 (cardiac pacemaker channel),
KCNMB1 (calcium-activated potassium channel), HTR3B (serotonin ion channel),
CNR1/CNR2 (cannabinoid receptors), TBXA2R (thromboxane receptor).

See `docs/findings/polypharmacy-convergence-nodes.md` for the full cardiac analysis.

#### Nephrotoxicity — "Triple whammy" (Finding 2)

```bash
echo "k=30
@ACE @PTGS1 @PTGS2 @SLC12A3" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

**Expected result**: Sparse convergence (7 proteins). Key nodes: ALOX15
(lipoxin synthesis — renal protective lipid mediators), HSPA5 (ER stress chaperone).
Confirms triple whammy is primarily hemodynamic, not protein-network-mediated.

#### GI bleeding — SSRI + NSAID + Anticoagulant (Finding 3)

```bash
echo "k=30
@SLC6A4 @PTGS1 @PTGS2 @VKORC1" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

**Expected result**: Minimal convergence (3 proteins, all PTGS1-PTGS2 co-neighbors).
Zero cross-pathway overlap. Confirms GI bleeding risk is additively independent
across serotonin/COX/coagulation pathways.

#### Anticholinergic cognitive burden (Finding 4)

```bash
echo "k=30
@CHRM1 @CHRM2 @CHRM3 @HRH1 @ACHE" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

**Expected result**: Dense convergence (30 proteins, 18 in 3+ neighborhoods). Key nodes:
- **CHRM5** — Unmonitored muscarinic receptor in hippocampus (not in ACB scoring)
- **HRH3** — Histamine autoreceptor; feedback loop that amplifies anticholinergic deficit
  when antihistamines trigger compensatory histamine release
- **HTR1D** — Serotonin-cholinergic cross-talk; explains mood changes with anticholinergic burden
- **OPRM1** — Opioid-cholinergic gateway; molecular basis for opioid + anticholinergic delirium synergy

#### Falls and fracture risk (Finding 5)

```bash
echo "k=30
@GABRA1 @SLC6A4 @ADRB1 @OPRM1 @CACNA1C" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

**Expected result**: Dense convergence (30 proteins). Key nodes:
- **KCNK family** (KCNK1/3/5/6/13/18) — Six K2P potassium channels that set neuronal
  resting membrane potential. Benzos and CCBs converge on the entire family, shifting the
  electrical baseline of baroreceptor and vestibular neurons.
- **HTR3B** — Recurring cross-syndrome node (also in cardiac analysis)
- **HCN2** — Pacemaker channel in both heart and thalamocortical neurons
- **TRPM2** — Oxidative stress calcium channel; benzo + opioid + CCB convergence point

### Step 5: FAERS validation (optional)

The `scripts/faers_validate.sh` script queries the FDA Adverse Event Reporting System
via the openFDA API to check cardiac adverse event rates in single-drug vs multi-drug
reports:

```bash
bash scripts/faers_validate.sh
```

This takes ~6 minutes (API rate limiting). Results are saved to
`docs/findings/faers_validation/`. Note: the FAERS validation using reaction-rate
analysis was **inconclusive** — see `docs/findings/faers-validation.md` for the honest
assessment of why reaction-rate is the wrong metric for this analysis and what a proper
validation would require.

---

## How it works

### Lambda fold algorithm

1. **Fingerprint**: Each document is hashed into a 256-dimensional sparse TF-IDF vector
   using FNV-1a hashing into int16 buckets.

2. **Neighbor discovery**: On ingestion, the new document is compared against all existing
   documents using cosine similarity (parallelized across all CPU cores). The top
   `fold_k` neighbors above `fold_threshold` are selected.

3. **Fold edge creation**: For each neighbor, a bidirectional fold dimension is created:
   - The new document gains dimension `256 + neighbor.node_index` with weight proportional
     to similarity
   - The neighbor gains dimension `256 + new_doc.node_index` with the same weight
   - These "fold dimensions" encode graph structure directly in the vector space

4. **Retrieval**: Two modes depending on query type:
   - **Text search**: Strips fold dimensions, computes IOT distance on base 256 dims only
   - **Neighborhood query**: Computes cosine similarity on fold dimensions only (index >= 256),
     measuring shared structural connections in the protein interaction graph

### Why it sharpens with scale

The effective dimensionality grows as `256 + N` where N is the number of documents.
With 19,699 proteins, the effective space is 19,955-dimensional. The IOT distance metric's
contrast ratio scales as `sqrt(effective_dim)`, so retrieval discrimination improves as
more documents are added — the opposite of the "curse of dimensionality" that degrades
most high-dimensional retrieval systems.

---

## Project structure

```
SSRAG/
  CMakeLists.txt              # Build system
  include/ssrag/              # Public headers
    document_store.hpp        # Thread-safe chunk storage
    fingerprint.hpp           # TF-IDF sparse fingerprinting
    lambda_fold.hpp           # Fold engine interface
    mcp_server.hpp            # MCP server interface
  src/
    document_store.cpp         # DocumentStore implementation
    fingerprint.cpp            # FNV-1a tokenization and hashing
    lambda_fold.cpp            # Lambda fold engine (parallel ingest + search)
    ingest_string.cpp          # STRING database ingestion pipeline
    query_string.cpp           # Interactive query tool (3 query modes)
    mcp_server.cpp             # MCP JSON-RPC server
    main.cpp                   # MCP server entry point
  tests/                       # Unit tests
  scripts/
    faers_validate.sh          # FDA FAERS adverse event validation
  docs/
    design/                    # Architecture and design docs
    findings/                  # Research results and raw data
      polypharmacy-convergence-nodes.md   # Cardiac convergence analysis
      polypharmacy-beyond-cardiac.md      # Nephro, GI, anticholinergic, falls
      faers-validation.md                 # FAERS validation (honest assessment)
      raw-query-results.txt               # Reproducible raw outputs
      raw-query-results-extended.txt      # Extended query outputs
    cursebreaking/             # IOT metric theory (LaTeX)
  catwhisper/                  # CatWhisper library (separate repo, see .gitignore)
  data/                        # STRING data + SSRAG indexes (not in repo, see Step 2)
```

## Dependencies

- [CatWhisper](https://github.com/IreGaddr/catwhisper) — IOT distance metric, sparse vectors, ANN index
- [STRING v12.0](https://string-db.org/) — Protein interaction database (download separately)
- Vulkan SDK — GPU acceleration for CatWhisper
- C++23 — Required language standard
- pthreads — Parallel ingestion and query

## License

This project and its findings are provided as-is for research purposes.
The STRING database is subject to its own [license terms](https://string-db.org/cgi/access).
