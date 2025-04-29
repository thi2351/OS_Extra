// heap.h
#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdint.h>

/**
 * Generic binary heap (min-heap or max-heap depending on cmp).
 * Stores elements of fixed size.
 */
typedef struct {
    void   *data;       /* pointer to elements array */
    size_t  size;       /* number of elements in heap */
    size_t  capacity;   /* allocated capacity */
    size_t  elem_size;  /* size of each element */
    /**
     * Comparison function:
     *   return <0 if a < b,
     *          =0 if a == b,
     *          >0 if a > b
     */
    int    (*cmp)(const void *a, const void *b);
} heap_t;

/**
 * Initialize a heap.
 * @h           pointer to heap_t
 * @elem_size   size of each element in bytes
 * @capacity    initial capacity (# of elements)
 * @cmp         comparison function
 * @return 0 on success, -1 on allocation failure
 */
int heap_init(heap_t *h,
              size_t elem_size,
              size_t capacity,
              int (*cmp)(const void *a, const void *b));

/**
 * Push an element onto the heap.
 * @h     pointer to heap
 * @elem  pointer to element to insert
 * @return 0 on success, -1 on allocation failure
 */
int heap_push(heap_t *h, const void *elem);

/**
 * Pop the top element from the heap into out.
 * @h    pointer to heap
 * @out  pointer to buffer where popped element is copied
 * @return 0 on success, -1 if heap empty
 */
int heap_pop(heap_t *h, void *out);

/**
 * Peek at the top element without removing it.
 * @h    pointer to heap
 * @out  pointer to buffer where top element is copied
 * @return 0 on success, -1 if heap empty
 */
int heap_peek(const heap_t *h, void *out);

/**
 * Free heap resources.
 */
void heap_free(heap_t *h);

#endif // HEAP_H