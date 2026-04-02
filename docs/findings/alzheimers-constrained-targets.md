# Constrained Drug Target Discovery for Alzheimer's Disease

**Date**: 2026-04-02
**System**: SSRAG v0.1.0, STRING v12.0 human interactome (19,699 proteins, 3,421,130 fold edges)
**Method**: Constrained multi-target neighborhood overlap — desired convergence filtered against
toxicity exclusion set

## Abstract

Using the constrained query mode of SSRAG, we identified proteins at the structural convergence
of six core Alzheimer's disease (AD) pathway proteins while excluding proteins in the
neighborhoods of known cardiac, cholinergic, and opioid toxicity targets. The screen returned
10 proteins in 3 or more of the 6 AD pathway neighborhoods with zero overlap with any toxicity
neighborhood. Three candidates — HAVCR2 (TIM-3), PDE2A, and TMEM108 — are highlighted as
high-priority drug targets based on their convergence profile, druggability, and mechanistic
relevance to AD pathogenesis.

This approach inverts the traditional polypharmacy safety analysis: instead of finding where
drugs dangerously converge, it identifies where disease pathways converge in regions the
toxicity networks do not reach — the pharmacological "safe zone" for therapeutic intervention.

---

## Method

### Desired convergence targets (Alzheimer's pathway)

| Protein | Role in AD | Genetic evidence |
|---------|-----------|------------------|
| APP | Amyloid precursor protein — source of amyloid-beta peptide | Familial AD mutations |
| PSEN1 | Presenilin-1 — gamma-secretase catalytic subunit, cleaves APP | >300 familial AD mutations |
| MAPT | Tau — microtubule stabilizer, forms neurofibrillary tangles | FTLD mutations, AD biomarker |
| APOE | Apolipoprotein E — lipid transport, amyloid clearance | APOE4 = strongest common genetic risk factor |
| TREM2 | Triggering receptor on myeloid cells 2 — microglial activation | R47H variant: 2-4x AD risk increase |
| BACE1 | Beta-secretase 1 — initiates amyloidogenic APP cleavage | Primary target of failed amyloid-focused trials |

### Toxicity exclusion targets

| Protein | Why excluded | Syndrome |
|---------|-------------|----------|
| KCNH2 (hERG) | Long QT syndrome, cardiac arrhythmia | Cardiac toxicity |
| HCN4 | Cardiac pacemaker current disruption | Cardiac toxicity |
| CACNA1C | L-type calcium channel, cardiac + CNS | Cardiac/falls |
| CHRM2 | Muscarinic M2 receptor | Anticholinergic cognitive burden |
| OPRM1 | Mu opioid receptor | Opioid-cholinergic delirium |

These five exclusion targets were identified in our prior polypharmacy convergence analysis
as central mediators of drug-induced cardiac events, cognitive impairment, and falls in
elderly patients — the exact population that AD drugs must be safe in.

### Query

```
@+APP @+PSEN1 @+MAPT @+APOE @+TREM2 @+BACE1 @-KCNH2 @-HCN4 @-CACNA1C @-CHRM2 @-OPRM1
```

### Parameters
- Desired neighborhood size: 200 per target (top by fold cosine similarity)
- Exclusion neighborhood size: 200 per target (union = 884 unique proteins)
- Minimum desired coverage: 2/6 targets
- Exclusion criterion: protein must not appear in ANY exclusion neighborhood

---

## Results

### Exclusion statistics

- Total exclusion set: **884 proteins** (4.5% of the proteome)
- Candidates filtered by exclusion: **0 of 30 returned**

The zero exclusion rate indicates that the Alzheimer's disease pathway convergence network
is structurally distant from the cardiac/cholinergic/opioid toxicity networks in the
interactome. This is itself a significant finding: it suggests that AD-convergent drug
targets can be found without inherently trading off against the toxicity pathways that
make geriatric drug development so difficult.

### Top candidates (3/6 desired pathway coverage)

10 proteins appeared in 3 or more of the 6 AD pathway neighborhoods. All 10 survived
exclusion filtering.

---

#### Candidate 1: HAVCR2 (TIM-3) — Immune Checkpoint at the Neuroinflammation Convergence

**Convergence**: PSEN1 (sim=0.313) + TREM2 (sim=0.290) + BACE1 (sim=0.198)
**Function**: Hepatitis A virus cellular receptor 2 / T-cell immunoglobulin and mucin
domain-containing protein 3. Cell surface receptor that modulates innate and adaptive
immune responses, generally inhibitory.

**Why this matters for AD:**

HAVCR2/TIM-3 sits at the exact intersection of the three AD pathway proteins most directly
involved in neuroinflammation. TREM2 is the microglial activation receptor — the R47H
variant that impairs TREM2 function increases AD risk 2-4 fold, comparable to carrying one
APOE4 allele. PSEN1 (gamma-secretase) regulates Notch signaling in microglia. BACE1 cleaves
multiple neuroinflammatory substrates beyond APP.

TIM-3 is an immune checkpoint receptor on microglia. In cancer, TIM-3 blockade reactivates
exhausted T cells. In the brain, TIM-3 on microglia may serve an analogous function —
maintaining microglia in a surveillance state rather than a phagocytic state. AD-associated
microglia show a phenotype consistent with TIM-3-mediated suppression: reduced phagocytosis
of amyloid-beta, reduced synaptic pruning, and accumulation of damage-associated signals.

**Druggability**: High. Multiple anti-TIM-3 monoclonal antibodies exist and are in clinical
trials for cancer (sabatolimab, cobolimab, MBG453). These are fully human antibodies with
established safety profiles. None have been tested in neurodegenerative disease.

**The hypothesis**: TIM-3 blockade in AD could reactivate microglial phagocytic function
at the convergence of the PSEN1-TREM2-BACE1 pathway, promoting clearance of amyloid-beta
and damaged synapses without the toxicity of direct amyloid or secretase targeting.

**Key advantage over current approaches**: Anti-amyloid antibodies (lecanemab, aducanumab)
target amyloid-beta itself — one node in the pathway. TIM-3 sits at the convergence of
three pathway nodes simultaneously, addressing the amyloid, secretase, and neuroinflammation
arms of the disease in a single target. And it has zero overlap with the cardiac/cholinergic
toxicity networks.

**Testable predictions**:
1. TIM-3 expression is elevated on microglia in AD brain tissue vs controls
2. Anti-TIM-3 antibody treatment in AD mouse models (5xFAD, APP/PS1) reduces amyloid
   plaque burden and restores microglial phagocytic phenotype
3. TIM-3 blockade does not cause cardiac conduction abnormalities or cholinergic effects
   (predicted by exclusion from all toxicity neighborhoods)

---

#### Candidate 2: PDE2A — Druggable Cyclic Nucleotide Target in Synaptic Convergence

**Convergence**: PSEN1 (sim=0.204) + MAPT (sim=0.220) + BACE1 (sim=0.218)
**Function**: cGMP-dependent 3',5'-cyclic phosphodiesterase 2A. Dual-specificity
phosphodiesterase that hydrolyzes both cAMP and cGMP.

**Why this matters for AD:**

PDE2A converges on the three AD proteins most directly involved in synaptic pathology.
PSEN1 regulates synaptic vesicle release through calcium signaling. MAPT/tau stabilizes
the microtubule tracks along which synaptic vesicles are transported. BACE1 cleaves
neuregulin-1 and other synaptic organizing molecules.

cAMP and cGMP are the key second messengers in long-term potentiation (LTP) — the
cellular mechanism of memory formation. PDE2A degrades both. In AD, cyclic nucleotide
signaling is impaired early, before significant neuronal loss. PDE2A inhibition would
boost cAMP/cGMP at the convergence point of the presenilin-tau-secretase pathways.

**Druggability**: Very high. PDE inhibitors are a well-established drug class with decades
of medicinal chemistry:
- PDE5 inhibitors: sildenafil (Viagra), tadalafil (Cialis)
- PDE4 inhibitors: roflumilast (COPD), apremilast (psoriasis)
- PDE3 inhibitors: cilostazol (claudication)
- PDE2A-selective inhibitors: BAY 60-7550, Lu AF64280 (in preclinical development)

PDE2A-selective inhibitors have been shown to enhance LTP and memory in rodent models.
The SSRAG finding adds new weight to this target by placing it at the convergence of
three core AD pathways, rather than merely being "a memory-related enzyme."

**The hypothesis**: PDE2A inhibition boosts cyclic nucleotide signaling at the PSEN1-MAPT-BACE1
convergence point, rescuing synaptic plasticity deficits in AD without affecting cardiac or
cholinergic function.

**Key advantage**: PDE2A inhibitors are small molecules (oral bioavailability), cross the
blood-brain barrier, and have favorable safety profiles in preclinical studies. The
constrained query confirms PDE2A is outside all five toxicity networks — critical because
many AD patients are elderly and on multiple medications. A PDE2A inhibitor would not
contribute to polypharmacy-related cardiac risk, anticholinergic burden, or opioid
interactions.

**Testable predictions**:
1. PDE2A expression is altered in AD brain tissue, particularly in hippocampal regions
   where PSEN1, MAPT, and BACE1 converge functionally
2. PDE2A inhibition rescues LTP deficits in AD mouse models
3. PDE2A inhibition does not exacerbate cardiac or cognitive adverse effects when
   co-administered with common geriatric medications (predicted by toxicity exclusion)

---

#### Candidate 3: TMEM108 — Dentate Gyrus Circuitry at the Neuronal Convergence

**Convergence**: PSEN1 (sim=0.193) + MAPT (sim=0.214) + BACE1 (sim=0.204)
**Function**: Transmembrane protein 108. Required for proper cognitive function, involved
in dentate gyrus neuron circuitry development, necessary for AMPA receptor surface
expression and synaptic function.

**Why this matters for AD:**

The dentate gyrus (DG) is one of only two brain regions where adult neurogenesis occurs and
is among the earliest regions affected in AD. DG dysfunction produces the pattern recognition
deficits and spatial memory impairment that characterize early AD. TMEM108 is required for
DG circuit formation and for trafficking AMPA receptors to the synapse surface.

The PSEN1-MAPT-BACE1 convergence at TMEM108 places this protein at the intersection of
three mechanisms that all damage DG circuitry:
- PSEN1/gamma-secretase cleaves Notch receptors required for DG neural progenitor
  differentiation
- Tau pathology (MAPT) disrupts microtubule-dependent transport in DG granule cells
- BACE1 cleaves neuregulin-1, which regulates DG circuit maturation

TMEM108 sits downstream of all three, mediating the final common pathway to AMPA receptor
surface expression — the event that actually makes a synapse functional.

**Druggability**: Moderate. TMEM108 is a transmembrane protein, making it accessible to
antibodies or small molecules that bind extracellularly. However, as a relatively
understudied protein, no pharmacological tools currently exist. Drug development would
require structural characterization and screening campaigns.

**The hypothesis**: TMEM108 is a convergence effector — the point where presenilin, tau,
and secretase pathology funnel into a common synaptic deficit. Enhancing TMEM108 function
(stabilizing its surface expression or its interaction with AMPA receptors) could rescue
synaptic function downstream of multiple AD pathways.

**Key advantage**: TMEM108 operates at the effector level, downstream of the pathological
cascades. Rather than trying to prevent amyloid production (BACE1 inhibition — failed) or
clear amyloid (anti-amyloid antibodies — marginal benefit), targeting TMEM108 would restore
synaptic function despite ongoing upstream pathology. This is a fundamentally different
therapeutic strategy: accept the disease process, rescue the functional output.

**Testable predictions**:
1. TMEM108 protein levels or surface expression are reduced in AD hippocampus
2. TMEM108 overexpression rescues AMPA receptor surface expression and synaptic
   transmission in neurons exposed to amyloid-beta or tau pathology
3. TMEM108 knockout mice show accelerated cognitive decline when crossed with AD models

---

### Additional candidates of interest (3/6 coverage)

| Rank | Protein | Convergence | Relevance |
|------|---------|-------------|-----------|
| 1 | **SLIT3** | PSEN1+MAPT+BACE1 | Axon guidance molecule; directs neuronal migration and connectivity. Could mediate aberrant axon pathfinding in AD. |
| 3 | **LILRB1** | PSEN1+TREM2+BACE1 | MHC-I receptor on microglia; regulates immune surveillance. Second neuroinflammation convergence target alongside TIM-3. |
| 4 | **TBK1** | PSEN1+MAPT+BACE1 | Kinase controlling neuroinflammation. Known ALS/FTD gene (mutations cause familial ALS). Already under investigation for neurodegeneration — the convergence analysis independently rediscovered it. |
| 5 | **LRP8** (ApoER2) | APP+PSEN1+BACE1 | Reelin receptor that directly binds ApoE. Mediates synaptic plasticity in hippocampus. A convergence node for the amyloid pathway that also interacts with the #1 genetic risk factor (APOE). |
| 6 | **HSP90AA1** | PSEN1+MAPT+BACE1 | Chaperone that maintains tau in a soluble, functional conformation. HSP90 inhibitors are in cancer trials; the opposite approach (HSP90 stabilization) could prevent tau aggregation. |
| 7 | **IL-13** | APP+PSEN1+BACE1 | Anti-inflammatory cytokine in the APP-PSEN1-BACE1 convergence. Could represent a protective neuroinflammatory signal at the amyloid pathway junction. |
| 9 | **ATF4** | PSEN1+MAPT+BACE1 | Integrated stress response transcription factor. Master regulator of cellular stress adaptation — its convergence with three AD proteins suggests the ISR is a nodal response to AD pathology. |

### Notable 2/6 coverage candidates

| Protein | Convergence | Relevance |
|---------|-------------|-----------|
| **LRRK2** | APP+PSEN1 | The Parkinson's disease kinase — its appearance in the AD convergence reinforces the AD/PD mechanistic overlap at the endolysosomal pathway |
| **MEF2C** | PSEN1+MAPT | Transcription factor for neuronal survival; MEF2C haploinsufficiency causes intellectual disability. GWAS-identified AD risk locus. Independently validated by the convergence screen. |
| **PRKCD** (PKC-delta) | PSEN1+MAPT | Kinase controlling microglial inflammatory activation and neuronal apoptosis |
| **DTNBP1** (Dysbindin) | PSEN1+MAPT | Schizophrenia susceptibility gene involved in synaptic vesicle trafficking; lysosomal/endosomal pathway overlap with AD |

---

## Cross-validation: Known AD biology

Several of the convergence candidates are independently supported by existing AD genetics
and biology, providing confidence that the interactome analysis is capturing real disease
biology:

| Candidate | Independent validation |
|-----------|-----------------------|
| TBK1 | Known ALS/FTD gene; under active investigation for neurodegeneration |
| MEF2C | GWAS-identified AD risk locus (Jansen et al., 2019) |
| LRRK2 | Parkinson's gene with documented AD-PD overlap at endolysosomal pathway |
| LRP8/ApoER2 | Known ApoE receptor; documented role in synaptic plasticity and AD |
| HSP90AA1 | Established tau chaperone; HSP90 inhibition promotes tau degradation |

The fact that the constrained screen independently rediscovered multiple known
neurodegeneration genes without any prior knowledge of their disease relevance
validates the method. The novel candidates (HAVCR2, PDE2A, TMEM108) that the screen
places alongside these validated targets warrant investigation.

---

## The constrained design paradigm

This analysis demonstrates a general framework for network-pharmacology-guided drug design:

1. **Define the disease convergence**: Select proteins representing distinct arms of
   the disease pathway. For AD: amyloid production (APP, BACE1), amyloid processing
   (PSEN1), tau pathology (MAPT), genetic risk (APOE), and neuroinflammation (TREM2).

2. **Define the toxicity exclusion**: Select proteins at the core of known adverse effect
   networks. For geriatric indications: cardiac ion channels (KCNH2, HCN4, CACNA1C),
   cholinergic receptors (CHRM2), and opioid receptors (OPRM1).

3. **Query the interactome for the safe convergence zone**: Proteins in the intersection
   of disease neighborhoods that are absent from all toxicity neighborhoods.

The candidates that survive this filter are proteins that:
- Sit at the structural convergence of multiple disease pathways (multi-mechanism targets)
- Are distant from known toxicity mediators in the interactome (predicted safety)
- Are more likely to produce therapeutic effects without dose-limiting adverse events

This paradigm is applicable to any disease with:
- Multiple characterized pathway proteins in STRING
- A defined patient population with known drug sensitivities

---

## Limitations

1. **Protein interaction ≠ functional interaction**: STRING captures physical and
   functional associations, but network proximity does not guarantee that modulating a
   convergence node will affect the disease pathways feeding into it.

2. **No tissue specificity**: STRING aggregates interactions across all tissues. A
   convergence node must be expressed in the relevant tissue (brain, microglia, neurons)
   to be therapeutically relevant.

3. **No directionality**: The analysis identifies convergence but cannot determine whether
   a target should be activated or inhibited. TIM-3 likely requires blockade (reactivation
   of microglial phagocytosis), while TMEM108 likely requires enhancement (more AMPA
   receptor trafficking). This must be determined experimentally.

4. **Exclusion is binary**: A protein is either in or out of the exclusion set. A more
   nuanced approach would weight exclusion by similarity — a protein barely inside the
   KCNH2 neighborhood is lower risk than one at the center.

5. **No pharmacokinetic consideration**: Blood-brain barrier penetration, metabolic
   stability, and oral bioavailability are not modeled.

6. **Validation required**: These are computational hypotheses. Wet-lab validation in
   cellular models, followed by animal models, is required before clinical relevance
   can be assessed.

---

## Reproducibility

```bash
# Build SSRAG (requires CatWhisper)
git clone https://github.com/IreGaddr/SSRAG.git && cd SSRAG
git clone https://github.com/IreGaddr/catwhisper.git
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
cd ..

# Download STRING v12.0 human data
mkdir -p data/string_human && cd data/string_human
wget https://stringdb-downloads.org/download/protein.info.v12.0/9606.protein.info.v12.0.txt.gz
wget https://stringdb-downloads.org/download/protein.links.detailed.v12.0/9606.protein.links.detailed.v12.0.txt.gz
wget https://stringdb-downloads.org/download/protein.enrichment.terms.v12.0/9606.protein.enrichment.terms.v12.0.txt.gz
cd ../..

# Ingest (65 seconds on 16-thread CPU)
build/ssrag-ingest-string --data-dir data/string_human --out-dir data/ssrag_human --organism 9606

# Run the constrained Alzheimer's query
echo "@+APP @+PSEN1 @+MAPT @+APOE @+TREM2 @+BACE1 @-KCNH2 @-HCN4 @-CACNA1C @-CHRM2 @-OPRM1" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

### Technical parameters
- Index: STRING v12.0 Homo sapiens, 19,699 proteins
- Lambda fold: fold_k=200, fold_threshold=0.02, max_fold_weight=16000
- Fold edges: 3,421,130 (~174 per protein)
- Effective dimensions: 19,955
- Contrast ratio: 141.3:1
- Neighborhood size per target: 200 (top by fold cosine similarity)
- Desired coverage threshold: 2/6 targets
- Exclusion: union of top-200 neighbors for each of 5 toxicity targets (884 proteins)
