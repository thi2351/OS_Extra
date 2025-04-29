#include "heap.h"
#include <string.h>
#include <stdlib.h>
static void swap_elems(heap_t *h, size_t i, size_t j) {
    char *base = (char*)h->data;
    char *a = base + i * h->elem_size;
    char *b = base + j * h->elem_size;
    char *tmp = malloc(h->elem_size);
    memcpy(tmp, a, h->elem_size);
    memcpy(a, b, h->elem_size);
    memcpy(b, tmp, h->elem_size);
    free(tmp);
}

int heap_init(heap_t *h, size_t elem_size, size_t capacity,
              int (*cmp)(const void *a, const void *b)) {
    h->data = malloc(elem_size * capacity);
    if (!h->data) return -1;
    h->size = 0;
    h->capacity = capacity;
    h->elem_size = elem_size;
    h->cmp = cmp;
    return 0;
}

int heap_push(heap_t *h, const void *elem) {
    if (h->size == h->capacity) {
        size_t newcap = h->capacity ? h->capacity * 2 : 1;
        void *newdata = realloc(h->data, newcap * h->elem_size);
        if (!newdata) return -1;
        h->data = newdata;
        h->capacity = newcap;
    }
    /* copy element at end */
    char *base = (char*)h->data;
    memcpy(base + h->size * h->elem_size, elem, h->elem_size);
    /* up-heap */
    size_t i = h->size++;
    while (i > 0) {
        size_t parent = (i - 1) >> 1;
        char *p = base + parent * h->elem_size;
        char *c = base + i * h->elem_size;
        if (h->cmp(p, c) <= 0) break;
        swap_elems(h, parent, i);
        i = parent;
    }
    return 0;
}

int heap_pop(heap_t *h, void *out) {
    if (h->size == 0) return -1;
    char *base = (char*)h->data;
    /* copy root */
    memcpy(out, base, h->elem_size);
    /* move last to root */
    memcpy(base, base + (h->size - 1) * h->elem_size, h->elem_size);
    h->size--;
    /* down-heap */
    size_t i = 0;
    while (1) {
        size_t left = 2*i + 1;
        size_t right = 2*i + 2;
        size_t smallest = i;
        char *s = base + smallest * h->elem_size;
        if (left < h->size) {
            char *l = base + left * h->elem_size;
            if (h->cmp(l, s) < 0) {
                smallest = left;
                s = l;
            }
        }
        if (right < h->size) {
            char *r = base + right * h->elem_size;
            if (h->cmp(r, s) < 0) {
                smallest = right;
            }
        }
        if (smallest == i) break;
        swap_elems(h, i, smallest);
        i = smallest;
    }
    return 0;
}

int heap_peek(const heap_t *h, void *out) {
    if (h->size == 0) return -1;
    memcpy(out, h->data, h->elem_size);
    return 0;
}

void heap_free(heap_t *h) {
    free(h->data);
    h->data = NULL;
    h->size = h->capacity = 0;
    h->elem_size = 0;
    h->cmp = NULL;
}
