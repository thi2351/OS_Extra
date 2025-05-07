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

for log in "$OUTDIR"/*.out; do
  [[ -e "$log" ]] || continue
  name=$(basename "${log%.out}")
  python3 "$SCRIPT" "$log" \
         --csv  "$METRICDIR/$name.csv" --avg\
         --gantt "$METRICDIR/$name.png"
  read wt tat <<<"$(python3 "$SCRIPT" "$log")"
done

echo "✅  Metrics & charts written to $METRICDIR/"

# 1. run MLQ on every workload
mkdir -p output_mlq
for f in testcase/*.in; do
    base=$(basename "${f%.in}")
    python3 tools/sim_mlq.py "$f" > "output_mlq/${base}.out"
done

# 2. reuse your parser
mkdir -p metrics_mlq
for log in output_mlq/*.out; do
    base=$(basename "${log%.out}")
    python3 tools/parse_cfs.py "$log" \
           --csv "metrics_mlq/${base}.csv" --avg\
            --gantt "metrics_mlq/${base}.png"
done
