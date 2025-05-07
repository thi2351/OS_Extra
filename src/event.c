#include <stdlib.h>
#include <string.h>
#include "event.h"

static int ev_cmp(void *a, void *b) {
    const event_t *A = a, *B = b;
    if (A->time < B->time) return -1;
    if (A->time > B->time) return +1;
    if (A->ev != B->ev)
      return (A->ev == EVENT_END ? -1 : +1);
    if (A->ev == EVENT_ARRIVAL) {
        if (A->proc < B->proc) return -1;
        if (A->proc > B->proc) return +1;
        return 0;
    } else {
        if (A->cpu < B->cpu) return -1;
        if (A->cpu > B->cpu) return +1;
        return 0;
    }
}


static void *clone_event(void *data) {
    event_t *src = data;
    event_t *dst = malloc(sizeof(*dst));
    memcpy(dst, src, sizeof(*dst));
    return dst;
}

static void free_event(void *data) {
    free(data);
}

void event_tree_init(event_tree *et) {
    et->tree = new_rbtree(ev_cmp, clone_event, free_event);
}

void event_tree_destroy(event_tree *et) {
    destroy_rbtree(et->tree);
    et->tree = NULL;
}

void event_tree_insert(event_tree *et, const event_t *ev) {
    rbtree_insert(et->tree, (void*)ev);
}

static void *rbtree_find_min_node(RBTree *tr) {
    RBNode *n = tr->root;
    if (!n) return NULL;
    while (n->left) n = n->left;
    return n->data;
}

bool event_tree_peek(const event_tree *et, event_t *out_ev) {
    void *d = rbtree_find_min_node(et->tree);
    if (!d) return false;
    *out_ev = *(event_t*)d;
    return true;
}

bool event_tree_pop(event_tree *et, event_t *out_ev) {
    void *d = rbtree_find_min_node(et->tree);
    if (!d) return false;
    *out_ev = *(event_t*)d;
    rbtree_delete(et->tree, d);
    return true;
}

int event_delete(event_tree *et, const event_t *to_del) {
    event_t *found = (event_t *)rbtree_search(et->tree, (void *)to_del);
    if (found) {
        rbtree_delete(et->tree, found);
        return 0;
    }
    return -1;
}