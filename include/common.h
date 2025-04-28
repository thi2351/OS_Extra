#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

//Simplify pcb_t for CFS_SCHED


typedef struct pcb_t {
    uint32_t pid;
    uint64_t vruntime;
    uint32_t weight;
} pcb_t;

#endif
