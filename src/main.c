#include "common.h"
#include "cfs.h"    
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/**
 * load_processes:
 *   - filename    : path to the input file
 *   - out_pcbs    : &pcb_t*      -> array of pcb_t
 *   - out_arrival : &int*        -> arrival times
 *   - out_burst   : &int*        -> burst times
 *   - out_remain  : &int*        -> remaining times
 *   - out_n       : &int         -> number of processes
 *
 * File format:
 *   n
 *   pid niceness arrival_time burst_time
 *   ...
 */
void load_processes(const char *filename,
                    pcb_t   **out_pcbs,
                    int     **out_arrival,
                    int     **out_burst,
                    int     **out_remain,
                    int     *out_n)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int n;
    if (fscanf(fp, "%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Error: invalid process count in '%s'\n", filename);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    *out_n = n;

    *out_pcbs    = malloc(sizeof(pcb_t) * n);
    *out_arrival = malloc(sizeof(int)   * n);
    *out_burst   = malloc(sizeof(int)   * n);
    *out_remain  = malloc(sizeof(int)   * n);
    if (!*out_pcbs || !*out_arrival || !*out_burst || !*out_remain) {
        perror("malloc");
        fclose(fp);
        exit(EXIT_FAILURE);
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
        (*out_arrival)[i] = at;
        (*out_burst)[i]   = bt;
        (*out_remain)[i]  = bt;
    }

    fclose(fp);
}

/**
 * simulate_cfs: Event-driven simulation of the CFS scheduler.
 * Uses cfs_timeslice(p) for slice calculation and CFS RQ API.
 */
void simulate_cfs(pcb_t *pcbs,
                  int   *arrival,
                  int   *burst,
                  int   *remain,
                  int    n)
{
    /* Simplest ready array and current tracking */
    bool *finished = calloc(n, sizeof(bool));
    if (!finished) { perror("calloc"); exit(EXIT_FAILURE); }

    uint64_t t = 0;
    int done = 0;

    /* Event: arrival times only, handle preemption between arrivals */
    while (done < n) {
        /* Enqueue all that arrive at time t */
        for (int i = 0; i < n; i++) {
            if (!finished[i] && arrival[i] == (int)t) {
                cfs_enqueue(&pcbs[i]);
                printf("[t=%4llu] enq PID=%u\n", (unsigned long long)t, pcbs[i].pid);
            }
        }

        /* Pick next */
        pcb_t *p = cfs_pick_next();
        if (!p) { t++; continue; } //Fix this, if !p, process the t into earliest time that have process running on CPU
        int idx = p - pcbs;

        uint64_t slice = cfs_timeslice(p);
        /* run until next arrival or slice or completion */
        uint64_t next_at = UINT64_MAX;
        for (int i = 0; i < n; i++) {
            if (!finished[i] && arrival[i] > (int)t && (uint64_t)arrival[i] < next_at)
                next_at = arrival[i];
        }
        uint64_t run_for = slice;
        if (remain[idx] < (int)run_for) run_for = remain[idx];
        if (next_at < t + run_for) run_for = next_at - t;

        /* execute */
        remain[idx] -= (int)run_for;
        cfs_update_vruntime(p, run_for);
        printf("[t=%4llu] run PID=%u for %4llu (rem=%d)\n",
               (unsigned long long)t, p->pid,
               (unsigned long long)run_for, remain[idx]);

        t += run_for;

        if (remain[idx] == 0) {
            cfs_dequeue(p);
            finished[idx] = true;
            done++;
            printf("[t=%4llu] fini PID=%u\n", (unsigned long long)t, p->pid);
        }
    }

    printf("All done at t=%llu\n", (unsigned long long)t);
    free(finished);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pcb_t *pcbs;
    int *arrival, *burst, *remain;
    int  n;

    load_processes(argv[1], &pcbs, &arrival, &burst, &remain, &n);
    cfs_init_rq();
    simulate_cfs(pcbs, arrival, burst, remain, n);

    free(pcbs);
    free(arrival);
    free(burst);
    free(remain);
    return EXIT_SUCCESS;
}
