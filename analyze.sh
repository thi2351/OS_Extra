#!/usr/bin/env bash
set -euo pipefail

BIN=simulate_cfs
TESTDIR=./testcase
OUTDIR=./output
MLQOUT=./output_mlq
METRIC_CFS=./metrics
METRIC_MLQ=./metrics_mlq
PARSER=./tools/parse_cfs.py
SIM_MLQ=./tools/sim_mlq.py

mkdir -p "$OUTDIR" "$MLQOUT" "$METRIC_CFS" "$METRIC_MLQ"
# mkdir -p "$MLQOUT" "$METRIC_MLQ"

echo "🛠 Building CFS…"
make -s clean && make -s

echo "🚀 Running CFS testcases…"
for tc in "$TESTDIR"/*.{in}; do
  [[ -e "$tc" ]] || continue
  b=$(basename "$tc")
  out="$OUTDIR/${b%.*}.out"
  "./$BIN" "$tc" | tee "$out"
done

echo "📊 Parsing CFS logs…"
for log in "$OUTDIR"/*.out; do
  b=$(basename "${log%.out}")
  python3 "$PARSER" "$log" \
    --csv "$METRIC_CFS/$b.csv" --gantt "$METRIC_CFS/$b.png" --avg > /dev/null
done
echo "✅ CFS metrics → $METRIC_CFS/"

echo "🚀 Running MLQ testcases…"
for tc in "$TESTDIR"/*.in; do
  b=$(basename "${tc%.in}")
  out="$MLQOUT/$b.out"
  python3 "$SIM_MLQ" "$tc" | tee "$out"
done

echo "📊 Parsing MLQ logs…"
for log in "$MLQOUT"/*.out; do
  b=$(basename "${log%.out}")
  python3 "$PARSER" "$log" \
    --csv "$METRIC_MLQ/$b.csv" --gantt "$METRIC_MLQ/$b.png" --avg > /dev/null
done
echo "✅ MLQ metrics → $METRIC_MLQ/"
