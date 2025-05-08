#!/usr/bin/env python3
import sys, heapq, collections

QUANTUM = 200
MAXPRIO = 40  # niceness -20..19 → 0..39

class PCB:
    def __init__(self, pid, nice, at, bt):
        self.pid = pid
        self.nice = nice
        self.prio = max(0, min(nice + 20, MAXPRIO - 1))
        self.arrival = at
        self.remain = bt

def load(path):
    with open(path) as f:
        num_cpus, n = map(int, f.readline().split())
        procs = []
        for _ in range(n):
            pid, nice, at, bt = map(int, f.readline().split())
            procs.append((at, PCB(pid, nice, at, bt)))
    return num_cpus, sorted(procs, key=lambda x: x[0])

def sim_mlq(path):
    num_cpus, arrivals = load(path)
    queues = [collections.deque() for _ in range(MAXPRIO)]
    cpus = [None] * num_cpus

    time = 0
    event_q = []
    counter = 0
    finished = 0
    total = len(arrivals)

    def push(t, typ, *args):
        nonlocal counter
        heapq.heappush(event_q, (t, typ, counter, *args))
        counter += 1

    for at, pcb in arrivals:
        push(at, 'arrive', pcb)

    while event_q:
        t, typ, _, *args = heapq.heappop(event_q)

        if typ == 'arrive':
            pcb = args[0]
            print(f"[t = {t}] Enqueue PID={pcb.pid}")
            queues[pcb.prio].append(pcb)

        elif typ == 'stop':
            cpu_id, pcb = args
            print(f"[t = {t}] Stopped PID={pcb.pid} in CPU {cpu_id + 1}")
            pcb.remain -= QUANTUM
            if pcb.remain > 0:
                print(f"[t = {t}] Enqueue PID={pcb.pid}")
                queues[pcb.prio].append(pcb)
            else:
                print(f"[t = {t}] Finish PID={pcb.pid}")
                finished += 1
            cpus[cpu_id] = None

        # Try to assign processes to free CPUs
        for i in range(num_cpus):
            if cpus[i] is None:
                for q in queues:
                    if q:
                        proc = q.popleft()
                        cpus[i] = (proc, t)
                        print(f"[t = {t}] Assigned process with PID={proc.pid} to CPU {i + 1}")
                        slice = min(QUANTUM, proc.remain)
                        push(t + slice, 'stop', i, proc)
                        break

        if finished == total and not any(cpus):
            print(f"[t = {t}] All done at t = {t}")
            break

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <workload.in>")
        sys.exit(1)
    sim_mlq(sys.argv[1])
