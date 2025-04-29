#!/usr/bin/env python3
"""
Extracts metrics from simulate_cfs textual logs.

Output options:
  --csv   <file>   write per-PID metrics as CSV
  --gantt <file>   draw a Gantt chart (requires matplotlib)
  --print-avg      print "WT  TAT" (average waiting / turnaround)
"""
import re, sys, csv, argparse, collections, matplotlib.pyplot as plt
pat_run    = re.compile(r'\[t=(\d+)] run PID=(\d+) for (\d+)')
pat_finish = re.compile(r'\[t=(\d+)] finish PID=(\d+)')
pat_enq    = re.compile(r'\[t=(\d+)] enqueue PID=(\d+)')

def parse(path):
    arrival, runs, finish = {}, collections.defaultdict(list), {}
    with open(path) as f:
        for line in f:
            if m:=pat_enq.search(line):   arrival.setdefault(int(m[2]), int(m[1]))
            if m:=pat_run.search(line):   runs[int(m[2])].append((int(m[1]), int(m[3])))
            if m:=pat_finish.search(line):finish[int(m[2])] = int(m[1])
    return arrival, runs, finish

def metrics(arrival, runs, finish):
    rows=[]
    for pid in sorted(arrival):
        burst = sum(d for _,d in runs[pid])
        tat   = finish[pid]-arrival[pid]
        wt    = tat-burst
        rows.append((pid,arrival[pid],burst,tat,wt))
    return rows

def gantt(rows, runs, outpng):
    fig,ax=plt.subplots()
    yticks,ylabels = [],[]
    for i,(pid,_,_,_,_) in enumerate(rows):
        start=0
        for t,d in runs[pid]:
            ax.broken_barh([(t,d)], (i-0.4,0.8))
        yticks.append(i)
        ylabels.append(f"PID{pid}")
    ax.set_yticks(yticks); ax.set_yticklabels(ylabels)
    ax.set_xlabel("time (ns)"); ax.set_title(outpng.rsplit('/',1)[-1])
    fig.tight_layout(); fig.savefig(outpng); plt.close(fig)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("log")
    ap.add_argument("--csv"); ap.add_argument("--gantt")
    ap.add_argument("--print-avg", action="store_true")
    ap.add_argument("--quiet", action="store_true")
    a=ap.parse_args()
    arr,runs,fin = parse(a.log)
    rows = metrics(arr,runs,fin)
    if a.csv:
        with open(a.csv,"w",newline="") as f:
            w=csv.writer(f); w.writerow(
            ("PID","Arrival","Burst","Turnaround","Waiting"))
            w.writerows(rows)
    if a.gantt: gantt(rows,runs,a.gantt)
    if not a.quiet:
        for r in rows: print(r)
    if a.print_avg:
        avgW=sum(r[4] for r in rows)/len(rows)
        avgT=sum(r[3] for r in rows)/len(rows)
        print(f"{avgW:.2f} {avgT:.2f}")

if __name__=="__main__": main()
