#include <stdlib.h>
#include "cpu.h"
#include "heap.h"

cpu_manager cpu_m;

void cpu_init(int n) {
    cpu_m.n = n;
    // allocate array of cpu_t structs
    cpu_m.cpu_list = malloc(n * sizeof(cpu_t));
    // heap will store cpu_t* pointers
    heap_init(&cpu_m.cpu_heap, sizeof(cpu_t*), n, cpu_freecmp);
    for (int i = 0; i < n; i++) {
        cpu_m.cpu_list[i].cpu_id          = i + 1;
        cpu_m.cpu_list[i].running_time    = 0;
        cpu_m.cpu_list[i].running_process = NULL;
        cpu_m.cpu_list[i].last_dispatch   = 0;
        // push pointer to this cpu onto heap
        cpu_t *ptr = &cpu_m.cpu_list[i];
        heap_push(&cpu_m.cpu_heap, &ptr);
    }
}

void cpu_destroy(void) {
    heap_free(&cpu_m.cpu_heap);
    free(cpu_m.cpu_list);
    cpu_m.n = 0;
}

cpu_t *cpu_peek(void) {
    cpu_t *c;
    return heap_peek(&cpu_m.cpu_heap, &c) == 0 ? c : NULL;
}

cpu_t *cpu_pop(void) {
    cpu_t *c;
    return heap_pop(&cpu_m.cpu_heap, &c) == 0 ? c : NULL;
}

void cpu_push(cpu_t *c) {
    heap_push(&cpu_m.cpu_heap, &c);
}

int cpu_dispatch(pcb_t *p, int current_time) {
    cpu_t *c;
    if (heap_pop(&cpu_m.cpu_heap, &c) != 0) {
        return -1;
    }
    c->running_process = p;
    c->last_dispatch   = current_time;
    cpu_m.total_weight_proc += p->weight;
    return 0;
}

int cpu_release(cpu_t *c, int current_time) {
    c->running_time    += (current_time - c->last_dispatch);
    pcb_t* p = c->running_process;
    if (p) cpu_m.total_weight_proc -= p->weight;
    c->running_process = NULL;
    
    heap_push(&cpu_m.cpu_heap, &c);
    return 0;
}
