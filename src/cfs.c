#include "cfs.h"   
#include "rbtree.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

struct cfs_rq cfs_rq;

static int cfs_cmp(void *a, void *b) {
    struct pcb_t *p1 = (struct pcb_t*)a;
    struct pcb_t *p2 = (struct pcb_t*)b;
    uint64_t v1 = p1->vruntime;
    uint64_t v2 = p2->vruntime;
    if (v1 < v2) return -1;
    if (v1 > v2) return 1;
    if (p1->pid < p2->pid) return -1;
    if (p1->pid > p2->pid) return 1;
    return 0;
}
static struct pcb_t *cfs_tree_min(void) {
    RBNode *node = cfs_rq.tree->root;
    if (!node) return NULL;
    while (node->left)
        node = node->left;
    return (struct pcb_t*)node->data;
}

void cfs_init_rq(void) {
    cfs_rq.tree = new_rbtree(cfs_cmp, NULL, NULL);
    cfs_rq.total_weight = 0;
    pthread_mutex_init(&cfs_rq.rq_lock, NULL);
}

uint32_t cfs_compute_weight(int nice) {
    /* clamp nice vào [-20, 19] */
    if (nice < -20) nice = -20;
    if (nice >  19) nice = 19;

    /* exponent = (-nice) / 10.0 */
    float exp = (-nice) / 10.0f;

    /* weight = WEIGHT_NORM * 2^exp */
    float w = WEIGHT_NORM * powf(2.0f, exp);

    /* làm tròn xuống int (hoặc dùng roundf để làm tròn gần nhất) */
    return (uint32_t)(w + 0.5f);
}

void cfs_enqueue(struct pcb_t *p) {
    pthread_mutex_lock(&cfs_rq.rq_lock);
    rbtree_insert(cfs_rq.tree, p);
    cfs_rq.total_weight += p->weight;
    pthread_mutex_unlock(&cfs_rq.rq_lock);
}

void cfs_dequeue(struct pcb_t *p) {
    pthread_mutex_lock(&cfs_rq.rq_lock);
    rbtree_delete(cfs_rq.tree, p);
    cfs_rq.total_weight -= p->weight;
    pthread_mutex_unlock(&cfs_rq.rq_lock);
}

struct pcb_t *cfs_pick_next(void) {
    struct pcb_t *p;
    pthread_mutex_lock(&cfs_rq.rq_lock);
    p = cfs_tree_min();
    pthread_mutex_unlock(&cfs_rq.rq_lock);
    return p;
}

uint64_t cfs_timeslice(struct pcb_t *p) {
    uint64_t slice = (SCHED_LATENCY_NSEC * p->weight)
                     / (cfs_rq.total_weight ?: 1);
    return (slice < MIN_GRANULARITY_NSEC ? MIN_GRANULARITY_NSEC : slice);
}

void cfs_update_vruntime(struct pcb_t *p, uint64_t delta_ns) {
    p->vruntime += (delta_ns * WEIGHT_NORM)
                            / (p->weight ?: WEIGHT_NORM);
}

void cfs_task_tick(struct pcb_t *p, uint64_t elapsed_ns) {
    if (!p) return;
    cfs_dequeue(p);
    cfs_update_vruntime(p, elapsed_ns);
    cfs_enqueue(p);
}
