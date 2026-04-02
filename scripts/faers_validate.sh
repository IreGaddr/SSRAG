#!/usr/bin/env bash
# FAERS validation of polypharmacy convergence node findings
# Queries openFDA for cardiac adverse event rates in single vs multi-drug reports
set -euo pipefail

API="https://api.fda.gov/drug/event.json"
LIMIT=100  # max per request without key
PAGES=10   # pull 10 pages = 1000 events per query

# Our 6 drug classes (representative drugs)
DRUGS=(amlodipine sertraline omeprazole gabapentin lorazepam lisinopril)
DRUG_CLASSES=("CCB" "SSRI" "PPI" "Gabapentinoid" "Benzodiazepine" "ACE_inhibitor")

# Cardiac MedDRA preferred terms from our convergence node findings
CARDIAC_TERMS="Bradycardia|Arrhythmia|Cardiac arrest|QT prolonged|Electrocardiogram QT prolonged|Torsade de pointes|Ventricular tachycardia|Ventricular fibrillation|Sinus node dysfunction|Heart rate decreased|Cardiac failure|Cardiac disorder|Sudden cardiac death|Syncope|Palpitations|Tachycardia"

OUTDIR="/home/ire/code/ssrag/docs/findings/faers_validation"
mkdir -p "$OUTDIR"

echo "=== FAERS Polypharmacy Cardiac Validation ==="
echo "Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo ""

# ─────────────────────────────────────────────────────────────
# Phase 1: Single-drug cardiac event rates
# ─────────────────────────────────────────────────────────────
echo "Phase 1: Single-drug cardiac event baselines"
echo "─────────────────────────────────────────────"

for i in "${!DRUGS[@]}"; do
    drug="${DRUGS[$i]}"
    class="${DRUG_CLASSES[$i]}"

    echo -n "  $class ($drug): "

    total_events=0
    cardiac_events=0

    for page in $(seq 0 $((PAGES-1))); do
        skip=$((page * LIMIT))
        resp=$(curl -s "${API}?search=patient.drug.openfda.generic_name:${drug}&limit=${LIMIT}&skip=${skip}" 2>/dev/null)

        # Count total reactions in this page
        page_total=$(echo "$resp" | jq '[.results[]?.patient.reaction[]?.reactionmeddrapt] | length' 2>/dev/null || echo 0)
        total_events=$((total_events + page_total))

        # Count cardiac reactions
        page_cardiac=$(echo "$resp" | jq -r '[.results[]?.patient.reaction[]?.reactionmeddrapt] | map(select(. != null)) | .[]' 2>/dev/null | grep -ciE "$CARDIAC_TERMS" || true)
        cardiac_events=$((cardiac_events + page_cardiac))

        # Respect rate limit (40/min without key)
        sleep 1.6
    done

    if [ "$total_events" -gt 0 ]; then
        rate=$(echo "scale=4; $cardiac_events * 100 / $total_events" | bc)
        echo "cardiac=${cardiac_events}/${total_events} reactions (${rate}%)"
    else
        echo "no data"
    fi

    echo "$drug,$class,$cardiac_events,$total_events" >> "$OUTDIR/single_drug_rates.csv"
done

echo ""

# ─────────────────────────────────────────────────────────────
# Phase 2: Pairwise combinations — key convergence pairs
# ─────────────────────────────────────────────────────────────
echo "Phase 2: Pairwise drug combinations (cardiac rates)"
echo "────────────────────────────────────────────────────"

# Test the most clinically relevant pairs from our findings
PAIRS=(
    "amlodipine+sertraline"     # CCB+SSRI — HCN4 convergence (bradycardia)
    "amlodipine+gabapentin"     # CCB+Gabapentinoid — KCNMB1 convergence
    "sertraline+omeprazole"     # SSRI+PPI — HTR3B convergence
    "lorazepam+gabapentin"      # Benzo+Gabapentinoid — CNR1/CNR2 convergence
    "amlodipine+lorazepam"      # CCB+Benzo — combined cardiac depression
    "sertraline+gabapentin"     # SSRI+Gabapentinoid
)

for pair in "${PAIRS[@]}"; do
    drug1="${pair%%+*}"
    drug2="${pair#*+}"

    echo -n "  ${drug1} + ${drug2}: "

    total_events=0
    cardiac_events=0

    for page in $(seq 0 $((PAGES-1))); do
        skip=$((page * LIMIT))
        resp=$(curl -s "${API}?search=patient.drug.openfda.generic_name:${drug1}+AND+patient.drug.openfda.generic_name:${drug2}&limit=${LIMIT}&skip=${skip}" 2>/dev/null)

        page_total=$(echo "$resp" | jq '[.results[]?.patient.reaction[]?.reactionmeddrapt] | length' 2>/dev/null || echo 0)
        total_events=$((total_events + page_total))

        page_cardiac=$(echo "$resp" | jq -r '[.results[]?.patient.reaction[]?.reactionmeddrapt] | map(select(. != null)) | .[]' 2>/dev/null | grep -ciE "$CARDIAC_TERMS" || true)
        cardiac_events=$((cardiac_events + page_cardiac))

        sleep 1.6
    done

    if [ "$total_events" -gt 0 ]; then
        rate=$(echo "scale=4; $cardiac_events * 100 / $total_events" | bc)
        echo "cardiac=${cardiac_events}/${total_events} reactions (${rate}%)"
    else
        echo "no data"
    fi

    echo "${drug1},${drug2},${cardiac_events},${total_events}" >> "$OUTDIR/pairwise_rates.csv"
done

echo ""

# ─────────────────────────────────────────────────────────────
# Phase 3: Triple and quad combinations
# ─────────────────────────────────────────────────────────────
echo "Phase 3: 3-drug and 4-drug combinations"
echo "────────────────────────────────────────"

COMBOS=(
    "amlodipine+sertraline+omeprazole"                # CCB+SSRI+PPI
    "amlodipine+sertraline+gabapentin"                # CCB+SSRI+Gabapentinoid
    "amlodipine+lorazepam+gabapentin"                 # CCB+Benzo+Gabapentinoid
    "sertraline+omeprazole+gabapentin"                # SSRI+PPI+Gabapentinoid
    "amlodipine+sertraline+omeprazole+gabapentin"     # 4-drug
    "amlodipine+sertraline+lorazepam+gabapentin"      # 4-drug
)

for combo in "${COMBOS[@]}"; do
    IFS='+' read -ra parts <<< "$combo"

    # Build search query
    search=""
    label=""
    for part in "${parts[@]}"; do
        if [ -n "$search" ]; then
            search="${search}+AND+"
            label="${label}+"
        fi
        search="${search}patient.drug.openfda.generic_name:${part}"
        label="${label}${part}"
    done

    echo -n "  ${label}: "

    total_events=0
    cardiac_events=0
    total_reports=0

    for page in $(seq 0 $((PAGES-1))); do
        skip=$((page * LIMIT))
        resp=$(curl -s "${API}?search=${search}&limit=${LIMIT}&skip=${skip}" 2>/dev/null)

        # Check if we got results
        page_reports=$(echo "$resp" | jq '.results | length' 2>/dev/null || echo 0)
        if [ "$page_reports" = "0" ] || [ "$page_reports" = "null" ]; then
            break
        fi
        total_reports=$((total_reports + page_reports))

        page_total=$(echo "$resp" | jq '[.results[]?.patient.reaction[]?.reactionmeddrapt] | length' 2>/dev/null || echo 0)
        total_events=$((total_events + page_total))

        page_cardiac=$(echo "$resp" | jq -r '[.results[]?.patient.reaction[]?.reactionmeddrapt] | map(select(. != null)) | .[]' 2>/dev/null | grep -ciE "$CARDIAC_TERMS" || true)
        cardiac_events=$((cardiac_events + page_cardiac))

        sleep 1.6
    done

    if [ "$total_events" -gt 0 ]; then
        rate=$(echo "scale=4; $cardiac_events * 100 / $total_events" | bc)
        echo "cardiac=${cardiac_events}/${total_events} reactions (${rate}%), ${total_reports} reports"
    else
        echo "no data (${total_reports} reports)"
    fi

    echo "${label},${#parts[@]},${cardiac_events},${total_events},${total_reports}" >> "$OUTDIR/multi_drug_rates.csv"
done

echo ""

# ─────────────────────────────────────────────────────────────
# Phase 4: Get total report counts for denominators
# ─────────────────────────────────────────────────────────────
echo "Phase 4: Total FAERS reports per drug"
echo "──────────────────────────────────────"

for drug in "${DRUGS[@]}"; do
    total=$(curl -s "${API}?search=patient.drug.openfda.generic_name:${drug}&limit=1" | jq '.meta.results.total' 2>/dev/null || echo "?")
    echo "  $drug: $total reports"
    sleep 1.6
done

echo ""
echo "=== Validation complete ==="
echo "Results saved to: $OUTDIR/"
