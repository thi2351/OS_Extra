#include "common.h"
#include "cfs.h"
#include "heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>

void load_processes(const char *filename,
                    pcb_t   **out_pcbs,
                    int     **out_arrival,
                    int     **out_remain,
                    int     *out_n)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen"); exit(EXIT_FAILURE); }

    int n;
    if (fscanf(fp, "%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Error: invalid process count in '%s'\n", filename);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    *out_n = n;

    *out_pcbs    = malloc(sizeof(pcb_t) * n);
    *out_arrival = malloc(sizeof(int)   * n);
    *out_remain  = malloc(sizeof(int)   * n);
    if (!*out_pcbs || !*out_arrival || !*out_remain) {
        perror("malloc"); fclose(fp); exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        int pid, nice, at, bt;
        if (fscanf(fp, "%d %d %d %d", &pid, &nice, &at, &bt) != 4
            || nice < -20 || nice > 19) {
            fprintf(stderr,
                    "Error: bad format or niceness out of range at line %d in '%s'\n",
                    i + 2, filename);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        (*out_pcbs)[i].pid      = (uint32_t)pid;
        (*out_pcbs)[i].vruntime = 0;
        (*out_pcbs)[i].weight   = cfs_compute_weight(nice);
        (*out_arrival)[i]       = at;
        (*out_remain)[i]        = bt;
    }
    fclose(fp);
}

typedef struct {
    uint64_t time;
    pcb_t   *proc;
} arrival_event_t;

static int arrival_cmp(const void *a, const void *b) {
    const arrival_event_t *e1 = a;
    const arrival_event_t *e2 = b;
    if (e1->time < e2->time) return -1;
    if (e1->time > e2->time) return  1;
    return 0;
}

void simulate_cfs(pcb_t *pcbs, int *arrival, int *remain, int n) {
    bool *finished = calloc(n, sizeof(bool));
    if (!finished) { perror("calloc"); exit(EXIT_FAILURE); }

    heap_t arrivals;
    if (heap_init(&arrivals, sizeof(arrival_event_t), n, arrival_cmp) != 0) {
        perror("heap_init arrivals"); exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        arrival_event_t ev = { .time = (uint64_t)arrival[i], .proc = &pcbs[i] };
        heap_push(&arrivals, &ev);
    }

    uint64_t t = 0;
    int done_count = 0;

    while (done_count < n) {
        arrival_event_t ae;
        while (heap_peek(&arrivals, &ae) == 0 && ae.time == t) {
            heap_pop(&arrivals, &ae);
            cfs_enqueue(ae.proc);
            printf("[t=%4llu] enqueue PID=%u\n", (unsigned long long)t, ae.proc->pid);
        }

        pcb_t *curr = cfs_pick_next();
        if (!curr) {
            if (heap_pop(&arrivals, &ae) == 0) {
                t = ae.time;
                cfs_enqueue(ae.proc);
                printf("[t=%4llu] enqueue PID=%u\n", (unsigned long long)t, ae.proc->pid);
                continue;
            }
            break;
        }

        int idx = (int)(curr - pcbs);
        assert(idx >= 0 && idx < n);

        uint64_t slice = cfs_timeslice(curr);
        uint64_t next_arr = UINT64_MAX;
        if (heap_peek(&arrivals, &ae) == 0) next_arr = ae.time;

        uint64_t run_for = slice;
        if ((uint64_t)remain[idx] < run_for) run_for = remain[idx];
        if (t + run_for > next_arr) run_for = next_arr - t;

        remain[idx] -= (int)run_for;
        cfs_task_tick(curr, run_for);
        printf("[t=%4llu] run  PID=%u for %4llu (rem=%d)\n",
               (unsigned long long)t, curr->pid,
               (unsigned long long)run_for, remain[idx]);

        t += run_for;

        printf(" VRUNTIMES: ");
        for (int i = 0; i < n; i++) {
            printf("[PID=%u: vruntime=%.2f] ", pcbs[i].pid, pcbs[i].vruntime);
        }
        printf("\n");
        while (heap_peek(&arrivals, &ae) == 0 && ae.time == t) {
            heap_pop(&arrivals, &ae);
            cfs_enqueue(ae.proc);
            printf("[t=%4llu] enqueue PID=%u\n", (unsigned long long)t, ae.proc->pid);
        }

        if (remain[idx] == 0) {
            cfs_dequeue(curr);
            finished[idx] = true;
            done_count++;
            printf("[t=%4llu] finish PID=%u\n", (unsigned long long)t, curr->pid);
            printf("\n");
        }
    }

    printf("All done at t=%llu\n", (unsigned long long)t);

    heap_free(&arrivals);
    free(finished);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pcb_t *pcbs;
    int   *arrival, *remain;
    int    n;

    load_processes(argv[1], &pcbs, &arrival, &remain, &n);
    cfs_init_rq();
    simulate_cfs(pcbs, arrival, remain, n);

    free(pcbs);
    free(arrival);
    free(remain);
    return EXIT_SUCCESS;
}
