#!/usr/bin/env bash
# ------------------------------------------------------------------
# make_testcases.sh – generate the evaluation workloads
# ------------------------------------------------------------------
set -euo pipefail
TESTDIR=./testcase
mkdir -p "$TESTDIR"

write() {          # $1 = filename  ;  remaining args = here-doc lines
  local file="$TESTDIR/$1"
  printf '%s\n' "$@" > "$file"
  echo "✅  wrote $file"
}

# ── Scenario 1 : same arrival, varying niceness ───────────────────
write sc01_vary_nice.in \
"4" \
"1  -10   0   80" \
"2    0   0   80" \
"3    5   0   80" \
"4   10   0   80"

# ── Scenario 2 : staggered arrivals, different bursts ─────────────
write sc02_arrivals.in \
"5" \
"1   0    0   120" \
"2  -5    8    40" \
"3   5   15    60" \
"4   0   25    30" \
"5  -5   40    20"

# ── Scenario 3 : mixed realistic workload ────────────────────────
write sc03_mixed.in \
"6" \
"1  -10    0   100" \
"2    0    0    60" \
"3    5   10    30" \
"4   -5   20    80" \
"5   10   35    10" \
"6    0   50    50"

echo "🎉  All evaluation test-cases ready in $TESTDIR/"
