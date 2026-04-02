# Polypharmacy Convergence Analysis: Beyond Cardiac Risk

**Date**: 2026-04-01
**System**: SSRAG v0.1.0, STRING v12.0 human interactome (19,699 proteins, 3,421,130 fold edges)
**Method**: Multi-target neighborhood overlap via fold-dimension cosine similarity

## Abstract

Following our initial cardiac convergence analysis (see `polypharmacy-convergence-nodes.md`),
we extended the interactome overlap approach to four additional polypharmacy syndromes common
in elderly patients: nephrotoxicity ("triple whammy"), GI bleeding, anticholinergic cognitive
burden, and falls/fracture risk. The analysis reveals that these syndromes differ fundamentally
in their network architecture. Nephrotoxicity and GI bleeding show sparse or absent protein-level
convergence, suggesting their mechanisms are primarily hemodynamic or additively independent.
In contrast, anticholinergic cognitive burden and falls/fracture risk show dense convergence
networks with novel mediator proteins — most notably the HRH3 histamine autoreceptor as a
feedback-loop disruptor in anticholinergic burden, and a striking K2P potassium channel family
convergence (KCNK1/3/5/6/13/18) as the potential molecular basis for polypharmacy-induced falls.

---

## 1. Nephrotoxicity: The "Triple Whammy"

### Clinical context
The combination of an ACE inhibitor, NSAID, and diuretic is the most well-characterized
nephrotoxic drug combination in geriatric medicine. It causes acute kidney injury through
disruption of renal hemodynamic autoregulation: ACE inhibitors dilate the efferent arteriole,
NSAIDs constrict the afferent arteriole, and diuretics reduce intravascular volume. The
question: does the protein interactome reveal additional convergence beyond hemodynamics?

### Drug targets queried
| Drug class | Target protein | Role |
|------------|---------------|------|
| ACE inhibitor | ACE | Angiotensin-converting enzyme |
| NSAID | PTGS1 (COX-1) | Constitutive prostaglandin synthesis |
| NSAID | PTGS2 (COX-2) | Inducible prostaglandin synthesis |
| Thiazide diuretic | SLC12A3 | Sodium-chloride cotransporter |

### Results: Sparse convergence (7 proteins in 2/4 neighborhoods)

No protein appeared in more than 2 of the 4 target neighborhoods. The overlap is dominated
by PTGS1-PTGS2 co-neighbors (expected — both are COX enzymes with shared interactors).

**Notable convergence nodes:**

#### ALOX15 — Arachidonate 15-lipoxygenase
- **Neighborhoods**: ACE (sim=0.350) + PTGS1 (sim=0.216)
- **Function**: Catalyzes stereo-specific peroxidation of polyunsaturated fatty acids,
  producing lipoxins and other specialized pro-resolving mediators (SPMs)
- **Significance**: ALOX15-derived lipoxins are renal protective — they promote resolution
  of inflammation and protect against ischemia-reperfusion injury in the kidney. The
  convergence of ACE and COX-1 pathways at ALOX15 suggests a mechanism beyond hemodynamics:
  ACE inhibitors and NSAIDs may simultaneously suppress distinct inputs to the ALOX15
  lipoxin synthesis pathway, eliminating renal-protective lipid mediators from two
  directions simultaneously. This "resolution failure" model of triple-whammy nephrotoxicity
  has not been well-characterized.

#### HSPA5 (BiP/GRP78) — ER stress chaperone
- **Neighborhoods**: ACE (sim=0.193) + PTGS2 (sim=0.315)
- **Function**: Master regulator of the unfolded protein response in the endoplasmic reticulum
- **Significance**: ER stress in renal tubular epithelial cells is a documented pathway to
  drug-induced acute kidney injury. The convergence of ACE and COX-2 pathways at HSPA5
  suggests that both drug classes may independently promote ER stress in renal cells, with
  their combination overwhelming the protective unfolded protein response.

### Interpretation

The sparse convergence is itself informative. The triple whammy is primarily a **hemodynamic**
phenomenon — it operates through vascular tone regulation at the organ level, not through
shared protein-protein interaction networks. The STRING interactome captures molecular
interactions within and between cells, but cannot model arteriolar tone balance, glomerular
filtration pressure, or intravascular volume — the actual mechanisms of triple-whammy
nephrotoxicity. The ALOX15 and HSPA5 findings suggest modest secondary molecular contributions,
but the network architecture confirms that this syndrome is not protein-convergence-driven.

---

## 2. GI Bleeding: SSRI + NSAID + Anticoagulant

### Clinical context
SSRIs deplete platelet serotonin stores (reducing aggregation), NSAIDs inhibit
thromboxane-dependent platelet aggregation and damage gastric mucosa, and anticoagulants
impair the coagulation cascade. The combination produces GI bleeding at rates 4-12x above
any single drug class. Is this convergent or additive?

### Drug targets queried
| Drug class | Target protein | Role |
|------------|---------------|------|
| SSRI | SLC6A4 | Serotonin transporter (platelet uptake) |
| NSAID | PTGS1 (COX-1) | Thromboxane A2 synthesis |
| NSAID | PTGS2 (COX-2) | Inducible prostaglandin synthesis |
| Anticoagulant (warfarin) | VKORC1 | Vitamin K epoxide reductase |

### Results: Minimal convergence (3 proteins, all PTGS1-PTGS2 co-neighbors)

The three overlapping proteins (ALOX5, RAP1A, CASP7) are all shared between PTGS1 and
PTGS2 only — the two COX isoforms that share known interactors. There is **zero convergence**
between the serotonin transporter (SLC6A4), the COX enzymes, and the vitamin K system (VKORC1).

### Interpretation

This result strongly supports the **additively independent** model of SSRI+NSAID+anticoagulant
GI bleeding. Each drug class impairs hemostasis through a completely separate molecular
pathway:
- SSRIs: platelet serotonin depletion → reduced dense granule secretion
- NSAIDs: thromboxane A2 inhibition → reduced platelet aggregation + mucosal damage
- Anticoagulants: coagulation factor depletion → impaired clot formation

The absence of network convergence means there is no shared protein mediator that could
be targeted to mitigate the combined risk. The risk is genuinely additive — three independent
hits to hemostasis that happen to compound. Clinical management must address each pathway
separately.

This is a negative result that carries clinical information: combination risk calculators
for SSRI+NSAID+anticoagulant bleeding can justifiably use additive risk models rather than
needing to account for synergistic interactions.

---

## 3. Anticholinergic Cognitive Burden

### Clinical context
The cumulative anticholinergic burden from multiple medications is a major driver of
cognitive decline, delirium, and dementia acceleration in elderly patients. Many drugs have
"hidden" anticholinergic activity not listed in their primary mechanism — certain
antidepressants, antihistamines, bladder medications, and antipsychotics all contribute.
The Anticholinergic Cognitive Burden (ACB) scale scores individual drugs, but the molecular
basis for why cumulative burden is worse than any single drug is poorly understood.

### Drug targets queried
| Drug class | Target protein | Role |
|------------|---------------|------|
| Anticholinergic (various) | CHRM1 | Muscarinic M1 receptor (cortical cognition) |
| Anticholinergic (various) | CHRM2 | Muscarinic M2 receptor (cardiac/presynaptic) |
| Bladder anticholinergic | CHRM3 | Muscarinic M3 receptor (smooth muscle/glands) |
| Antihistamine | HRH1 | Histamine H1 receptor |
| Cognitive target | ACHE | Acetylcholinesterase |

### Results: Dense convergence (30 proteins in 2+ neighborhoods, 18 in 3/5)

This is the densest convergence network of any syndrome analyzed. 18 proteins appear in
3 or more of the 5 target neighborhoods. The convergence is concentrated in the G-protein
coupled receptor (GPCR) superfamily, revealing a hidden cross-talk network between
muscarinic and histaminergic signaling.

**Major convergence nodes:**

#### Finding 1: CHRM5 — The Unmonitored Muscarinic Receptor
- **Neighborhoods**: CHRM1 + CHRM2 + HRH1 (3/5, avg_sim=0.460)
- **Function**: Muscarinic M5 receptor, expressed in hippocampus, VTA, and substantia nigra
- **Significance**: M5 is the least-studied muscarinic receptor and is not included in
  standard anticholinergic burden scoring. It is the only muscarinic subtype that shows
  convergent modulation from both the muscarinic AND histaminergic pathways in the
  interactome. M5 knockout mice show impaired hippocampal plasticity and reward learning.
  If drugs with "incidental" anticholinergic or antihistaminergic activity are suppressing
  M5 through network effects, this receptor may be a critical unmonitored mediator of
  anticholinergic cognitive decline. **The ACB scale may need a hidden M5 component.**

#### Finding 2: HRH3 — Feedback Loop Disruption
- **Neighborhoods**: CHRM1 + CHRM2 + HRH1 (3/5, avg_sim=0.314)
- **Function**: Histamine H3 receptor — presynaptic autoreceptor controlling release of
  histamine, acetylcholine, norepinephrine, serotonin, and dopamine in the CNS
- **Significance**: This is potentially the most important finding in this analysis. HRH3
  is the brain's master volume knob for multiple neurotransmitter systems. It serves as a
  negative feedback autoreceptor: when histamine levels are high, HRH3 activation suppresses
  further release of histamine AND acetylcholine.

  The convergence of muscarinic (CHRM1, CHRM2) and histaminergic (HRH1) pathways at HRH3
  reveals a **feedback catastrophe model** of anticholinergic cognitive burden:

  1. Antihistamines block HRH1 → brain compensates by increasing histamine release
  2. Increased histamine activates HRH3 autoreceptors
  3. HRH3 activation suppresses acetylcholine release (heteroreceptor function)
  4. Reduced acetylcholine release + direct muscarinic blockade from anticholinergic drugs
     → double hit to cholinergic signaling
  5. The compensatory response to HRH1 blockade actively WORSENS the anticholinergic deficit

  This feedback mechanism explains why anticholinergic + antihistaminergic polypharmacy
  produces cognitive effects worse than the sum of individual drug burdens. The brain's
  attempt to compensate for antihistamine effects inadvertently amplifies anticholinergic
  toxicity through HRH3-mediated cholinergic suppression.

  **Clinical implication**: HRH3 inverse agonists (pitolisant, approved for narcolepsy)
  could theoretically be protective against anticholinergic cognitive burden by blocking
  the feedback-mediated cholinergic suppression. This is a testable pharmacological hypothesis.

#### Finding 3: HTR1D — Serotonin-Cholinergic Cross-Talk
- **Neighborhoods**: CHRM1 + CHRM2 + HRH1 (3/5, avg_sim=0.392)
- **Function**: Serotonin 5-HT1D receptor, involved in cortical and trigeminal signaling
- **Significance**: The appearance of a serotonin receptor in the anticholinergic-
  antihistaminergic convergence network reveals unexpected cross-modality signaling. 5-HT1D
  receptors modulate acetylcholine release in the cortex. If anticholinergic drugs are
  suppressing 5-HT1D through network effects, this would produce combined cholinergic AND
  serotonergic deficits, potentially explaining why high anticholinergic burden correlates
  with both cognitive decline and mood disturbances (apathy, depression) in elderly patients.

#### Finding 4: OPRM1 — Opioid-Cholinergic Gateway
- **Neighborhoods**: CHRM2 + HRH1 (2/5, avg_sim=0.414)
- **Function**: Mu-type opioid receptor
- **Significance**: The mu opioid receptor appears in both muscarinic M2 and histamine H1
  neighborhoods. This convergence provides a molecular basis for the well-documented but
  poorly understood potentiation between anticholinergic drugs and opioids in causing
  delirium. The protein interaction network physically links the cholinergic, histaminergic,
  and opioid signaling systems. Patients on anticholinergic medications who receive opioids
  (common post-surgically in elderly) may experience amplified cognitive effects because the
  drugs converge through shared protein mediators, not merely through additive CNS depression.

#### Finding 5: Serotonin receptor cluster (HTR1A, HTR1E, HTR1F, HTR4, HTR7)
- **Neighborhoods**: Various 3/5 and 2/5 overlaps
- Five different serotonin receptor subtypes appear in the anticholinergic convergence
  network. This wholesale appearance of the serotonergic system in a muscarinic-histaminergic
  query was unexpected and suggests that anticholinergic burden may produce broader
  neurotransmitter disruption than currently modeled. The ACB scale treats anticholinergic
  burden as a purely cholinergic phenomenon — the interactome reveals it simultaneously
  affects serotonergic, histaminergic, and purinergic signaling.

### Interpretation

The anticholinergic convergence network is the densest observed across all polypharmacy
syndromes analyzed. This density is partially expected (GPCRs form a highly interconnected
superfamily) but the specific convergence nodes — HRH3, HTR1D, OPRM1 — provide mechanistic
explanations for clinically observed phenomena that lack current molecular models:

1. **Why cumulative anticholinergic burden is super-additive**: HRH3 feedback loop disruption
2. **Why anticholinergic drugs cause mood changes**: Serotonin receptor convergence
3. **Why opioid + anticholinergic delirium is synergistic**: OPRM1 convergence
4. **Why ACB scoring underestimates risk**: CHRM5 is unmonitored

---

## 4. Falls and Fracture Risk

### Clinical context
Falls are the leading cause of injury death in adults over 65. Polypharmacy is the strongest
modifiable risk factor, but the molecular mechanism linking medication count to fall risk
is poorly characterized. The standard model is additive CNS depression — more sedating drugs
means more drowsiness. Our analysis reveals a far more specific mechanism: convergent
modulation of the K2P (two-pore-domain potassium) channel family that controls neuronal
resting membrane potential and vestibular/proprioceptive function.

### Drug targets queried
| Drug class | Target protein | Role |
|------------|---------------|------|
| Benzodiazepine | GABRA1 | GABA-A receptor alpha-1 subunit |
| SSRI | SLC6A4 | Serotonin transporter |
| Beta-blocker | ADRB1 | Beta-1 adrenergic receptor |
| Opioid | OPRM1 | Mu opioid receptor |
| CCB | CACNA1C | L-type calcium channel alpha subunit |

### Results: Dense convergence (30 proteins in 2+ neighborhoods, 3 in 3/5)

The convergence is dominated by ion channels — specifically the GABRA1-CACNA1C axis,
revealing that benzodiazepines and calcium channel blockers converge on a shared network
of neuronal excitability regulators.

**Major convergence nodes:**

#### Finding 1: The K2P Channel Family Convergence
Six members of the two-pore-domain potassium channel family (KCNK) appear in both GABRA1
and CACNA1C neighborhoods:

| Protein | avg_sim | Function |
|---------|---------|----------|
| KCNK6 (TWIK-2) | 0.498 | Outward rectifier, widely expressed |
| KCNK5 (TASK-2) | 0.481 | pH-sensitive, renal/neural expression |
| KCNK18 (TRESK) | 0.477 | Pain pathway, dorsal root ganglia |
| KCNK1 (TWIK-1) | 0.476 | Background K+ in astrocytes, sets resting potential |
| KCNK13 (THIK-1) | 0.476 | Inward rectifier, brain expression |
| KCNK3 (TASK-1) | 0.475 | pH/O2-sensitive, carotid body, adrenal cortex |

**Significance**: K2P channels are the "leak" potassium channels that set the resting
membrane potential in neurons, determining their baseline excitability. They are the
foundation on which all other ion channel activity operates — if the resting potential
shifts, every voltage-gated channel downstream is affected.

The convergence of GABA-A receptors and L-type calcium channels at the entire K2P family
means that benzodiazepines and CCBs are not merely causing independent sedation and
vasodilation — they are jointly modulating the fundamental electrical setpoint of neurons.
A patient on both a benzodiazepine and a CCB has pharmacological pressure on the K2P
channels from two directions simultaneously, producing a shift in resting membrane potential
that makes all neurons less excitable.

This model explains several clinical observations:
- **Why falls increase non-linearly with drug count**: K2P modulation shifts the entire
  excitability landscape, so each additional drug that touches the K2P network doesn't just
  add sedation — it shifts the baseline on which all other drugs operate
- **Why falls happen on standing (orthostatic)**: K2P channels in baroreceptor neurons
  (KCNK3/TASK-1 specifically) regulate the baroreflex — suppression delays the compensatory
  heart rate and vasoconstrictor response to standing
- **Why dose reduction of ANY sedating drug helps**: Relieving K2P pressure from any
  direction partially restores the resting potential setpoint

#### Finding 2: HTR3B — Recurring Ion Channel Convergence Node
- **Neighborhoods**: GABRA1 + SLC6A4 + CACNA1C (3/5, avg_sim=0.380)
- **Function**: Serotonin 3B receptor — ligand-gated cation channel
- HTR3B appeared in our cardiac analysis as a CCB-SSRI convergence node, and it appears
  here again in the falls analysis with the addition of GABRA1 (benzos). HTR3B is a ligand-
  gated ion channel expressed in both the autonomic nervous system (where it regulates
  cardiovascular reflexes) and the vestibular system (where it modulates balance). Its
  recurring appearance across both cardiac and falls analyses suggests it is a central
  mediator of polypharmacy risk that spans multiple organ systems.

  **Testable prediction**: HTR3B antagonists (ondansetron, granisetron — commonly used as
  antiemetics) might have a protective effect against polypharmacy-related falls, by
  preserving vestibular/autonomic function at this convergence point. Ondansetron is already
  safely used in elderly patients. A retrospective study of fall rates in patients on
  polypharmacy who receive vs don't receive ondansetron could test this prediction.

#### Finding 3: HCN2 — Pacemaker Channel in Falls
- **Neighborhoods**: GABRA1 + SLC6A4 + CACNA1C (3/5, avg_sim=0.374)
- **Function**: Hyperpolarization-activated cyclic nucleotide-gated channel 2 — controls
  pacemaker activity in both the heart and rhythmically firing neurons
- HCN2 is in the same family as HCN4 (our cardiac convergence finding). While HCN4 is
  predominantly cardiac, HCN2 is expressed in both the heart and the brain, particularly
  in thalamocortical relay neurons and the brainstem. Convergent modulation of HCN2 by
  benzos, SSRIs, and CCBs could simultaneously:
  - Slow cardiac pacemaker activity → orthostatic bradycardia
  - Disrupt thalamocortical oscillations → impaired arousal/alertness
  - Suppress brainstem postural control circuits → balance impairment

  All three of these are documented clinical predictors of falls, and they share a common
  molecular substrate in HCN2.

#### Finding 4: TRPM2 — Oxidative Stress Ion Channel
- **Neighborhoods**: GABRA1 + OPRM1 + CACNA1C (3/5, avg_sim=0.287)
- **Function**: Transient receptor potential cation channel M2 — calcium-permeable channel
  activated by oxidative stress, ADP-ribose, and intracellular calcium
- **Significance**: TRPM2 mediates calcium influx in response to oxidative stress. In
  neurons, excessive TRPM2 activation leads to excitotoxic cell death. The convergence of
  benzodiazepine, opioid, and calcium channel blocker pathways at TRPM2 reveals a potential
  mechanism for the well-documented association between polypharmacy and accelerated
  neurodegeneration. Chronic convergent modulation of TRPM2 could produce low-grade
  excitotoxicity or impaired oxidative stress responses in neurons, contributing to the
  irreversible component of polypharmacy-related cognitive and motor decline.

  The opioid contribution (OPRM1) is particularly notable. Opioid use disorder is associated
  with white matter degeneration and cerebellar atrophy — TRPM2-mediated calcium dysregulation
  could be the mechanism.

#### Finding 5: KCNE2 — Cardiac-Gastric Dual Risk
- **Neighborhoods**: GABRA1 + CACNA1C (2/5, avg_sim=0.476)
- **Function**: Potassium channel beta subunit that modulates KCNQ1 and KCNH2 (hERG)
- KCNE2 loss-of-function causes both long QT syndrome and gastric acid hypersecretion.
  Its appearance in the falls network connects cardiac arrhythmia risk with the same
  drug combinations that cause falls — these are not independent adverse effects but
  manifestations of the same convergent ion channel disruption.

#### Finding 6: CACNG6 — Calcium Channel Regulatory Subunit
- **Neighborhoods**: GABRA1 + CACNA1C (2/5, avg_sim=0.484)
- **Function**: Voltage-dependent calcium channel gamma-6 subunit, regulates L-type
  calcium channels containing CACNA1C
- CACNG6 is a regulatory subunit of the very calcium channel that CCBs target. Its
  appearance in the GABRA1 (benzodiazepine) neighborhood means that GABA-A receptor
  signaling influences L-type calcium channel regulation through CACNG6. The channel
  that CCBs are designed to block is ALSO being modulated by the benzodiazepine-GABA
  pathway — pharmacological synergy through shared channel subunit regulation.

### Interpretation

The falls/fracture analysis reveals the most mechanistically coherent convergence network
in this study. The K2P channel family convergence provides a unified molecular model for
polypharmacy-induced falls that goes beyond "additive CNS depression":

**The K2P Resting Potential Model of Polypharmacy Falls**:

1. K2P (KCNK) channels set the resting membrane potential in neurons throughout the
   vestibular system, baroreceptor circuits, and postural control pathways
2. Benzodiazepines modulate K2P channels through the GABRA1 interaction network
3. CCBs modulate K2P channels through the CACNA1C interaction network
4. The combined effect shifts resting membrane potential in baroreceptor and vestibular
   neurons, raising the threshold for activation
5. On standing, the delayed baroreceptor response (via KCNK3/TASK-1) causes orthostatic
   hypotension, while impaired vestibular signaling (via KCNK18/TRESK) disrupts balance
6. Additional drugs (SSRIs, opioids) that converge at HTR3B, HCN2, or TRPM2 further
   destabilize this already-shifted baseline

This model makes specific testable predictions:
- K2P channel activators (e.g., riluzole activates KCNK3) should be protective against
  polypharmacy falls
- Falls risk should correlate with the number of drugs in a patient's regimen that have
  K2P-interacting targets, not merely with total drug count or sedation score
- KCNK3 (TASK-1) polymorphisms should modify falls susceptibility in polypharmacy patients

---

## Cross-Syndrome Architecture

The four syndromes fall into two distinct architectural categories:

### Category A: Mechanistically Independent (sparse/absent convergence)
- **Nephrotoxicity**: Primarily hemodynamic, not protein-network-mediated
- **GI bleeding**: Three independent hits to hemostasis with no shared mediator

These syndromes are well-modeled by additive risk frameworks. Drug interaction checkers
that flag additive risk are sufficient.

### Category B: Network-Convergent (dense convergence at specific ion channels)
- **Anticholinergic cognitive burden**: GPCR feedback network, HRH3 feedback disruption
- **Falls/fracture risk**: K2P channel family, resting membrane potential disruption

These syndromes require network-aware risk models. Individual drug risk scores (ACB scale,
sedation scores) underestimate combined risk because they do not capture convergent
modulation of shared protein targets. The interactome analysis reveals which specific proteins
mediate the super-additive risk.

### Recurring convergence nodes across analyses

| Protein | Appeared in | Role |
|---------|-------------|------|
| HTR3B | Cardiac + Falls | Ligand-gated ion channel, autonomic/vestibular |
| HCN2/HCN4 | Cardiac + Falls | Pacemaker current, heart and brain |
| KCNE2 | Cardiac + Falls | K+ channel subunit, cardiac + gastric |
| OPRM1 | Anticholinergic + Falls | Opioid-cholinergic-histaminergic gateway |
| CHRM5 | Anticholinergic | Unmonitored muscarinic receptor |
| HRH3 | Anticholinergic | Feedback autoreceptor |

The recurrence of HTR3B, HCN2, and KCNE2 across both cardiac and falls analyses suggests
these proteins are central hubs of polypharmacy risk, not syndrome-specific mediators. A
patient whose medications converge at these nodes may be simultaneously at elevated risk for
arrhythmia, falls, cognitive decline, and GI complications — different clinical presentations
of the same underlying convergent ion channel disruption.

---

## Methodology

### Query construction
For each syndrome, we identified the primary molecular targets of the causative drug classes
using established pharmacological databases. Each target was queried as a protein in the
SSRAG index, and the multi-target overlap query identified proteins appearing in 2 or more
of the target neighborhoods (top 200 neighbors per target by fold-dimension cosine
similarity).

### Convergence scoring
Proteins are ranked by (1) number of target neighborhoods they appear in, then (2) average
fold cosine similarity across those neighborhoods. Fold cosine similarity measures shared
structural connections in the protein interactome — two proteins are neighbors if they share
many interaction partners, not merely if they directly interact.

### Limitations
1. **STRING is not tissue-specific**: The interactome represents all known human protein
   interactions regardless of tissue. A convergence node may not be co-expressed with the
   relevant drug targets in the tissue of interest.
2. **Functional annotation bias**: Well-studied proteins (GPCRs, ion channels) have more
   known interactions in STRING, potentially inflating their appearance in convergence
   analyses.
3. **No pharmacokinetic modeling**: The analysis identifies network-level convergence
   targets but does not model drug concentration, metabolism, or actual binding affinity
   at convergence nodes.
4. **Fold-dimension cosine similarity measures structural neighborhood overlap**, not
   direct functional interaction. A high similarity score means two proteins share many
   interaction partners, which may indicate functional proximity or may reflect shared
   membership in a large protein complex.

### Reproducibility
All queries can be reproduced with:
```bash
# Example: falls/fracture analysis
echo "@GABRA1 @SLC6A4 @ADRB1 @OPRM1 @CACNA1C" | \
    build/ssrag-query --data-dir data/ssrag_human --k 30
```

Raw query outputs saved in `docs/findings/raw-query-results-extended.txt`.

### Technical parameters
- Index: STRING v12.0 Homo sapiens, 19,699 proteins
- Lambda fold: fold_k=200, fold_threshold=0.02, max_fold_weight=16000
- Fold edges: 3,421,130 (~174 per protein)
- Effective dimensions: 19,955
- Contrast ratio: 141.3:1
- Neighborhood size per target: 200 (top by fold cosine similarity)
- Overlap threshold: 2+ target neighborhoods
