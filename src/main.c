#include "common.h"
#include "cfs.h"
#include "heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>

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
            fprintf(stderr, "Error: bad format or niceness out of range at line %d in '%s'\n", i+2, filename);
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

void simulate_cfs(pcb_t *pcbs, int *arrival, int *remain, int n) {
    bool *finished = calloc(n, sizeof(bool));
    if (!finished) { perror("calloc"); exit(EXIT_FAILURE); }

    heap_t arrivals;
    heap_init(&arrivals, sizeof(arrival_event_t), n, arrival_cmp);
    for (int i = 0; i < n; i++) {
        arrival_event_t ev = { .time = (uint64_t)arrival[i], .proc = &pcbs[i] };
        heap_push(&arrivals, &ev);
    }

    uint64_t t = 0;
    int done = 0;

    while (done < n) {
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
                printf("\n");
                continue;
            }
            break;
        }

                int idx = (int)(curr - pcbs);
        assert(idx >= 0 && idx < n);

        uint64_t slice = cfs_timeslice(curr);
        uint64_t rem = remain[idx];
        uint64_t run_total = slice < rem ? slice : rem;
        uint64_t run_done = 0;

        arrival_event_t ae2;
        while (run_done < run_total) {
            uint64_t next_arr = UINT64_MAX;
            if (heap_peek(&arrivals, &ae2) == 0) next_arr = ae2.time;

            uint64_t t_until_arr = next_arr > t ? next_arr - t : 0;
            uint64_t to_run = run_total - run_done;
            if (t_until_arr > 0 && to_run > t_until_arr)
                to_run = t_until_arr;

            printf("[t=%4llu] run PID=%u for %4llu",
                   (unsigned long long)t, curr->pid,
                   (unsigned long long)to_run);
            printf("\n");

            t += to_run;
            run_done += to_run;

            // handle any arrivals exactly at new time t
            while (heap_peek(&arrivals, &ae2) == 0 && ae2.time == t) {
                heap_pop(&arrivals, &ae2);
                cfs_enqueue(ae2.proc);
                printf("[t=%4llu] enqueue PID=%u", (unsigned long long)t, ae2.proc->pid);
                printf("\n");
                uint32_t time_slice = cfs_timeslice(curr);
                run_total = time_slice < remain[idx] ? time_slice : remain[idx];
            }
            if (run_done >= run_total) {
                cfs_task_tick(curr, run_done);
                remain[idx] -= (int)run_done;
                continue;
            }
            // compare curr vs best (without updated vruntime for curr)
            pcb_t *best = cfs_pick_next();
            if (best != curr) {
                printf("[t=%4llu] preempt PID=%u -> PID=%u", (unsigned long long)t, curr->pid, best->pid);
                printf("\n");
                cfs_task_tick(curr, run_done);
                remain[idx] -= (int)run_done;

                curr = best;
                idx = (int)(curr - pcbs);
                slice = cfs_timeslice(curr);
                rem = remain[idx];
                run_total = slice < rem ? slice : rem;
                run_done = 0;
                break; 
            }
        }

        if (remain[idx] == 0) {
            cfs_dequeue(curr);
            finished[idx] = true;
            done++;
            printf("[t=%4llu] finish PID=%u\n", (unsigned long long)t, curr->pid);
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
