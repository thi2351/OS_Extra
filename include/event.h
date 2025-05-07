#ifndef EVENT_TREE_H
#define EVENT_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "rbtree.h"

typedef enum { EVENT_ARRIVAL, EVENT_END } event_type;

typedef struct {
    event_type ev;
    uint64_t   time;
    pcb_t      *proc;
    cpu_t      *cpu;
} event_t;

typedef struct {
    RBTree *tree;
} event_tree;


void event_tree_init(event_tree *et);
void event_tree_destroy(event_tree *et);
void event_tree_insert(event_tree *et, const event_t *ev);
bool event_tree_peek(const event_tree *et, event_t *out_ev);
bool event_tree_pop(event_tree *et, event_t *out_ev);
int event_delete(event_tree *et, const event_t *to_del);

#endif // EVENT_TREE_H
