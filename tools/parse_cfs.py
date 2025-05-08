#!/usr/bin/env python3
"""
parse_cfs.py – extract metrics & (optionally) Gantt charts
from logs in format:

  [t = 0] Enqueue PID=2
  [t = 0] Assigned process with PID=2 to CPU 1
  [t = 200] Stopped PID=2 in CPU 1
  [t = 400] Finish PID=2

USAGE
  python3 parse_cfs.py LOGFILE [--csv file] [--gantt file] [--avg] [--quiet]
"""

import re, csv, argparse, sys, statistics, textwrap, collections, pathlib

_RE_ENQ    = re.compile(r'\[t\s*=\s*(\d+)] Enqueue PID=(\d+)')
_RE_ASSIGN = re.compile(r'\[t\s*=\s*(\d+)] Assigned process with PID=(\d+) to CPU (\d+)')
_RE_STOP   = re.compile(r'\[t\s*=\s*(\d+)] Stopped PID=(\d+) in CPU (\d+)')
_RE_FINISH = re.compile(r'\[t\s*=\s*(\d+)] Finish PID=(\d+)')

def parse_log(path: str):
    arrival, finish = {}, {}
    runs: dict[int, list[tuple[int,int,int]]] = collections.defaultdict(list)  # CPU → list of (start, duration, pid)
    active: dict[tuple[int,int], int] = {}  # (pid, cpu) → start time

    with open(path, encoding='utf-8') as f:
        for ln in f:
            if m := _RE_ENQ.match(ln):
                t, pid = int(m[1]), int(m[2])
                arrival.setdefault(pid, t)
            elif m := _RE_ASSIGN.match(ln):
                t, pid, cpu = int(m[1]), int(m[2]), int(m[3])
                active[(pid, cpu)] = t
            elif m := _RE_STOP.match(ln):
                t, pid, cpu = int(m[1]), int(m[2]), int(m[3])
                if (pid, cpu) in active:
                    start = active.pop((pid, cpu))
                    runs[cpu].append((start, t - start, pid))
            elif m := _RE_FINISH.match(ln):
                t, pid = int(m[1]), int(m[2])
                finish[pid] = t
                for (p, c), start in list(active.items()):
                    if p == pid:
                        runs[c].append((start, t - start, pid))
                        active.pop((p, c))

    return arrival, finish, runs

def gantt(runs, out_png):
    try:
        import matplotlib.pyplot as plt
        import matplotlib.cm as cm
        import matplotlib.colors as mcolors
    except ImportError:
        sys.exit("matplotlib required for --gantt (pip install matplotlib)")

    # Get unique PIDs to assign consistent colors
    all_pids = {pid for tasks in runs.values() for _, _, pid in tasks}
    pid_list = sorted(all_pids)
    cmap = cm.get_cmap("tab20", len(pid_list))
    color_map = {pid: cmap(i) for i, pid in enumerate(pid_list)}

    cpus = sorted(runs)
    fig, ax = plt.subplots(figsize=(10, 0.6 * len(cpus)))
    yticks, ylabels = [], []

    for i, cpu in enumerate(cpus):
        for start, dur, pid in runs[cpu]:
            ax.broken_barh([(start, dur)], (i - 0.4, 0.8), facecolors=color_map[pid])
            ax.text(start + dur / 2, i, f'PID{pid}', ha='center', va='center', fontsize=8, color='white')
        yticks.append(i)
        ylabels.append(f'CPU{cpu}')

    ax.set_yticks(yticks)
    ax.set_yticklabels(ylabels)
    ax.set_xlabel("Time (ns)")
    ax.set_title(pathlib.Path(out_png).name)
    fig.tight_layout()
    fig.savefig(out_png)
    plt.close(fig)

def main():
    ap = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent(__doc__)
    )
    ap.add_argument("log", help="path to the simulation log file")
    ap.add_argument("--csv", help="write per-PID metrics to CSV")
    ap.add_argument("--gantt", help="output PNG path for Gantt chart")
    ap.add_argument("--avg", action="store_true", help="print average waiting and turnaround time")
    ap.add_argument("--quiet", action="store_true", help="suppress per-PID output")
    args = ap.parse_args()

    arr, fin, runs = parse_log(args.log)

    if args.gantt:
        gantt(runs, args.gantt)

if __name__ == "__main__":
    main()
