#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

//Simplify pcb_t for CFS_SCHED


typedef struct pcb_t {
    uint32_t pid;
    double    vruntime;
    uint32_t weight;
} pcb_t;

typedef struct cpu {
    uint32_t cpu_id;
    uint32_t running_time;
    pcb_t* running_process;
    uint32_t last_dispatch;
} cpu_t;



#endif
