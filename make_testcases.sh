#!/usr/bin/env bash
# --------------------------------------------------------------------
# make_testcases.sh
#
#  • With no arguments: re-generate the five reference CFS inputs used
#    in the README (tc01-tc05) and place them in ./testcase/.
#  • With arguments:    generate a custom random workload file.
#
# Usage
#   ./make_testcases.sh               # create reference tests
#   ./make_testcases.sh custom 50 60  # 50 procs, niceness ±60 random
# --------------------------------------------------------------------
set -euo pipefail

TESTDIR=./testcase
mkdir -p "$TESTDIR"

# --------------------------------------------------------------------
# helper: write file from heredoc
mk() {  # 1:filename   2+:here-doc lines
  local f="$TESTDIR/$1"
  shift
  printf '%s\n' "$@" > "$f"
  echo "✅  wrote $f"
}

# --------------------------------------------------------------------
if [[ $# -eq 0 ]]; then
  echo "⇢ Generating reference test-cases in $TESTDIR"

  mk tc01_equal_nice.in       \
  "3"                         \
  "1 0 0 30"                  \
  "2 0 0 30"                  \
  "3 0 0 30"

  mk tc02_weighted_nice.in    \
  "3"                         \
  "1 -10 0 60"                \
  "2   0 0 60"                \
  "3  10 0 60"

  mk tc03_staggered_arrival.in\
  "3"                         \
  "1 0  0 40"                 \
  "2 0 20 40"                 \
  "3 0 40 40"

  mk tc04_preemption.in       \
  "2"                         \
  "1  5  0 100"               \
  "2 -15 5  20"

  mk tc05_min_granularity.in  \
  "10"                        \
  "1 0 0 1"  "2 0 0 1"  "3 0 0 1"  "4 0 0 1"  "5 0 0 1" \
  "6 0 0 1"  "7 0 0 1"  "8 0 0 1"  "9 0 0 1" "10 0 0 1"

  exit 0
fi

# --------------------------------------------------------------------
# custom random generator
if [[ $1 == "custom" ]]; then
  COUNT=${2:-20}        # number of processes
  RAND_NICE=${3:-40}    # max |niceness| (0..RAND_NICE)
  OUT="$TESTDIR/random_${COUNT}.in"

  printf '%d\n' "$COUNT" > "$OUT"
  for ((i=1;i<=COUNT;i++)); do
    nice=$(( (RANDOM % (2*RAND_NICE+1)) - RAND_NICE ))
    at=$(( RANDOM % 100 ))          # arrival 0-99
    burst=$(( 10 + RANDOM % 91 ))   # burst 10-100
    printf "%d %d %d %d\n" "$i" "$nice" "$at" "$burst" >> "$OUT"
  done
  echo "✅  wrote $OUT  (random workload)"
  exit 0
fi

echo "Usage:"
echo "  $0              # build reference suite"
echo "  $0 custom N R   # create random N-process testcase, niceness ±R"
exit 1
