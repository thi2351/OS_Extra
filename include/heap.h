// heap.h
#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    void   *data;       /* pointer to elements array */
    size_t  size;       /* number of elements in heap */
    size_t  capacity;   /* allocated capacity */
    size_t  elem_size;  /* size of each element */
    int    (*cmp)(const void *a, const void *b);
} heap_t;

int heap_init(heap_t *h,
              size_t elem_size,
              size_t capacity,
              int (*cmp)(const void *a, const void *b));
int heap_push(heap_t *h, const void *elem);
int heap_pop(heap_t *h, void *out);
int heap_peek(const heap_t *h, void *out);
void heap_free(heap_t *h);

#endif // HEAP_H
