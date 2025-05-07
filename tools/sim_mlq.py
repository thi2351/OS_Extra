#!/usr/bin/env python3
"""
sim_mlq.py  –  stand-alone MLQ scheduler for OS_Extra test-cases
─────────────────────────────────────────────────────────────────────
Input  : <workload>.in  (same 4-column format used by simulate_cfs)
Output : prints log lines:
           [t=   X] enqueue PID=n
           [t=   X] run PID=n for   D
           [t=   X] finish PID=n
         identical to CFS logs → can be parsed by parse_cfs.py

MLQ policy (faithful to assignment):
  • Niceness (–20 … +19) is mapped to priority level P = nice + 20
    (0 = highest, 39 = lowest).
  • System has 40 queues; index P holds that priority’s PCBs.
  • CPU visits queues in priority order, each queue granted
      slot = MAX_PRIOS – P   time quanta (see assignment).
  • Quantum = 10 ns (matches MIN_GRANULARITY_NSEC used in CFS part).
  • Round-robin inside each queue.

This is *good enough* for comparative metrics (WT / TAT) with CFS.
"""

import sys, heapq, collections, argparse, pathlib

MAX_P = 40                  # priority levels (0..39)
QUANT = 10                  # 10 ns time slice
SLOT_FACTOR = MAX_P         # slot = MAX_P – priority

class PCB:
    __slots__ = ("pid","prio","remain")
    def __init__(self,pid,prio,burst):
        self.pid = pid; self.prio = prio; self.remain = burst

# ── replace the old load() with: ─────────────────────────────
def load(path):
    """return [(arrival_time, PCB), …] sorted by arrival_time"""
    procs=[]
    with open(path) as f:
        n = int(f.readline().strip())
        for _ in range(n):
            line = f.readline().strip()
            if not line:            # skip empty lines if any
                continue
            pid,nice,at,bt = map(int, line.split())
            prio = nice + 20 if -20 <= nice <= 19 else 39
            procs.append((at, PCB(pid, prio, bt)))
    return sorted(procs, key=lambda x: x[0])   # ← key-function added


def mlq_run(inp):
    arrivals = load(inp)
    arr_idx = 0
    t = 0
    queues = [collections.deque() for _ in range(MAX_P)]
    finished = 0
    total = len(arrivals)

    def enqueue(pcb):
        queues[pcb.prio].append(pcb)
        print(f"[t={t:4d}] enqueue PID={pcb.pid}")

    while finished < total:
        # bring in arrivals at current time
        while arr_idx < total and arrivals[arr_idx][0] == t:
            enqueue(arrivals[arr_idx][1]); arr_idx += 1
        # if no runnable, fast-forward to next arrival
        if all(not q for q in queues):
            nxt = arrivals[arr_idx][0]
            t = nxt
            continue

        # iterate priority queues
        for p in range(MAX_P):
            if not queues[p]: continue
            slot = SLOT_FACTOR - p               # number of quanta
            while slot and queues[p]:
                pcb = queues[p].popleft()
                run_ns = min(QUANT, pcb.remain)
                print(f"[t={t:4d}] run PID={pcb.pid} for {run_ns:4d}")
                pcb.remain -= run_ns
                t += run_ns
                # enqueue new arrivals that occurred during run
                while arr_idx < total and arrivals[arr_idx][0] <= t:
                    enqueue(arrivals[arr_idx][1]); arr_idx += 1
                if pcb.remain == 0:
                    finished += 1
                    print(f"[t={t:4d}] finish PID={pcb.pid}")
                else:
                    queues[p].append(pcb)
                slot -= 1
            # after consuming its slot, move to next queue
    print(f"All done at t={t}")

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="MLQ simulator")
    ap.add_argument("infile", help="work-load *.in")
    args = ap.parse_args()
    mlq_run(args.infile)
