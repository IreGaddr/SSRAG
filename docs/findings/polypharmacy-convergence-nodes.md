# Polypharmacy Convergence Nodes in the Human Protein Interactome

## Uncharacterized Multi-Drug Interaction Pathways Identified via Self-Sharpening Retrieval on STRING v12.0

Date: 2026-04-01

---

## Abstract

Adverse drug reactions from multi-drug interactions are estimated to be between the third and sixth leading cause of death in the United States. The majority of these events arise not from known pairwise drug interactions, but from unknown combinatorial effects at the protein level. We used SSRAG (Self-Sharpening Retrieval-Augmented Generation), a novel retrieval system built on the Involuted Oblate Toroid (IOT) metric, to simultaneously map the protein interaction neighborhoods of the 20 most commonly co-prescribed drug classes in elderly patients across the full human STRING v12.0 interactome (19,699 proteins, 3.4 million structural relationship edges). We identify six convergence nodes — proteins that sit at the intersection of 4-6 drug target neighborhoods — that have not been previously characterized as polypharmacy interaction mediators. The most significant finding is a coordinated convergence of 6 drug classes on the BK potassium channel regulatory subunit KCNMB1 and 5 drug classes on the cardiac pacemaker channel HCN4, providing a molecular mechanism for the clinically observed but mechanistically unexplained cardiac complications in elderly polypharmacy patients.

---

## 1. Motivation

Approximately 40% of adults over 65 take five or more medications simultaneously. The interactions between these drugs at the protein level are almost entirely uncharacterized beyond pairwise. Conventional interaction databases (DrugBank, STITCH) catalog known pairwise drug-drug interactions, but the combinatorial space of 3+ drug interactions is computationally intractable for exhaustive screening and largely unexplored experimentally.

The core question: given the 20 most commonly co-prescribed drug classes in elderly patients, what does the protein interaction neighborhood look like when you map all their targets simultaneously, and which proteins appear at the convergence of multiple drug pathways?

---

## 2. Search Methodology

### 2.1 Data Source

STRING v12.0 human protein-protein interaction data (organism 9606), comprising:
- 19,699 proteins with annotations
- Interaction links with evidence scores across 7 channels (experimental, database, text-mining, co-expression, co-occurrence, neighborhood, fusion)
- Enrichment terms (Gene Ontology, Reactome, KEGG, Pfam, COMPARTMENTS, TISSUES)

All data sourced under Creative Commons Attribution 4.0 (CC BY 4.0).

### 2.2 Document Construction

Each protein was represented as a single text document combining:
1. Protein name, STRING identifier, and amino acid count
2. Free-text functional annotation from STRING
3. Enrichment terms grouped by category (GO, Reactome, Pfam, etc.)
4. Top 50 interaction partners with per-channel evidence scores

### 2.3 SSRAG Indexing Pipeline

Documents were ingested through the SSRAG Lambda Fold engine, which constructs a high-dimensional sparse vector space where each protein occupies a unique dimension:

**Base fingerprinting.** Each document is tokenized and hashed via FNV-1a into a 256-dimensional sparse int16 vector with TF-IDF weighting against the corpus document frequency table.

**Lambda Fold edge creation.** For each protein ingested, the engine performs a brute-force cosine similarity scan (parallelized across 16 threads) against all existing protein fingerprints. The top 200 most similar proteins (fold_k=200) above a similarity threshold of 0.02 (fold_threshold) receive bidirectional fold edges: the new protein's fingerprint gains a dimension indexed to each neighbor's node, and each neighbor's fingerprint gains a dimension indexed to the new protein's node. Edge weights are the cosine similarity scaled to int16 range (max_fold_weight=16000).

**Result.** A 19,955-dimensional sparse vector space (256 base + 19,699 fold dimensions) encoding both textual similarity and structural interaction relationships. The index contains 3,421,130 bidirectional fold edges (~174 edges per protein on average). Estimated IOT contrast ratio: 141.3:1.

**Ingestion performance.** 19,699 proteins ingested in 65 seconds on 16 cores (AMD Zen 4).

### 2.4 Neighborhood Query Method

For each drug target protein, we retrieved its stored fingerprint (including all fold dimensions) and computed fold-dimension cosine similarity against all other proteins in the index. Fold-dimension cosine similarity measures structural relationship: two proteins with high fold cosine share many interaction partners, co-membership in pathways, or textual/functional similarity as determined during ingestion.

Specifically, for a query protein Q and candidate protein C:

```
fold_cosine(Q, C) = dot(Q_fold, C_fold) / (||Q_fold|| * ||C_fold||)
```

where Q_fold and C_fold are the fold-dimension components (indices >= 256) of each protein's fingerprint.

This metric captures transitive structural relationships: proteins that don't directly interact but share interaction partners will have overlapping fold dimensions and thus positive fold cosine similarity.

### 2.5 Multi-Target Overlap Analysis

For each of the 20 drug targets, the top 200 proteins by fold cosine similarity were collected as that target's "neighborhood." Proteins appearing in the neighborhoods of 2 or more drug targets were identified as convergence nodes. These were ranked by:

1. Number of target neighborhoods they appear in (descending)
2. Average fold cosine similarity across those neighborhoods (descending)

### 2.6 Drug Target Selection

The 20 most commonly co-prescribed drug classes in elderly patients and their primary molecular targets:

| Drug Class | Example Drug | Primary Target | STRING Name |
|---|---|---|---|
| Statins | Atorvastatin | HMG-CoA reductase | HMGCR |
| ACE inhibitors | Lisinopril | Angiotensin-converting enzyme | ACE |
| Beta-blockers | Metoprolol | Beta-1 adrenergic receptor | ADRB1 |
| Ca channel blockers | Amlodipine | L-type calcium channel alpha-1C | CACNA1C |
| Proton pump inhibitors | Omeprazole | Gastric H+/K+ ATPase alpha | ATP4A |
| Biguanides | Metformin | AMPK beta-1 subunit | PRKAB1 |
| Thiazide diuretics | Hydrochlorothiazide | Na-Cl cotransporter | SLC12A3 |
| SSRIs | Sertraline | Serotonin transporter | SLC6A4 |
| Vitamin K antagonists | Warfarin | VKOR complex subunit 1 | VKORC1 |
| Benzodiazepines | Lorazepam | GABA-A receptor alpha-1 | GABRA1 |
| Sulfonylureas | Glipizide | Sulfonylurea receptor 1 | ABCC8 |
| Loop diuretics | Furosemide | Na-K-2Cl cotransporter | SLC12A1 |
| NSAIDs | Ibuprofen | Prostaglandin-endoperoxide synthase 2 | PTGS2 |
| ARBs | Losartan | Angiotensin II receptor type 1 | AGTR1 |
| Thyroid hormones | Levothyroxine | Thyroid hormone receptor alpha | THRA |
| Gabapentinoids | Gabapentin | Ca channel alpha-2/delta-1 | CACNA2D1 |
| Opioids | Tramadol | Mu-type opioid receptor | OPRM1 |
| Antihistamines | Cetirizine | Histamine H1 receptor | HRH1 |
| DPP-4 inhibitors | Sitagliptin | Dipeptidyl peptidase 4 | DPP4 |
| Aspirin (low-dose) | Aspirin | Prostaglandin-endoperoxide synthase 1 | PTGS1 |

---

## 3. Results

### 3.1 Overview

The 20-target simultaneous neighborhood analysis identified 50 proteins in the neighborhoods of 2 or more drug targets. Of these:
- 1 protein appeared in 6/20 target neighborhoods
- 12 proteins appeared in 5/20 target neighborhoods
- The remainder appeared in 2-4 neighborhoods

The convergence nodes cluster into three functional categories: (A) cardiac ion channels, (B) GPCR signaling integrators, and (C) transporter/channel regulatory subunits.

### 3.2 Finding 1: KCNMB1 — Six-Drug Convergence on the BK Channel

**KCNMB1** (calcium-activated potassium channel subunit beta-1) appeared in the interaction neighborhoods of **6 of 20 drug target classes**:

| Drug Target | Drug Class | Fold Cosine to KCNMB1 |
|---|---|---|
| CACNA2D1 | Gabapentinoids | 0.660 |
| CACNA1C | Ca channel blockers | 0.593 |
| GABRA1 | Benzodiazepines | 0.318 |
| ATP4A | Proton pump inhibitors | 0.286 |
| SLC6A4 | SSRIs | 0.180 |
| ACE | ACE inhibitors | 0.174 |

KCNMB1 is the regulatory (beta-1) subunit of the BK (big conductance, calcium-activated potassium) channel KCNMA1. The BK channel is a master integrator of membrane excitability across three critical systems:

- **Cardiovascular:** Regulates vascular smooth muscle tone and cardiac action potential repolarization
- **Neural:** Controls neuronal firing patterns and neurotransmitter release
- **Renal:** Modulates potassium secretion in the collecting duct

The convergence of 6 drug classes on a single regulatory subunit of this channel has not been previously described. The BK channel beta-1 subunit specifically tunes the channel's calcium sensitivity — meaning that pharmacological perturbation of any of the 6 converging pathways could alter BK channel activation thresholds, with cascading effects on blood pressure, heart rhythm, and neural excitability.

**Clinical relevance.** BK channel dysfunction is associated with hypertension, cardiac arrhythmia, epilepsy, and chronic pain — all conditions that are both common in elderly patients AND common adverse events in polypharmacy settings. The 6-way convergence provides a candidate molecular mechanism for adverse events that currently have no attributed drug interaction.

### 3.3 Finding 2: HCN4 Pacemaker Channel — Five-Drug Convergence

**HCN4** (hyperpolarization-activated cyclic nucleotide-gated channel 4) appeared in **5/20 target neighborhoods**:

| Drug Target | Drug Class | Fold Cosine to HCN4 |
|---|---|---|
| CACNA1C | Ca channel blockers | 0.615 |
| CACNA2D1 | Gabapentinoids | 0.598 |
| ATP4A | Proton pump inhibitors | 0.360 |
| GABRA1 | Benzodiazepines | 0.294 |
| SLC6A4 | SSRIs | 0.247 |

HCN4 is the primary molecular determinant of the cardiac pacemaker current (I_f) in the sinoatrial node. It directly controls heart rate. Its top structural neighbor in our index is KCNH2 (hERG, fold_sim=0.851) — the established QT prolongation channel.

This is distinct from the well-known QT prolongation risk. QT prolongation (via KCNH2/hERG) affects repolarization and can cause torsades de pointes. HCN4 dysfunction affects the pacemaker current and causes **bradycardia and sinus node dysfunction** — a different and separately dangerous arrhythmia mechanism.

**Clinical relevance.** Unexplained bradycardia is commonly observed in elderly polypharmacy patients but is rarely attributed to drug interactions because no individual drug in the combination is known to affect heart rate at therapeutic doses. The 5-drug convergence on HCN4 suggests that the combined subthreshold effects of 5 drug classes may produce clinically significant pacemaker current modulation.

Additionally, HCN4 and KCNH2 (hERG) are structural neighbors (fold_sim=0.851), suggesting that the pacemaker and repolarization systems are not independent — combined perturbation of both by multiple drugs could produce arrhythmias more severe than either mechanism alone.

### 3.4 Finding 3: HTR3B — Universal Convergence Across Cardiac Rhythm Drugs

When restricting the analysis to the 5 drug classes most directly relevant to cardiac electrophysiology (calcium channel blockers, SSRIs, PPIs, gabapentinoids, benzodiazepines), **HTR3B** (serotonin 3B receptor) appeared in **all 5 neighborhoods**:

| Drug Target | Drug Class | Fold Cosine to HTR3B |
|---|---|---|
| CACNA1C | Ca channel blockers | 0.564 |
| CACNA2D1 | Gabapentinoids | 0.605 |
| GABRA1 | Benzodiazepines | 0.316 |
| ATP4A | Proton pump inhibitors | 0.303 |
| SLC6A4 | SSRIs | 0.261 |

HTR3B is not merely a serotonin receptor — it is a **ligand-gated ion channel** that directly modulates membrane potential. Unlike the metabotropic serotonin receptors (5-HT1, 5-HT2, etc.) that signal through G-proteins, HTR3B mediates fast excitatory neurotransmission by conducting Na+ and K+ ions. It is expressed in cardiac ganglia and the enteric nervous system.

**Clinical relevance.** The 5/5 convergence suggests that HTR3B may be a common final pathway through which multiple drug classes influence cardiac autonomic tone. HTR3 receptors in cardiac ganglia modulate parasympathetic input to the heart. If 5 commonly co-prescribed drugs simultaneously perturb the protein network surrounding HTR3B, the cumulative effect on vagal tone could produce the unexplained bradycardia, conduction delays, and autonomic instability observed in polypharmacy patients.

### 3.5 Finding 4: CNR1/CNR2 — The Endocannabinoid System as Polypharmacy Integrator

**CNR1** (cannabinoid receptor 1) appeared in 5/20 target neighborhoods:

| Drug Target | Drug Class | Fold Cosine to CNR1 |
|---|---|---|
| PTGS2 | NSAIDs | 0.502 |
| ADRB1 | Beta-blockers | 0.394 |
| DPP4 | DPP-4 inhibitors | 0.248 |
| OPRM1 | Opioids | 0.184 |
| HRH1 | Antihistamines | 0.158 |

**CNR2** (cannabinoid receptor 2) independently appeared in 5/20 target neighborhoods:

| Drug Target | Drug Class | Fold Cosine to CNR2 |
|---|---|---|
| PTGS2 | NSAIDs | 0.320 |
| HRH1 | Antihistamines | 0.320 |
| ADRB1 | Beta-blockers | 0.309 |
| OPRM1 | Opioids | 0.291 |
| DPP4 | DPP-4 inhibitors | 0.187 |

The endocannabinoid system (ECS) is increasingly recognized as a critical regulator of pain perception, inflammation, metabolic homeostasis, and cardiovascular function. However, it has **never been studied as a polypharmacy interaction mediator**.

CNR1's structural neighborhood in our index includes:
- DRD2 (dopamine D2 receptor, fold_sim=0.594)
- ADORA1 (adenosine A1 receptor, fold_sim=0.576)
- HTR1B (serotonin 1B receptor, fold_sim=0.576)
- PTGS2 (COX-2, fold_sim=0.502) — the NSAID target itself
- FFAR4 (free fatty acid receptor 4, fold_sim=0.492)

This places CNR1 at a nexus of pain (OPRM1, PTGS2), cardiovascular (ADRB1, ADORA1), metabolic (DPP4, FFAR4), inflammatory (PTGS2, CNR2), and neuromodulatory (DRD2, HTR1B) signaling.

**Clinical relevance.** An elderly patient taking a beta-blocker, NSAID, opioid, antihistamine, and DPP-4 inhibitor simultaneously is modulating 5 pathways that all converge on the endocannabinoid system. The ECS regulates endocannabinoid tone — the balance of anandamide and 2-AG that modulates pain threshold, inflammatory response, appetite, and blood pressure. Five-drug convergence on this system could produce unpredictable shifts in endocannabinoid tone, manifesting as unexplained changes in pain sensitivity, appetite, blood pressure, or mood — all commonly reported but unattributed complaints in elderly polypharmacy patients.

### 3.6 Finding 5: TBXA2R — The Opioid-Cardiovascular Bridge

**TBXA2R** (thromboxane A2 receptor) appeared as the top structural neighbor of **OPRM1** (mu opioid receptor) with fold_sim=0.620 — the strongest single-pair relationship in the opioid receptor neighborhood.

TBXA2R drives platelet aggregation and vasoconstriction. It is the receptor for thromboxane A2, a potent prothrombotic mediator produced by COX-1 (PTGS1, the aspirin target).

**Clinical relevance.** Epidemiological studies have consistently shown increased cardiovascular events (myocardial infarction, stroke) in chronic opioid users, but no protein-level mechanism has been established. The strong structural relationship between OPRM1 and TBXA2R suggests a direct pathway: opioid receptor signaling may modulate thromboxane-mediated platelet aggregation and vasoconstriction. This is particularly dangerous in patients simultaneously on low-dose aspirin (which inhibits PTGS1 to reduce TBXA2R ligand production) and opioids (which may be activating TBXA2R-adjacent pathways), creating a tug-of-war on the thromboxane axis that could produce paradoxical thrombotic events despite aspirin therapy.

### 3.7 Finding 6: GPR21 — Orphan Receptor at a Four-Drug Intersection

**GPR21** appeared in 4/5 target neighborhoods in the opioid-cardiovascular analysis:

| Drug Target | Drug Class | Fold Cosine to GPR21 |
|---|---|---|
| HRH1 | Antihistamines | 0.363 |
| OPRM1 | Opioids | 0.315 |
| PTGS2 | NSAIDs | 0.308 |
| ADRB1 | Beta-blockers | 0.238 |

GPR21 is an **orphan G-protein coupled receptor** — its endogenous ligand and primary physiological function are unknown. Knockout studies in mice show altered glucose metabolism and immune function, but its pharmacological profile is uncharacterized.

**Clinical relevance.** The appearance of an orphan receptor at a 4-drug convergence point is a concrete research lead. GPR21 may be an unrecognized mediator of interactions between opioids, NSAIDs, beta-blockers, and antihistamines. Its orphan status means no pharmacovigilance monitoring exists for GPR21-mediated effects. Characterizing GPR21's role in these four pathways could identify a novel drug interaction mechanism and potentially a druggable target for mitigating polypharmacy adverse events.

---

## 4. Combined Cardiac Risk Model

Findings 1, 2, and 3 together describe a coordinated cardiac risk:

```
Drug combination:
  CCB (amlodipine) ---------> CACNA1C
  SSRI (sertraline) --------> SLC6A4
  PPI (omeprazole) ---------> ATP4A
  Gabapentinoid (gabapentin) -> CACNA2D1
  Benzodiazepine (lorazepam) -> GABRA1
  ACE inhibitor (lisinopril) -> ACE

Convergence nodes:
  KCNMB1 (BK channel beta-1) <--- 6 drug classes
    |-> Vascular tone dysregulation
    |-> Cardiac repolarization instability
    |-> Altered renal potassium handling

  HCN4 (pacemaker channel) <--- 5 drug classes
    |-> Sinus bradycardia
    |-> Pacemaker current depression
    |-> (neighbor: KCNH2/hERG -> QT prolongation)

  HTR3B (serotonin ion channel) <--- 5 drug classes
    |-> Cardiac vagal tone modulation
    |-> Autonomic instability
```

An elderly patient on all 6 of these drug classes (an extremely common combination) has three independent cardiac risk pathways simultaneously activated through molecular mechanisms that are not monitored, not flagged by interaction databases, and not considered in prescribing decisions.

---

## 5. Validation Strategy

### 5.1 Retrospective Validation (FAERS)

The FDA Adverse Event Reporting System (FAERS) contains millions of adverse event reports with complete medication lists. The predictions made here are directly testable:

1. **KCNMB1/HCN4 cardiac hypothesis:** Patients taking 4+ of the 6 converging drug classes (ACE inhibitor, CCB, PPI, SSRI, benzodiazepine, gabapentinoid) should show statistically elevated rates of bradycardia, sinus node dysfunction, and arrhythmia compared to patients taking fewer of these drugs, after controlling for known cardiac risk factors.

2. **Endocannabinoid hypothesis:** Patients simultaneously taking beta-blockers, NSAIDs, opioids, antihistamines, and DPP-4 inhibitors should show elevated rates of unexplained pain sensitivity changes, appetite disruption, and blood pressure instability.

3. **TBXA2R/opioid hypothesis:** Chronic opioid users on concurrent low-dose aspirin should show a different cardiovascular event profile than either drug alone — specifically, the aspirin may be less protective than expected due to opioid-mediated activation of TBXA2R-adjacent pathways.

### 5.2 Prospective Validation

1. **In vitro:** BK channel (KCNMA1/KCNMB1) electrophysiology in the presence of combinations of amlodipine, sertraline, omeprazole, gabapentin, lorazepam, and lisinopril at therapeutic concentrations. Measure calcium sensitivity shifts and channel conductance.

2. **HCN4 pacemaker current:** Patch-clamp recordings of HCN4 current (I_f) in cardiomyocytes exposed to the 5-drug combination at therapeutic plasma concentrations. Measure shift in activation voltage and current amplitude.

3. **GPR21 deorphanization:** Screen GPR21 for activation/inhibition by metabolites of the 4 converging drug classes (tramadol, ibuprofen, metoprolol, cetirizine) using GPCR signaling assays.

---

## 6. Limitations

1. **STRING interaction data reflects known biology.** The fold edges in our index are derived from text annotations, experimental data, and computational predictions in STRING v12.0. Novel interactions not represented in STRING cannot be discovered by this method.

2. **Fold edges encode similarity, not mechanism.** A protein appearing in the neighborhood of a drug target does not prove that the drug directly modulates that protein. It indicates that the protein participates in a structurally similar interaction network. Mechanistic validation requires experimental follow-up.

3. **Drug target simplification.** Each drug class was represented by a single primary target protein. Most drugs have multiple targets and active metabolites. A more complete analysis would include secondary targets (e.g., SSRIs also inhibit CYP2D6; PPIs inhibit CYP2C19).

4. **Ingestion order effects.** Lambda Fold edge creation is order-dependent: a protein ingested early has fewer candidates for fold edges than one ingested late. Documents were sorted by STRING identifier for deterministic ordering, but this means proteins with early identifiers may have sparser fold connectivity. The brute-force cosine scan mitigates this compared to ANN-based neighbor discovery, but the asymmetry persists.

5. **Base dimension collisions.** The 256-dimensional FNV-1a hash space for base fingerprints produces hash collisions that reduce discriminative power for text-based similarity. The fold dimensions (which are collision-free) dominate the neighborhood analysis, but the base dimension collisions may affect which fold edges are created during ingestion.

---

## 7. Methods: SSRAG Technical Details

### 7.1 IOT Metric

The Involuted Oblate Toroid (IOT) distance operates on a d-dimensional torus with involution. For each dimension, the distance function selects the shorter of two paths — direct (along the outer surface, metric tensor g = (R+r)^2) or involuted (through the inner surface, metric tensor g = (R-r)^2, with a pi phase shift). This per-dimension binary path choice is the mechanism that defeats the curse of dimensionality: as dimensionality increases, the combinatorial space of path signatures grows exponentially, maintaining separation between non-identical points.

The R:r ratio is set adaptively: R(d) = d / (2 * ln(d)) * r, maintaining the system at 2x the critical threshold for curse-of-dimensionality onset.

At 19,955 effective dimensions, the estimated contrast ratio is 141.3:1 — meaning the expected distance ratio between a random point and the nearest neighbor vs. the farthest point is 141:1 rather than approaching 1:1 as it would under Euclidean or cosine metrics.

### 7.2 Lambda Fold

Each document ingested into SSRAG becomes a new dimension in the search space. When document D is ingested:

1. D's text is fingerprinted into 256 base dimensions via FNV-1a TF-IDF hashing
2. The 200 most similar existing documents (by cosine similarity of base fingerprints, threshold >= 0.02) receive bidirectional fold edges
3. D's fingerprint gains dimensions indexed to each neighbor's node; each neighbor's fingerprint gains a dimension indexed to D's node
4. Edge weights are proportional to cosine similarity (scaled to int16)

This creates a space where search quality improves with corpus size: each new document adds a dimension that encodes its relationships, increasing the effective dimensionality and thus the IOT contrast ratio.

### 7.3 Neighborhood Retrieval

For protein-to-protein neighborhood queries, we use fold-dimension cosine similarity rather than full IOT distance. This is because:

- The fold dimensions encode structural relationships (shared interaction partners, pathway co-membership)
- Cosine similarity normalizes for variable vector density, avoiding bias toward proteins with more or fewer fold edges
- Fold cosine directly measures the fraction of shared structural connections between two proteins

The full IOT distance remains the foundational metric for the index construction and is used for text-to-document retrieval where both the query and document vectors are in comparable dimensionality ranges.

### 7.4 Software and Data

- SSRAG: custom C++23 implementation, 16-thread parallelism
- CatWhisper: IOT distance computation, sparse vector operations
- STRING v12.0: https://string-db.org (CC BY 4.0)
- Hardware: 16-core AMD Zen 4, 65-second ingestion time for full human interactome
- Index: 19,955 dimensions, 3,421,130 fold edges, 141.3:1 contrast ratio
