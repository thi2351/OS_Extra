#!/usr/bin/env bash
# ----------------------------------------------------------
# run.sh – build & execute all CFS tests in ./testcase
#           outputs saved to ./output
# ----------------------------------------------------------
set -euo pipefail

# Configuration
BIN=simulate_cfs
TESTDIR=./testcase
OUTDIR=./output
EXT=("*.in" "*.txt")     # accepted extensions

# Ensure output directory exists
mkdir -p "$OUTDIR"

# Build
echo "🛠  Building project…"
make clean > /dev/null
make all    > /dev/null

# Run test suite
echo "🚀  Executing test-suite"
for pat in "${EXT[@]}"; do
  for tc in "$TESTDIR"/$pat; do
    [[ -e "$tc" ]] || continue
    name=$(basename "$tc")
    out="$OUTDIR/${name%.*}.out"
    printf "\n▶ Running %-20s → %-20s\n" "$name" "$(basename "$out")"
    "./$BIN" "$tc" | tee "$out"
  done
done

echo -e "\n✅  All outputs written to $OUTDIR/"
