# IOT Cursebreaking: Reference for SSRAG Implementation

This document captures the mathematical results from the cursebreaking proof that the SSRAG implementation needs. The full proof lives in `docs/iot_volume_expansion_proof.md` and `docs/cursebreaking/`.

## The IOT Metric

The Involuted Oblate Toroid defines a distance on a d-dimensional torus with major radius R and minor radius r:

```
Per-dimension distance:
  d_i = min(d_direct_i, alpha * d_involuted_i + beta)

Total distance:
  d_IOT(a,b) = sqrt(sum_i d_i^2)
```

The involution `I` flips each dimension: `(u_i, v_i) → (u_i + pi, -v_i)`. Taking the minimum of direct and involuted path creates a binary choice per dimension — this binary choice is what defeats the curse.

**Parameters:**
- `alpha = 0.92` — involuted path scale
- `beta = 0.04` — involuted path offset
- `R:r` — adaptive, see below

## Why It Reverses The Curse

On a Euclidean hypersphere, all volume concentrates on an equatorial shell in high dimensions. All points become equidistant.

On the IOT torus, the involution creates "wormholes" between the outer edge (volume-rich) and inner edge (volume-poor). For two database points a and b, the per-dimension binary choice (direct vs involuted) is DIFFERENT — their distances to a query are decorrelated. This breaks the concentration mechanism.

Formally: Euclidean pairwise distances become perfectly correlated as d→∞ (causing concentration). IOT pairwise distances stay decorrelated because different points make different binary path choices per dimension.

## Critical Scaling Law

The contrast ratio `C(d) = E[(D_max - D_min) / D_min]` depends on both dimension d and radius ratio R/r:

```
C(d) ~ [(R+r)/(R-r)]^(d/2) / sqrt(d)
```

Three regimes exist:

| r/R vs threshold | R/r | Contrast | Regime |
|------------------|-----|----------|--------|
| r/R > ln(d)/(2d) | R/r < 2d/ln(d) | GROWS with d | Expansion (anti-curse) |
| r/R = ln(d)/(2d) | R/r = 2d/ln(d) | CONSTANT | Critical |
| r/R < ln(d)/(2d) | R/r > 2d/ln(d) | SHRINKS with d | Contraction (curse) |

## Dimension-Adaptive R:r

To stay in the expansion regime at any dimensionality, use:

```
R(d) = d / (c * ln(d)) * r,    c = 2
```

This keeps the system at 2x critical regardless of d. Contrast grows as sqrt(d).

| Effective dims | R/r at c=2 | Contrast C(d) |
|---------------|------------|---------------|
| 32 | 2.9 | 4.0 |
| 128 | 7.7 | 8.0 |
| 308 | 30.6 | 13.7 |
| 1,024 | 41.1 | 22.6 |
| 4,096 | 134.2 | 45.3 |
| 100,000 | ~4,345 | ~316 |
| 1,000,000 | ~34,538 | ~1,000 |

## Integer-Native Arithmetic

Fingerprints MUST be int16, not quantized floats. This is not an optimization — it is the correct type.

The conventional pipeline introduces two unnecessary conversions:
```
int (true type) → float32 (widen, wasteful) → int8 (quantize, lossy)
```

SSRAG should go:
```
int16 (extraction) → int16 (storage) → int16 (distance computation)
```

Dot products accumulate in int64 to avoid overflow: `32767^2 * max_dims` must fit int64. At 1M dims: `32767^2 * 1000000 ≈ 1.07e15`, within int64 range.

## Path Signatures

For sub-linear search, compute a binary signature per (query, candidate) pair:
- For each active dimension, record whether the direct or involuted path was shorter (1 bit)
- Pack into uint64 words
- Candidates with similar Hamming distance on signatures are geometrically nearby on the IOT

This enables a two-phase search:
1. Hamming pre-filter: find candidates with similar path signatures (fast, GPU-friendly)
2. Exact IOT distance: rank filtered candidates by true IOT distance

The Hamming phase is embarrassingly parallel — each comparison is independent bitwise XOR + popcount.

## What SSRAG Needs From This

1. **IOT distance function**: `float iot_distance(SparseVector a, SparseVector b, IOTParams params)` — CatWhisper already has this
2. **Adaptive R:r**: call `adaptive_kappa(effective_dim)` before each distance computation — CatWhisper already has this
3. **Path signature computation**: for pre-filtering — CatWhisper already has this
4. **Sparse ANN index**: using path signatures for pre-filter, IOT distance for ranking — CatWhisper already has this

CatWhisper (`third_party/catwhisper/`) is the dependency. Everything above is implemented there. SSRAG builds on top: document ingestion, fingerprint extraction, lambda fold wiring, MCP interface.
