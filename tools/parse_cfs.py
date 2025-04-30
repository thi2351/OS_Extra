#!/usr/bin/env python3
"""
parse_cfs.py  –  extract metrics & (optionally) Gantt charts
from simulate_cfs logs.

USAGE
  python3 parse_cfs.py LOGFILE [options]

OPTIONS
  --csv   <file>   write per-PID metrics CSV
  --gantt <png>    produce Gantt chart (needs matplotlib)
  --avg            print "AVG_WT AVG_TAT" to stdout
  --quiet          suppress detailed per-PID printing

CSV columns: PID,Arrival,Burst,Turnaround,Waiting
"""

from __future__ import annotations
import re, csv, argparse, sys, statistics, textwrap, collections, pathlib

# ── log-line regex ────────────────────────────────────────────
_RE_RUN    = re.compile(r'\[t=\s*(\d+)] run PID=(\d+) for\s*(\d+)')
_RE_ENQ    = re.compile(r'\[t=\s*(\d+)] enqueue PID=(\d+)')
_RE_FINISH = re.compile(r'\[t=\s*(\d+)] finish PID=(\d+)')

# ── parse one logfile ─────────────────────────────────────────
def parse_log(path: str):
    arrival, finish = {}, {}
    runs: dict[int, list[tuple[int,int]]] = collections.defaultdict(list)

    with open(path, encoding='utf-8') as f:
        for ln in f:
            if m := _RE_ENQ.search(ln):
                arrival.setdefault(int(m[2]), int(m[1]))
            elif m := _RE_RUN.search(ln):
                runs[int(m[2])].append((int(m[1]), int(m[3])))
            elif m := _RE_FINISH.search(ln):
                finish[int(m[2])] = int(m[1])

    missing = [p for p in arrival if p not in finish]
    if missing:
        raise ValueError(f"{path}: PIDs never finished → {missing}")
    return arrival, runs, finish

# ── metric table ──────────────────────────────────────────────
def metrics(arrival,runs,finish):
    rows=[]
    for pid in sorted(arrival):
        burst = sum(d for _,d in runs[pid])
        tat   = finish[pid] - arrival[pid]
        wt    = tat - burst
        rows.append((pid, arrival[pid], burst, tat, wt))
    return rows

# ── gantt helper ──────────────────────────────────────────────
def gantt(rows,runs,out_png):
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("matplotlib required for --gantt (pip install matplotlib)")
    fig,ax = plt.subplots(figsize=(8, 0.5 + 0.4*len(rows)))
    yticks,labels=[],[]
    for i,(pid,*_) in enumerate(rows):
        for s,d in runs[pid]:
            ax.broken_barh([(s,d)], (i-0.4,0.8))
        yticks.append(i); labels.append(f"PID{pid}")
    ax.set_yticks(yticks); ax.set_yticklabels(labels)
    ax.set_xlabel("time (ns)")
    ax.set_title(pathlib.Path(out_png).name)
    fig.tight_layout(); fig.savefig(out_png); plt.close(fig)

# ── command-line interface ────────────────────────────────────
def main():
    ap = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent(__doc__))
    ap.add_argument("log")
    ap.add_argument("--csv")
    ap.add_argument("--gantt")
    ap.add_argument("--avg", action="store_true")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    arr,runs,fin = parse_log(args.log)
    rows = metrics(arr,runs,fin)

    if args.csv:
        with open(args.csv,"w",newline='') as f:
            w=csv.writer(f)
            w.writerow(("PID","Arrival","Burst","Turnaround","Waiting"))
            w.writerows(rows)

    if args.gantt:
        gantt(rows,runs,args.gantt)

    if not args.quiet:
        for r in rows:
            print(f"PID {r[0]:>3} | arr={r[1]:>5}  burst={r[2]:>4}  "
                  f"TAT={r[3]:>5}  WT={r[4]:>5}")

    if args.avg:
        wt = statistics.fmean(r[4] for r in rows)
        tat= statistics.fmean(r[3] for r in rows)
        print(f"{wt:.2f} {tat:.2f}")

if __name__ == "__main__":
    main()
