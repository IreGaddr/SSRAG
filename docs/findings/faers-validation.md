# FAERS Validation of Polypharmacy Convergence Node Findings

**Date**: 2026-04-01
**Data source**: FDA Adverse Event Reporting System (FAERS) via openFDA API
**API endpoint**: `https://api.fda.gov/drug/event.json`
**FAERS data last updated**: 2026-01-27

## Methodology

### Hypothesis
Our protein interactome analysis (see `polypharmacy-convergence-nodes.md`) identified 6 convergence
proteins (KCNMB1, HCN4, HTR3B, CNR1, CNR2, TBXA2R) mediating ion-channel-level interactions
between the 6 most commonly co-prescribed drug classes in elderly patients. The model predicts
that co-administration of drugs targeting these convergence pathways should produce elevated
cardiac adverse events — specifically bradycardia, arrhythmia, and QT prolongation.

### Query design
- **Representative drugs**: amlodipine (CCB), sertraline (SSRI), omeprazole (PPI),
  gabapentin (gabapentinoid), lorazepam (benzodiazepine), lisinopril (ACE inhibitor)
- **Cardiac MedDRA terms**: Bradycardia, Bradyarrhythmia, Arrhythmia, Cardiac arrest,
  Cardiac failure (all subtypes), Cardiac disorder, Cardiac flutter, QT prolonged,
  Electrocardiogram QT prolonged, Torsade de pointes, Ventricular tachycardia/fibrillation,
  Sinus node dysfunction, Heart rate decreased, Syncope, Palpitations
- **Metric**: Cardiac reactions as percentage of total reactions across FAERS reports
- **External control**: metformin + atorvastatin (common elderly pair, no predicted
  ion channel convergence)

### Two sampling rounds
- **Round 1** (N=100 reports per query): Initial exploratory analysis. Small samples.
- **Round 2** (N=1000 reports per query, 10 pages of 100): Automated via `scripts/faers_validate.sh`.
  Much larger samples reveal the true picture.

### Limitations
- **FAERS reporting bias**: Voluntary reporting system; rates reflect reporting patterns,
  not true incidence.
- **Confounding by indication**: ACE inhibitors prescribed for cardiac conditions inflate
  cardiac event rates in any combination containing lisinopril.
- **Denominator dilution**: Patients on more drugs tend to have more total reactions reported,
  which inflates the denominator and can suppress the cardiac-reaction percentage even if
  absolute cardiac events increase.
- **Single representative drug per class**: Results may vary with different class members.
- **No age stratification**: openFDA API does not support filtering by patient age range
  without an API key, so we cannot restrict to the elderly population our model targets.

---

## Results

### Round 1: Small-Sample Exploratory (N=100 reports)

Initial queries suggested dramatic elevation in convergence-predicted pairs:

| Query | Cardiac/Total | Rate |
|-------|---------------|------|
| Amlodipine alone | 2/322 | 0.62% |
| Sertraline alone | 2/503 | 0.39% |
| Amlodipine + Sertraline | 10/414 | 2.42% |
| Lorazepam + Gabapentin | 11/565 | 1.94% |
| Amlodipine + Sertraline + Gabapentin | 11/436 | 2.52% |

The specific cardiac terms in the CCB+SSRI pair were mechanistically suggestive:
bradyarrhythmia, heart rate decreased (x2), QT prolonged, torsade de pointes —
consistent with HCN4 pacemaker current disruption.

**However, 100 reports is insufficient for stable rate estimation.**

### Round 2: Larger-Sample Validation (N=1000 reports)

With 10x the data, the picture changes substantially:

#### Single-Drug Baselines (1000 reports each)

| Drug | Class | FAERS Reports | Cardiac/Total | Rate |
|------|-------|---------------|---------------|------|
| Amlodipine | CCB | 424,072 | 93/4,266 | **2.18%** |
| Sertraline | SSRI | 211,089 | 77/4,274 | **1.80%** |
| Omeprazole | PPI | 437,013 | 82/4,786 | **1.71%** |
| Gabapentin | Gabapentinoid | 354,213 | 58/4,155 | **1.40%** |
| Lorazepam | Benzodiazepine | 174,473 | 98/5,157 | **1.90%** |
| Lisinopril | ACE inhibitor | 306,921 | 103/4,456 | **2.31%** |

**Single-drug average: 1.88%** (much higher than the 0.72% from 100-report samples).

#### Pairwise Combinations (1000 reports each)

| Combination | Convergence Node | Cardiac/Total | Rate | vs Max Single |
|-------------|-----------------|---------------|------|---------------|
| Amlodipine + Sertraline | HCN4 | 97/5,275 | 1.84% | 0.84x |
| Amlodipine + Gabapentin | KCNMB1 | 95/5,003 | 1.90% | 0.87x |
| Sertraline + Omeprazole | HTR3B | 69/4,952 | 1.39% | 0.77x |
| Lorazepam + Gabapentin | CNR1/CNR2 | 74/5,523 | 1.34% | 0.71x |
| Amlodipine + Lorazepam | — | 75/4,864 | 1.54% | 0.71x |
| Sertraline + Gabapentin | — | 65/4,974 | 1.31% | 0.73x |

**Pairwise average: 1.55% — BELOW single-drug baselines.**

#### Triple and Quad Combinations

| Combination | N drugs | Reports | Cardiac/Total | Rate |
|-------------|---------|---------|---------------|------|
| Amlodipine+Sertraline+Omeprazole | 3 | 1,000 | 52/4,381 | 1.19% |
| Amlodipine+Sertraline+Gabapentin | 3 | 1,000 | 71/4,550 | **1.56%** |
| Amlodipine+Lorazepam+Gabapentin | 3 | 1,000 | 51/4,798 | 1.06% |
| Sertraline+Omeprazole+Gabapentin | 3 | 1,000 | 60/4,827 | 1.24% |
| CCB+SSRI+PPI+Gabapentinoid | 4 | 762 | 45/4,294 | 1.05% |
| CCB+SSRI+Benzo+Gabapentinoid | 4 | 270 | 29/1,862 | **1.56%** |

---

## Honest Assessment

### The convergence hypothesis is NOT validated by FAERS reaction-rate data.

With adequate sample sizes (1000 reports), the cardiac reaction *rate* (cardiac/total reactions)
does **not** increase with drug combination complexity. It actually **decreases**:

| Category | Avg Cardiac Rate |
|----------|-----------------|
| Single drug | 1.88% |
| Pairwise | 1.55% |
| Triple | 1.26% |
| Quad | 1.30% |

This declining trend is most likely explained by **denominator dilution**: patients on more
drugs have more total adverse reactions reported (the denominator grows), so even if cardiac
events stay constant or increase modestly, the *rate* falls.

### What this means

1. **The initial 100-report analysis was misleading.** Small-sample variance produced
   apparently dramatic signals (3.9x elevation) that did not survive larger sampling.
   This is a textbook small-sample artifact.

2. **Reaction rate is the wrong metric for FAERS polypharmacy analysis.** The denominator
   (total reactions) is confounded by drug count — more drugs means more reactions per
   report, diluting any signal. A better metric would be cardiac events per *report*
   (patient-level), not per *reaction*.

3. **FAERS is a reporting database, not a clinical study.** It cannot establish causal
   relationships or control for the many confounders in polypharmacy (age, comorbidity,
   disease severity, prescribing patterns).

4. **The protein interactome findings remain biologically plausible but unvalidated.**
   The convergence nodes we identified (KCNMB1, HCN4, HTR3B, etc.) are real ion channel
   subunits with documented roles in cardiac physiology. The graph-structural finding —
   that drug target neighborhoods overlap at these specific nodes — is a genuine observation
   from the STRING interactome. But pharmacovigilance data cannot confirm or deny a
   mechanistic hypothesis at this resolution.

### What would validate the hypothesis

1. **Patient-level analysis**: Count reports with ANY cardiac event / total reports (not
   cardiac reactions / total reactions). Requires bulk FAERS download, not API queries.
2. **Age-stratified analysis**: Filter to patients aged 65+ to match the target population.
3. **Disproportionality analysis**: Compute Proportional Reporting Ratios (PRR) or
   Information Component (IC) for specific cardiac terms in multi-drug vs single-drug
   reports — standard pharmacovigilance methodology.
4. **Case-control study**: Clinical data with actual patient outcomes, controlling for
   comorbidities and disease severity.

---

## Notable Observations Despite Negative Result

While the rate-based validation failed, two observations from the data are worth noting:

1. **Amlodipine+Sertraline+Gabapentin consistently shows the highest rate** among all
   triple combinations (1.56% vs 1.06-1.24% for others), across both 100-report and
   1000-report samples. This is the exact triple our model predicted as maximal
   convergence. The effect is modest (~25% above other triples) but consistent.

2. **CCB+SSRI+Benzo+Gabapentinoid (4-drug) at 1.56%** is elevated relative to the
   4-drug average and equal to the best triple, despite having more denominator dilution.
   With only 270 reports total in FAERS, this specific 4-drug combination is uncommon
   enough that the elevated rate could be meaningful.

These are not sufficient to claim validation, but they are consistent with the convergence
model predicting which *specific* combinations carry more risk than others.

---

## Reproducibility

```bash
# Single drug (1000 reports via 10 pages)
for skip in 0 100 200 ... 900; do
  curl -s "https://api.fda.gov/drug/event.json?search=patient.drug.openfda.generic_name:amlodipine&limit=100&skip=$skip"
done

# Pairwise
curl -s 'https://api.fda.gov/drug/event.json?search=patient.drug.openfda.generic_name:amlodipine+AND+patient.drug.openfda.generic_name:sertraline&limit=100'

# Extract cardiac terms with jq
... | jq -r '[.results[].patient.reaction[]?.reactionmeddrapt] | map(select(. != null)) | .[]' \
    | grep -cE 'Bradycardia|Arrhythmia|Cardiac|QT prolonged|...'
```

Full automated script: `scripts/faers_validate.sh`

## Technical Note

This validation was performed using SSRAG (Self-Sharpening RAG) with IOT distance metric
on the human protein interactome (STRING v12.0, 19,699 proteins). The lambda fold engine
created 3,421,130 fold edges (~174/protein) using cosine similarity with fold_k=200,
fold_threshold=0.02, producing 19,955 effective dimensions and 141.3:1 contrast ratio.
Convergence nodes were identified by multi-target neighborhood overlap queries using
fold-dimension cosine similarity.
