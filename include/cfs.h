#ifndef CFS_H
#define CFS_H

#include <stdint.h>
#include <pthread.h>
#include "rbtree.h"
#include "common.h"

#define SCHED_LATENCY_NSEC   200ULL
#define MIN_GRANULARITY_NSEC 10ULL
#define WEIGHT_NORM          1024.0

struct cfs_rq {
    RBTree          *tree;
    uint64_t         total_weight;
    pthread_mutex_t  rq_lock;
};

extern struct cfs_rq cfs_rq;

void     cfs_init_rq(void);
uint32_t cfs_compute_weight(int nice);
void     cfs_enqueue(pcb_t *p);
void     cfs_dequeue(pcb_t *p);
pcb_t   *cfs_pick_next(void);
uint64_t cfs_timeslice(pcb_t *p);
void     cfs_update_vruntime(pcb_t *p, uint64_t delta_ns);
void     cfs_task_tick(pcb_t *p, uint64_t elapsed_ns);

#endif
