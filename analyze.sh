#!/usr/bin/env bash
# ------------------------------------------------------------
# analyze.sh – harvest simulate_cfs logs in ./output
#   ▸ creates ./metrics/  with:
#       • <case>.csv   tidy per-process metrics
#       • <case>.png   simple Gantt chart
#   ▸ prints a summary table to the console
# ------------------------------------------------------------
set -euo pipefail
OUTDIR=./output
METRICDIR=./metrics
SCRIPT=./tools/parse_cfs.py     # see section 2
mkdir -p "$METRICDIR"

printf "🔎 Parsing logs in %s …\n" "$OUTDIR"
printf "%-20s %8s %8s\n" "test-case" "avg_WT" "avg_TAT"

for log in "$OUTDIR"/*.out; do
  [[ -e "$log" ]] || continue
  name=$(basename "${log%.out}")
  python "$SCRIPT" "$log" \
         --csv  "$METRICDIR/$name.csv" \
         --gantt "$METRICDIR/$name.png" \
         --quiet
  read wt tat <<<"$(python "$SCRIPT" "$log" --print-avg)"
  printf "%-20s %8.2f %8.2f\n" "$name" "$wt" "$tat"
done

echo "✅  Metrics & charts written to $METRICDIR/"