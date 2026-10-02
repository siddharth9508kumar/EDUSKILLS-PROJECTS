/**
 * @file heap.c
 * @brief Array-backed binary min-heap of Event records, ordered by
 *        start_time with Priority as a tie-breaker.
 */

#include "scheduler.h"

/**
 * @brief Compare two events to decide heap ordering.
 *
 * An event is considered "greater" (i.e. should sit further from the root)
 * than another if it starts later, or - when start times are equal - if it
 * has a lower Priority value.
 *
 * @param a First event.
 * @param b Second event.
 * @return Non-zero if a is strictly "greater" than b for heap-ordering
 *         purposes, zero otherwise.
 */
static int event_is_greater(const Event *a, const Event *b) {
    if (a->start_time != b->start_time) {
        return a->start_time > b->start_time;
    }
    return a->priority < b->priority;
}

/**
 * @brief Swap two Event records in place.
 * @param a First event.
 * @param b Second event.
 */
static void swap_events(Event *a, Event *b) {
    Event tmp = *a;
    *a = *b;
    *b = tmp;
}

MinHeap *create_heap(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = HEAP_INITIAL_CAPACITY;
    }

    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    if (heap == NULL) {
        return NULL;
    }

    heap->events = (Event *)malloc(initial_capacity * sizeof(Event));
    if (heap->events == NULL) {
        free(heap);
        return NULL;
    }

    heap->size = 0;
    heap->capacity = initial_capacity;
    heap->next_id = 1;
    return heap;
}

void free_heap(MinHeap *heap) {
    if (heap == NULL) {
        return;
    }
    free(heap->events);
    heap->events = NULL;
    heap->size = 0;
    heap->capacity = 0;
    free(heap);
}

void heapify_up(MinHeap *heap, size_t idx) {
    if (heap == NULL) {
        return;
    }
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (event_is_greater(&heap->events[parent], &heap->events[idx])) {
            swap_events(&heap->events[parent], &heap->events[idx]);
            idx = parent;
        } else {
            break;
        }
    }
}

void heapify_down(MinHeap *heap, size_t idx) {
    if (heap == NULL) {
        return;
    }
    for (;;) {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t smallest = idx;

        if (left < heap->size && event_is_greater(&heap->events[smallest], &heap->events[left])) {
            smallest = left;
        }
        if (right < heap->size && event_is_greater(&heap->events[smallest], &heap->events[right])) {
            smallest = right;
        }
        if (smallest == idx) {
            break;
        }
        swap_events(&heap->events[idx], &heap->events[smallest]);
        idx = smallest;
    }
}

int insert_event(MinHeap *heap, Event event) {
    if (heap == NULL) {
        return -1;
    }

    if (heap->size == heap->capacity) {
        size_t new_capacity = heap->capacity * 2;
        Event *resized = (Event *)realloc(heap->events, new_capacity * sizeof(Event));
        if (resized == NULL) {
            return -1; /* Original block is untouched and still valid/freeable. */
        }
        heap->events = resized;
        heap->capacity = new_capacity;
    }

    heap->events[heap->size] = event;
    heapify_up(heap, heap->size);
    heap->size += 1;
    return 0;
}

int extract_min(MinHeap *heap, Event *out) {
    if (heap == NULL || out == NULL || heap->size == 0) {
        return -1;
    }

    *out = heap->events[0];
    heap->size -= 1;

    if (heap->size > 0) {
        heap->events[0] = heap->events[heap->size];
        heapify_down(heap, 0);
    }
    return 0;
}

const Event *peek_min(const MinHeap *heap) {
    if (heap == NULL || heap->size == 0) {
        return NULL;
    }
    return &heap->events[0];
}

size_t find_index_by_id(const MinHeap *heap, int id) {
    if (heap == NULL) {
        return (size_t)-1;
    }
    for (size_t i = 0; i < heap->size; i++) {
        if (heap->events[i].id == id) {
            return i;
        }
    }
    return (size_t)-1;
}

Event *find_event_by_id(MinHeap *heap, int id) {
    size_t idx = find_index_by_id(heap, id);
    if (idx == (size_t)-1) {
        return NULL;
    }
    return &heap->events[idx];
}

int remove_event_by_id(MinHeap *heap, int id, Event *out) {
    if (heap == NULL) {
        return -1;
    }

    size_t idx = find_index_by_id(heap, id);
    if (idx == (size_t)-1) {
        return -1;
    }

    if (out != NULL) {
        *out = heap->events[idx];
    }

    size_t last = heap->size - 1;
    if (idx != last) {
        heap->events[idx] = heap->events[last];
        heap->size -= 1;
        /* The relocated element may need to move either up or down. */
        heapify_down(heap, idx);
        heapify_up(heap, idx);
    } else {
        heap->size -= 1;
    }

    return 0;
}

/**
 * @brief qsort() comparator ordering events chronologically by start_time.
 * @param a Pointer to first Event.
 * @param b Pointer to second Event.
 * @return Negative, zero or positive as a's start_time is less than, equal
 *         to, or greater than b's.
 */
static int compare_events_chronological(const void *a, const void *b) {
    const Event *ea = (const Event *)a;
    const Event *eb = (const Event *)b;
    if (ea->start_time < eb->start_time) return -1;
    if (ea->start_time > eb->start_time) return 1;
    /* Tie-break by priority (URGENT first) for a stable, useful ordering. */
    return (int)eb->priority - (int)ea->priority;
}

Event *get_sorted_snapshot(const MinHeap *heap, size_t *count) {
    if (count != NULL) {
        *count = 0;
    }
    if (heap == NULL || heap->size == 0 || count == NULL) {
        return NULL;
    }

    Event *copy = (Event *)malloc(heap->size * sizeof(Event));
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, heap->events, heap->size * sizeof(Event));
    qsort(copy, heap->size, sizeof(Event), compare_events_chronological);

    *count = heap->size;
    return copy;
}
