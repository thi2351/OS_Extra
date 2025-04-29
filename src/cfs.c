#include "cfs.h"
#include "rbtree.h"
#include <pthread.h>
#include <stdlib.h>

struct cfs_rq cfs_rq;

/**
 * Comparator for CFS run-queue: compare by vruntime (double), then weight (higher first), then pid.
 */
static int cfs_cmp(const void *a, const void *b) {
    const pcb_t *p1 = a;
    const pcb_t *p2 = b;
    double v1 = p1->vruntime;
    double v2 = p2->vruntime;
    if (v1 < v2) return -1;
    if (v1 > v2) return  1;
    if (p1->weight > p2->weight) return -1;
    if (p1->weight < p2->weight) return  1;
    if (p1->pid < p2->pid) return -1;
    if (p1->pid > p2->pid) return  1;
    return 0;
}

/**
 * Helper to find the minimum node in RBTree (leftmost).
 */
static pcb_t *cfs_tree_min(void) {
    RBNode *node = cfs_rq.tree->root;
    if (!node) return NULL;
    while (node->left) node = node->left;
    return (pcb_t *)node->data;
}

void cfs_init_rq(void) {
    cfs_rq.tree = new_rbtree(cfs_cmp, NULL, NULL);
    cfs_rq.total_weight = 0;
    pthread_mutex_init(&cfs_rq.rq_lock, NULL);
}

static const uint32_t nice_to_weight[40] = {
    88761, 71755, 56483, 46273, 36291, 29154, 23254, 18705, 14949, 11916,
     9548,  7620,  6100,  4904,  3906,  3121,  2501,  1991,  1586,  1277,
     1024,   820,   655,   526,   423,   335,   272,   215,   172,   137,
      110,    87,    70,    56,    45,    36,    29,    23,    18,    15
};

uint32_t cfs_compute_weight(int nice) {
    if (nice < -20) nice = -20;
    if (nice >  19) nice =  19;
    return nice_to_weight[nice + 20];
}

void cfs_enqueue(pcb_t *p) {
    pthread_mutex_lock(&cfs_rq.rq_lock);
    rbtree_insert(cfs_rq.tree, p);
    cfs_rq.total_weight += p->weight;
    pthread_mutex_unlock(&cfs_rq.rq_lock);
}

void cfs_dequeue(pcb_t *p) {
    pthread_mutex_lock(&cfs_rq.rq_lock);
    rbtree_delete(cfs_rq.tree, p);
    cfs_rq.total_weight -= p->weight;
    pthread_mutex_unlock(&cfs_rq.rq_lock);
}

pcb_t *cfs_pick_next(void) {
    pthread_mutex_lock(&cfs_rq.rq_lock);
    pcb_t *p = cfs_tree_min();
    pthread_mutex_unlock(&cfs_rq.rq_lock);
    return p;
}

uint64_t cfs_timeslice(pcb_t *p) {
    uint64_t total = cfs_rq.total_weight ? cfs_rq.total_weight  : 1;
    uint64_t slice = (SCHED_LATENCY_NSEC * p->weight) / total;
    return (slice < MIN_GRANULARITY_NSEC ? MIN_GRANULARITY_NSEC : slice);
}

void cfs_update_vruntime(pcb_t *p, uint64_t delta_ns) {
    // virtual runtime is double now
    double proportion = (double)delta_ns * WEIGHT_NORM / (double)p->weight;
    p->vruntime += proportion;
}

void cfs_task_tick(pcb_t *p, uint64_t elapsed_ns) {
    if (!p) return;
    cfs_dequeue(p);
    cfs_update_vruntime(p, elapsed_ns);
    cfs_enqueue(p);
}
