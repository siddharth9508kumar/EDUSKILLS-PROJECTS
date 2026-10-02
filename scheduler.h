/**
 * @file scheduler.h
 * @brief Public interface for the Real-Time Event Scheduler.
 *
 * This header declares all data structures, enumerations and function
 * prototypes shared across heap.c, utils.c and main.c. It intentionally
 * contains no executable code (except static inline helpers where noted)
 * so that it can be safely included from every translation unit.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

#define TITLE_MAX_LEN       100  /**< Maximum length (incl. NUL) of an event title. */
#define DESCRIPTION_MAX_LEN 256  /**< Maximum length (incl. NUL) of an event description. */
#define HEAP_INITIAL_CAPACITY 16 /**< Initial number of slots allocated for the heap array. */
#define LINE_BUFFER_SIZE    512  /**< Size of the scratch buffer used for console input. */

/* ------------------------------------------------------------------------
 * Enumerations
 * ---------------------------------------------------------------------- */

/**
 * @brief Relative importance of an event.
 *
 * The numeric value is used both for display and as a min-heap tie-breaker:
 * when two events share the same start_time, the one with the higher
 * Priority value (e.g. URGENT) is considered "smaller" and is served first.
 */
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_URGENT = 3
} Priority;

/**
 * @brief How (if at all) an event repeats after it fires.
 */
typedef enum {
    RECUR_NONE = 0,
    RECUR_DAILY = 1,
    RECUR_WEEKLY = 2,
    RECUR_MONTHLY = 3
} RecurrencePattern;

/* ------------------------------------------------------------------------
 * Core data structures
 * ---------------------------------------------------------------------- */

/**
 * @brief A single scheduled event.
 *
 * All character data is stored inline (fixed-size arrays) rather than via
 * pointers, so an Event can be copied by value with a plain assignment and
 * never needs its own free() call.
 */
typedef struct {
    int id;                                 /**< Unique identifier, assigned by the scheduler. */
    char title[TITLE_MAX_LEN];              /**< Short human-readable title. */
    char description[DESCRIPTION_MAX_LEN];  /**< Longer free-text description. */
    time_t start_time;                      /**< Epoch timestamp the event is scheduled for. */
    int duration_minutes;                   /**< Duration of the event, in minutes. */
    Priority priority;                      /**< Relative importance. */
    RecurrencePattern recurrence;           /**< Recurrence rule, if any. */
} Event;

/**
 * @brief Array-backed binary min-heap of Event records, ordered by start_time
 *        (with Priority used to break ties).
 *
 * The heap owns its backing array (events); it does not own any external
 * memory, since Event itself contains no pointers.
 */
typedef struct {
    Event *events;     /**< Dynamically allocated array of heap-ordered events. */
    size_t size;        /**< Number of events currently stored. */
    size_t capacity;     /**< Number of slots currently allocated in events. */
    int next_id;        /**< Next unique id to hand out to a newly created event. */
} MinHeap;

/* ------------------------------------------------------------------------
 * heap.c - Min-heap priority queue
 * ---------------------------------------------------------------------- */

/**
 * @brief Allocate and initialize a new, empty min-heap.
 * @param initial_capacity Number of Event slots to pre-allocate (must be > 0).
 * @return Pointer to a heap-allocated MinHeap, or NULL on allocation failure.
 */
MinHeap *create_heap(size_t initial_capacity);

/**
 * @brief Release all memory owned by a heap, including the heap struct itself.
 * @param heap Heap to destroy. Safe to call with NULL (no-op).
 */
void free_heap(MinHeap *heap);

/**
 * @brief Insert a new event into the heap, preserving the min-heap invariant.
 * @param heap Target heap (must not be NULL).
 * @param event Event to copy into the heap.
 * @return 0 on success, -1 if the heap needed to grow and allocation failed.
 */
int insert_event(MinHeap *heap, Event event);

/**
 * @brief Remove and return the event with the earliest start_time.
 * @param heap Source heap.
 * @param out  On success, populated with the removed event.
 * @return 0 on success, -1 if the heap is empty or arguments are invalid.
 */
int extract_min(MinHeap *heap, Event *out);

/**
 * @brief Look at (without removing) the event with the earliest start_time.
 * @param heap Heap to inspect.
 * @return Pointer to the root event (owned by the heap, valid until the next
 *         mutation), or NULL if the heap is empty or NULL.
 */
const Event *peek_min(const MinHeap *heap);

/**
 * @brief Restore the min-heap property by sifting the element at idx upward.
 * @param heap Heap being repaired.
 * @param idx  Index of the element that may violate the heap property.
 */
void heapify_up(MinHeap *heap, size_t idx);

/**
 * @brief Restore the min-heap property by sifting the element at idx downward.
 * @param heap Heap being repaired.
 * @param idx  Index of the element that may violate the heap property.
 */
void heapify_down(MinHeap *heap, size_t idx);

/**
 * @brief Find the array index of the event with the given id.
 * @param heap Heap to search.
 * @param id   Event id to look for.
 * @return Index within heap->events, or (size_t)-1 if not found.
 */
size_t find_index_by_id(const MinHeap *heap, int id);

/**
 * @brief Find an event by id without removing it.
 * @param heap Heap to search.
 * @param id   Event id to look for.
 * @return Pointer to the event inside the heap's backing array, or NULL if
 *         not found. The pointer is invalidated by any later insert/remove.
 */
Event *find_event_by_id(MinHeap *heap, int id);

/**
 * @brief Remove a specific event by id, wherever it sits in the heap.
 * @param heap Heap to remove from.
 * @param id   Id of the event to remove.
 * @param out  If non-NULL, populated with the removed event on success.
 * @return 0 on success, -1 if no event with that id exists.
 */
int remove_event_by_id(MinHeap *heap, int id, Event *out);

/**
 * @brief Produce a chronologically sorted snapshot of all events in the heap.
 *
 * The heap itself is left untouched; the returned array is a fresh copy
 * that the caller is responsible for freeing.
 *
 * @param heap  Heap to snapshot.
 * @param count Output parameter receiving the number of events copied.
 * @return Newly malloc'd array of length *count (chronological order), or
 *         NULL if the heap is empty or NULL (in which case *count is 0).
 */
Event *get_sorted_snapshot(const MinHeap *heap, size_t *count);

/* ------------------------------------------------------------------------
 * utils.c - time handling, input helpers, recurrence math
 * ---------------------------------------------------------------------- */

/**
 * @brief Read a full line of text from stdin into a fixed-size buffer,
 *        stripping the trailing newline and discarding any overflow.
 * @param prompt Prompt to display before reading (may be NULL for none).
 * @param buffer Destination buffer.
 * @param size   Size of buffer, in bytes.
 */
void read_line(const char *prompt, char *buffer, size_t size);

/**
 * @brief Prompt the user for an integer within [min_value, max_value],
 *        re-prompting on invalid input.
 * @param prompt    Prompt to display.
 * @param min_value Smallest acceptable value (inclusive).
 * @param max_value Largest acceptable value (inclusive).
 * @return The validated integer entered by the user.
 */
int read_int_in_range(const char *prompt, int min_value, int max_value);

/**
 * @brief Interactively prompt the user for a calendar date/time and convert
 *        it to a time_t using mktime(), re-prompting until a valid date is
 *        entered.
 * @return The resulting epoch timestamp (local time zone).
 */
time_t read_datetime_input(void);

/**
 * @brief Format a time_t as a human-readable local-time string using
 *        localtime() and strftime() (safer than raw ctime()).
 * @param t      Timestamp to format.
 * @param buffer Destination buffer.
 * @param size   Size of buffer, in bytes.
 */
void format_time(time_t t, char *buffer, size_t size);

/**
 * @brief Compute the next occurrence of a recurring event after its current
 *        start_time, based on its RecurrencePattern.
 *
 * Uses localtime()/mktime() so that calendar irregularities (month lengths,
 * leap years, DST transitions) are resolved by the C library rather than by
 * naive arithmetic on raw seconds.
 *
 * @param current_start Current (just-fired) start_time of the event.
 * @param pattern       Recurrence rule to apply.
 * @return The next start_time. If pattern is RECUR_NONE, current_start is
 *         returned unchanged.
 */
time_t get_next_occurrence(time_t current_start, RecurrencePattern pattern);

/**
 * @brief Human-readable name for a Priority value.
 * @param p Priority to render.
 * @return Pointer to a static, read-only string.
 */
const char *priority_to_string(Priority p);

/**
 * @brief Human-readable name for a RecurrencePattern value.
 * @param r Recurrence pattern to render.
 * @return Pointer to a static, read-only string.
 */
const char *recurrence_to_string(RecurrencePattern r);

/**
 * @brief Print a single event in a formatted, human-friendly block.
 * @param e Event to print. No-op if e is NULL.
 */
void print_event(const Event *e);

/**
 * @brief Prompt the user to choose a Priority from a small menu.
 * @return The selected Priority.
 */
Priority prompt_priority(void);

/**
 * @brief Prompt the user to choose a RecurrencePattern from a small menu.
 * @return The selected RecurrencePattern.
 */
RecurrencePattern prompt_recurrence(void);

#endif /* SCHEDULER_H */
