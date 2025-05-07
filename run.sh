#!/usr/bin/env bash
# ----------------------------------------------------------
# run.sh – build & execute every CFS test in ./testcase
#           outputs saved to ./output
# ----------------------------------------------------------
set -euo pipefail

BIN=simulate_cfs
TESTDIR=./testcase
OUTDIR=./output
EXT=("*.in" "*.txt")     # accepted extensions

mkdir -p "$OUTDIR"

echo "🛠  Building project …"
make -s clean
make  -s

echo "🚀  Executing test-suite"
for pat in "${EXT[@]}"; do
  for tc in $TESTDIR/$pat; do
    [[ -e "$tc" ]] || continue
    name=$(basename "$tc")
    out="$OUTDIR/${name%.*}.out"
    echo "——————————————————————————————————————"
    echo "▶ $name  →  $(basename "$out")"
    "./$BIN" "$tc" | tee "$out"
  done
done

echo "✅  Outputs written to $OUTDIR/"
./analyze.sh