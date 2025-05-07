#include "common.h"
#include "heap.h"

typedef struct {
    cpu_t* cpu_list;
    heap_t cpu_heap;
    uint32_t total_weight_proc;
    int n;
} cpu_manager;

//Dùng cpu ít sử dụng nhất
static int cpu_freecmp(const void *a, const void *b) {
    const cpu_t *c1 = a, *c2 = b;
    if (c1->running_time < c2->running_time) return -1;
    if (c1->running_time > c2->running_time) return  1;

    if (c1->cpu_id      < c2->cpu_id     ) return -1;
    if (c1->cpu_id      > c2->cpu_id     ) return  1;
    return 0;
}


extern cpu_manager cpu_m;

void   cpu_init(int n);
cpu_t *cpu_peek(void);
cpu_t *cpu_pop(void);
int    cpu_dispatch(pcb_t* p, int current_time);
int    cpu_release(cpu_t* c, int current_time);






