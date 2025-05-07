#include "common.h"
#include "cfs.h"
#include "heap.h"
#include "event.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>


static int arrival_cmp(const void *a, const void *b) {
    const event_t *e1 = a;
    const event_t *e2 = b;
    if (e1->time < e2->time) return -1;
    if (e1->time > e2->time) return  1;
    return 0;
}

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))
#define COMPUTE_END_TIME(t, run_done, run_for) \
    ((run_done) < (run_for) ? (uint64_t)((t) + ((run_for) - (run_done))) : (uint64_t)(t))

static event_t make_event(cpu_t *cpu, event_type ev_type, pcb_t *proc, uint64_t time) {
    event_t e;
    e.cpu  = cpu;
    e.ev   = ev_type;
    e.proc = proc;
    e.time = time;
    return e;
}

void load_processes(const char *filename,
                    pcb_t   **out_pcbs,
                    int     **out_arrival,
                    int     **out_remain,
                    int     *out_n,
                    int     *num_cpu)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen"); exit(EXIT_FAILURE); }

    int n, cpu;
    if (fscanf(fp, "%d %d", &cpu, &n) != 2 || n <= 0 || cpu <= 0) {
        fprintf(stderr, "Error: invalid process count in '%s'\n", filename);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    *out_n = n;
    *num_cpu = cpu;
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

void simulate_cfs(pcb_t *pcbs, int *arrival, int *remain, int num_cpu, int num_process) {
    bool *finished = calloc(num_process, sizeof(bool));
    if (!finished) { perror("calloc"); exit(EXIT_FAILURE); }
    int *time_slice = calloc(num_process, sizeof(int));
    if (!time_slice) { perror("calloc"); exit(EXIT_FAILURE); }

    // CPU Initialization
    cpu_init(num_cpu);

    // Event-driven tree Initialization
    event_tree ev_t;
    event_tree_init(&ev_t);
    for (int i = 0; i < num_process; i++) {
        event_t ev_insert = make_event(NULL, EVENT_ARRIVAL, &pcbs[i], arrival[i]);
        // printf("[t=%llu] enqueue PID=%u\n", (unsigned long long)0, i);
        event_tree_insert(&ev_t, &ev_insert);
    }

    uint64_t t = 0;
    int done = 0;
    while (done < num_process) {
        event_t ev;
        event_tree_pop(&ev_t, &ev);
        t = ev.time;
        printf("================================================\n");
        printf("Time stamp: %llu \n", (unsigned long long)t);
        if (ev.ev == EVENT_ARRIVAL) {
            // Step 1: Enqueue all process and dispatch the needed process;
            int entering_proc = 1;
            cfs_enqueue(ev.proc);
            printf("Enqueue PID=%u\n", ev.proc->pid);
            event_t start_ev;
            while (event_tree_peek(&ev_t, &start_ev) && start_ev.ev == EVENT_ARRIVAL && start_ev.time == t) {
                entering_proc++;
                event_tree_pop(&ev_t, &start_ev);
                cfs_enqueue(start_ev.proc);
                printf("Enqueue PID=%u\n", start_ev.proc->pid);
            }

            //Step 2: Try to assigned it to CPU
            while (1) {
                cpu_t *c = cpu_peek();
                pcb_t *p = cfs_pick_next();          
                if (!p || !c) break;
                entering_proc--;
                cpu_dispatch(p, t);
                cfs_dequeue(p);
                printf("Assigned process with PID=%u to CPU %u\n", p->pid, c->cpu_id);
                uint64_t slice = cfs_timeslice(p, cpu_m.total_weight_proc);
                int    idx   = (int)(p - pcbs);
                uint64_t run = slice < (uint64_t)remain[idx] ? slice : (uint64_t)remain[idx];
            
                event_t ev_end = make_event(c, EVENT_END, p, t + run);
                event_tree_insert(&ev_t, &ev_end);
                time_slice[idx] = (int)run;
                c->last_dispatch = t;
            }

            if (entering_proc <= 0) continue; 

            // Step 3: Try to assigned it by preempt other process in CPUs.
            for (int idx = 1; idx <= entering_proc; idx++) {
                int best_idx = -1;
                double best_vruntime = -1;

                for (int i = 0; i < num_cpu; i++) {
                    cpu_t *c = &cpu_m.cpu_list[i];
                    pcb_t *p = c->running_process;
                    if (!p) continue;
                    if (t - c->last_dispatch >= MIN_GRANULARITY_NSEC && 
                        p->vruntime >= best_vruntime) {
                        best_vruntime = p->vruntime;
                        best_idx = i;  
                    }
                }
                if (best_idx != -1) {
                    //Preempt current process on CPU.
                    cpu_t *c = &cpu_m.cpu_list[best_idx];
                    pcb_t *p1 = c->running_process;
                    event_t old_end = make_event(c, EVENT_END, p1, c->last_dispatch + time_slice[p1 - pcbs]);
                    event_delete(&ev_t, &old_end);
                    uint64_t ran = t - c->last_dispatch;
                    remain[p1 - pcbs] -= (int) ran;
                    cfs_task_tick(p1, ran, cpu_m.total_weight_proc);
                    cpu_release(c, ran);
                    if (remain[p1 - pcbs] <= 0) {
                        cfs_dequeue(p1);
                    }
                    c->running_process = NULL;
                    
                    pcb_t *p2 = cfs_pick_next();
                    cpu_dispatch(p2, t);
                    cfs_dequeue(p2);
                    printf("Preempt process PID=%u and entering process PID=%u to CPU %u\n", 
                        p1->pid, p2->pid, c->cpu_id);
                    uint64_t slice = cfs_timeslice(p2, cpu_m.total_weight_proc);
                    int    idx   = (int)(p2 - pcbs);
                    uint64_t run = slice < (uint64_t)remain[idx] ? slice : (uint64_t)remain[idx];
                    event_t ev_end = make_event(c, EVENT_END, p2, t + run);
                    event_tree_insert(&ev_t, &ev_end);
                    time_slice[idx] = (int)run;
                    c->last_dispatch = t;
                }
            }
        } else if (ev.ev == EVENT_END) {
            cpu_t *c = ev.cpu;
            pcb_t *p = ev.proc;
            if (c->running_process != p) continue;

            uint64_t run_done = t - c->last_dispatch;
            int idx = (int)(p - pcbs);
            remain[idx] -= (int)run_done;
            cfs_task_tick(p, run_done, cpu_m.total_weight_proc);
            cpu_release(c, run_done);

            if (remain[idx] == 0) {
                cfs_dequeue(p);
                done++;
                printf("Finish PID=%u\n", p->pid);
            }
            else {
                printf("Expired time-slice of PID=%u in CPU %u\n", p->pid, c->cpu_id);
            }

            pcb_t *next2 = cfs_pick_next();
            if (next2) {
                cpu_dispatch(next2, t);
                cfs_dequeue(next2);
                int idx = 0;
                for (; idx < num_cpu; idx++) {
                    if (cpu_m.cpu_list[idx].running_process == next2) break;
                }
                int ni = (int)(next2 - pcbs);
                uint64_t slice3 = cfs_timeslice(next2, cpu_m.total_weight_proc);
                uint64_t run3 = min(slice3, (uint64_t)remain[ni]);
                event_t ne2 = make_event(&cpu_m.cpu_list[idx], EVENT_END, next2, t + run3);
                event_tree_insert(&ev_t, &ne2);
                time_slice[ni] = (int)run3;
                cpu_m.cpu_list[idx].last_dispatch = t;
                printf("Assigned process with PID=%u to CPU %u\n", next2->pid, idx+1);
            }
        }
    }
    printf("================================================\n");
    printf("All done at Time stamp = %llu\n", (unsigned long long)t);
    event_tree_destroy(&ev_t);
    cpu_destroy();
    free(finished);
    free(time_slice);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pcb_t *pcbs;
    int   *arrival, *remain;
    int    num_process, num_cpu;

    load_processes(argv[1], &pcbs, &arrival, &remain, &num_process, &num_cpu);
    cfs_init_rq();
    simulate_cfs(pcbs, arrival, remain, num_cpu, num_process);

    free(pcbs);
    free(arrival);
    free(remain);
    return EXIT_SUCCESS;
}




// while (done < num_process) {
//     event_t ae;
//     while (heap_peek(&arrivals, &ae) == 0 && ae.time <= t) {
//         heap_pop(&arrivals, &ae);
//         cfs_enqueue(ae.proc);
//         printf("[t=%4llu] enqueue PID=%u\n", (unsigned long long)t, ae.proc->pid);
//     }

//     pcb_t *curr = cfs_pick_next();
//     if (!curr) {
//         if (heap_pop(&arrivals, &ae) == 0) {
//             t = ae.time;
//             cfs_enqueue(ae.proc);
//             printf("[t=%4llu] enqueue PID=%u\n", (unsigned long long)t, ae.proc->pid);
//             printf("\n");
//             continue;
//         }
//         break;
//     }

//     int idx = (int)(curr - pcbs);
//     assert(idx >= 0 && idx < num_process);

//     uint64_t slice = cfs_timeslice(curr);
//     uint64_t rem = remain[idx];
//     uint64_t run_total = slice < rem ? slice : rem;
//     uint64_t run_done = 0;

//     event_t ae2;
//     while (run_done < run_total) {
//         uint64_t next_arr = UINT64_MAX;
//         if (heap_peek(&arrivals, &ae2) == 0) next_arr = ae2.time;

//         uint64_t t_until_arr = next_arr > t ? next_arr - t : 0;
//         uint64_t to_run = run_total - run_done;
//         if (t_until_arr > 0 && to_run > t_until_arr)
//             to_run = t_until_arr;

//         printf("[t=%4llu] run PID=%u for %4llu",
//                (unsigned long long)t, curr->pid,
//                (unsigned long long)to_run);
//         printf("\n");

//         t += to_run;
//         run_done += to_run;

//         // handle any arrivals exactly at new time t
//         while (heap_peek(&arrivals, &ae2) == 0 && ae2.time == t) {
//             heap_pop(&arrivals, &ae2);
//             cfs_enqueue(ae2.proc);
//             printf("[t=%4llu] enqueue PID=%u", (unsigned long long)t, ae2.proc->pid);
//             printf("\n");
//             run_total = cfs_timeslice(curr);
//         }
//         if (run_done >= run_total) {
//             cfs_task_tick(curr, run_done);
//             remain[idx] -= (int)run_done;
//             continue;
//         }
//         // compare curr vs best (without updated vruntime for curr)
//         pcb_t *best = cfs_pick_next();
//         if (best != curr) {
//             printf("[t=%4llu] preempt PID=%u -> PID=%u", (unsigned long long)t, curr->pid, best->pid);
//             printf("\n");
//             cfs_task_tick(curr, run_done);
//             remain[idx] -= (int)run_done;

//             curr = best;
//             idx = (int)(curr - pcbs);
//             slice = cfs_timeslice(curr);
//             rem = remain[idx];
//             run_total = slice < rem ? slice : rem;
//             run_done = 0;
//             break; 
//         }
//     }

//     if (remain[idx] == 0) {
//         cfs_dequeue(curr);
//         finished[idx] = true;
//         done++;
//         printf("[t=%4llu] finish PID=%u\n", (unsigned long long)t, curr->pid);
//     }
// }

// printf("All done at t=%llu\n", (unsigned long long)t);
// heap_free(&arrivals);
// free(finished);




            // //Step 2: Recalculate time-slice
            // for (int i = 0; i < num_cpu; i++) {
            //     cpu_t *c = &cpu_m.cpu_list[i];
            //     pcb_t *p = c->running_process;
            //     if (!p) continue;
            //     uint64_t run_done  = t - c->last_dispatch;
            //     int      idx       = (int)(p - pcbs);
            //     uint64_t slice     = cfs_timeslice(p, cpu_m.total_weight_proc);
            //     uint64_t run_for   = slice < (uint64_t)remain[idx] ? slice : (uint64_t)remain[idx];
            //     event_t old_ev = {
            //         .ev   = EVENT_END,
            //         .time = c->last_dispatch + run_for,
            //         .proc = p,
            //         .cpu  = c
            //     };
            //     event_delete(&ev_t, &old_ev);
            //     if (run_done < run_for) {
            //         uint64_t remaining = run_for - run_done;
            //         event_t new_ev = {
            //             .ev   = EVENT_END,
            //             .time = t + remaining,
            //             .proc = p,
            //             .cpu  = c
            //         };
            //         event_tree_insert(&ev_t, &new_ev);
            //         c->running_time = new_ev.time;
            //     } else {
            //         event_t new_ev = {
            //             .ev   = EVENT_END,
            //             .time = t,
            //             .proc = p,
            //             .cpu  = c
            //         };
            //         event_tree_insert(&ev_t, &new_ev);
            //         c->running_time = t;
            //     }
            // }
            // //Step 3: Release all CPU end time-slice
            // event_t end_ev;
            // while (event_tree_peek(&ev_t, &end_ev) 
            //     && end_ev.ev == EVENT_END 
            //     && end_ev.time == t) 
            // {
            //     event_tree_pop(&ev_t, &end_ev);
            //     cpu_t   *c    = end_ev.cpu;
            //     pcb_t   *p    = end_ev.proc;
            //     int       idx = (int)(p - pcbs);
            //     uint64_t  run_done = t - c->last_dispatch;

            //     remain[idx] -= (int)run_done;
            //     cfs_task_tick(p, run_done);
            //     cpu_release(c, run_done);

            //     if (remain[idx] == 0) {
            //         done++;
            //         cfs_dequeue(p);
            //     }
            // }
            // //Step 4: Enqueue new process into CPU
            // while (1) {
            //     cpu_t *idle = cpu_peek();
            //     pcb_t *next = cfs_pick_next();
            //     if (!idle || !next) break;
        
            //     cpu_dispatch(next, t);
        
            //     int idx2 = (int)(next - pcbs);
            //     uint64_t slice2   = cfs_timeslice(next, cpu_m.total_weight_proc);
            //     uint64_t run_for2 = slice2 < (uint64_t)remain[idx2]
            //                         ? slice2
            //                         : (uint64_t)remain[idx2];
        
            //     event_t ne = {
            //         .ev   = EVENT_END,
            //         .time = t + run_for2,
            //         .proc = next,
            //         .cpu  = idle
            //     };
            //     event_tree_insert(&ev_t, &ne);
        
            //     idle->last_dispatch  = t;
            // }
            // //Step 5: Maximum: The new process need to enter the CPU
            // int best_idx = -1;
            // double best = -1;
            // pcb_t* next = cfs_pick_next();
            // if (next->vruntime != 0) continue;
            // for (int i = 0; i < num_cpu; i++) {
            //     if (cpu_m.cpu_list[i].running_process != NULL && t - cpu_m.cpu_list[i].last_dispatch >= MIN_GRANULARITY_NSEC && 
            //         cpu_m.cpu_list[i].running_process->vruntime >= best) {
            //         best_idx = i;
            //     }
            // }
            // //If not found, prepare for the first process reach and the biggest runtime
            // best = -1;
            // if (best_idx == -1) {
            //     for (int i = 0; i < num_cpu; i++) {
            //         if (cpu_m.cpu_list[i].running_process != NULL &&
            //             t - cpu_m.cpu_list[i].last_dispatch >= best) {

            //             if (t - cpu_m.cpu_list[i].last_dispatch == best) {
            //                 best_idx = cpu_m.cpu_list[i].running_process->vruntime > cpu_m.cpu_list[best_idx].running_process->vruntime 
            //                 ? i : best_idx;
            //             }
            //             best = t - cpu_m.cpu_list[i].last_dispatch;
            //             best_idx = i;
            //         }
            //     }
            //     cfs_dequeue(ev.proc);
            //     event_t key3 = {
            //         .cpu = NULL,
            //         .ev = EVENT_ARRIVAL,
            //         .proc = ev.proc,
            //         .time = t + MIN_GRANULARITY_NSEC
            //     };
            //     event_t key4 = {
            //         .cpu = &cpu_m.cpu_list[i];
            //         .proc = 
            //     }
            //     event_tree_insert(&ev_t, &key3);
            // }
            // //Preempt the process with maximum vruntime in list cpu
            