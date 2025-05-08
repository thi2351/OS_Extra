#include <stdlib.h>
#include "cpu.h"
#include "heap.h"

cpu_manager cpu_m;

void cpu_init(int n) {
    cpu_m.n = n;
    cpu_m.total_weight_proc = 0;

    // Cấp phát mảng cpu_t
    cpu_m.cpu_list = malloc(n * sizeof(cpu_t));

    // Khởi tạo heap lưu con trỏ cpu_t*
    heap_init(&cpu_m.cpu_heap, sizeof(cpu_t *), n, cpu_freecmp);

    for (int i = 0; i < n; ++i) {
        cpu_t *ptr = &cpu_m.cpu_list[i];
        ptr->cpu_id          = i + 1;
        ptr->running_time    = 0;
        ptr->running_process = NULL;
        ptr->last_dispatch   = 0;
        heap_push(&cpu_m.cpu_heap, &ptr);  
    }
}

void cpu_destroy(void) {
    heap_free(&cpu_m.cpu_heap);
    free(cpu_m.cpu_list);
    cpu_m.cpu_list = NULL;
    cpu_m.n = 0;
    cpu_m.total_weight_proc = 0;
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
    heap_push(&cpu_m.cpu_heap, &c);  // push địa chỉ của cpu_t*
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
    c->running_time += (current_time - c->last_dispatch);
    pcb_t *p = c->running_process;
    if (p) {
        cpu_m.total_weight_proc -= p->weight;
    }
    c->running_process = NULL;
    cpu_push(c);  // đưa CPU trở lại heap
    return 0;
}
