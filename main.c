/**
 * @file main.c
 * @brief Interactive CLI front-end for the Real-Time Event Scheduler.
 *
 * Presents a menu-driven interface backed by the MinHeap in heap.c and the
 * helpers in utils.c. All events live in a single MinHeap for the lifetime
 * of the process; free_heap() is called exactly once, on exit, so the
 * program is valgrind-clean with no leaked allocations.
 */

#include "scheduler.h"

/**
 * @brief Print the main menu of available actions.
 */
static void print_menu(void) {
    printf("\n==================== Real-Time Event Scheduler ====================\n");
    printf("  1) Add a new event\n");
    printf("  2) View next upcoming event\n");
    printf("  3) Process due events (fires + reschedules recurring ones)\n");
    printf("  4) List all events (chronological order)\n");
    printf("  5) Search for an event by ID\n");
    printf("  6) Cancel / delete an event by ID\n");
    printf("  7) Exit\n");
    printf("=====================================================================\n");
}

/**
 * @brief Handle menu option 1: gather event details from the user and
 *        insert the resulting Event into the heap.
 * @param heap Heap to insert into.
 */
static void handle_add_event(MinHeap *heap) {
    Event event;
    memset(&event, 0, sizeof(event));

    printf("\n-- Add New Event --\n");

    read_line("  Title: ", event.title, sizeof(event.title));
    read_line("  Description: ", event.description, sizeof(event.description));

    printf("  Enter the start date and time:\n");
    event.start_time = read_datetime_input();

    event.duration_minutes = read_int_in_range("  Duration in minutes: ", 1, 100000);
    event.priority = prompt_priority();
    event.recurrence = prompt_recurrence();

    event.id = heap->next_id++;

    if (insert_event(heap, event) == 0) {
        printf("\n  Event #%d (\"%s\") scheduled successfully.\n", event.id, event.title);
    } else {
        printf("\n  ERROR: Failed to schedule event (out of memory).\n");
        heap->next_id--; /* Give the id back since nothing was actually stored. */
    }
}

/**
 * @brief Handle menu option 2: show the earliest event without removing it.
 * @param heap Heap to inspect.
 */
static void handle_view_next(const MinHeap *heap) {
    const Event *next = peek_min(heap);
    printf("\n-- Next Upcoming Event --\n");
    if (next == NULL) {
        printf("  No events are currently scheduled.\n");
        return;
    }
    print_event(next);
}

/**
 * @brief Handle menu option 3: pop every event whose start_time has already
 *        passed, "fire" it, and re-insert it under its next occurrence if
 *        it recurs.
 * @param heap Heap to process.
 */
static void handle_process_due(MinHeap *heap) {
    printf("\n-- Processing Due Events --\n");

    time_t now = time(NULL);
    int processed_count = 0;

    for (;;) {
        const Event *top = peek_min(heap);
        if (top == NULL || top->start_time > now) {
            break;
        }

        Event current;
        if (extract_min(heap, &current) != 0) {
            break; /* Should not happen given the peek above, but stay safe. */
        }

        processed_count++;
        printf("\n  >> Firing event #%d: \"%s\"\n", current.id, current.title);
        print_event(&current);

        if (current.recurrence != RECUR_NONE) {
            Event rescheduled = current;
            rescheduled.start_time = get_next_occurrence(current.start_time, current.recurrence);

            char time_buf[64];
            format_time(rescheduled.start_time, time_buf, sizeof(time_buf));

            if (insert_event(heap, rescheduled) == 0) {
                printf("  Recurring event: re-scheduled for %s.\n", time_buf);
            } else {
                printf("  ERROR: could not re-schedule recurring event (out of memory).\n");
            }
        }
    }

    if (processed_count == 0) {
        printf("  No events are currently due.\n");
    } else {
        printf("\n  Total events processed: %d\n", processed_count);
    }
}

/**
 * @brief Handle menu option 4: print every event in chronological order.
 * @param heap Heap to list.
 */
static void handle_list_all(const MinHeap *heap) {
    printf("\n-- All Scheduled Events (chronological) --\n");

    size_t count = 0;
    Event *sorted = get_sorted_snapshot(heap, &count);

    if (sorted == NULL || count == 0) {
        printf("  No events are currently scheduled.\n");
        free(sorted);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        print_event(&sorted[i]);
    }
    printf("  Total: %zu event(s).\n", count);

    free(sorted);
}

/**
 * @brief Handle menu option 5: look up and display a single event by id.
 * @param heap Heap to search.
 */
static void handle_search(MinHeap *heap) {
    printf("\n-- Search Event by ID --\n");
    int id = read_int_in_range("  Enter event ID: ", 1, 2000000000);

    Event *found = find_event_by_id(heap, id);
    if (found == NULL) {
        printf("  No event found with ID %d.\n", id);
        return;
    }
    print_event(found);
}

/**
 * @brief Handle menu option 6: remove a specific event from the heap.
 * @param heap Heap to remove from.
 */
static void handle_cancel(MinHeap *heap) {
    printf("\n-- Cancel / Delete Event by ID --\n");
    int id = read_int_in_range("  Enter event ID to cancel: ", 1, 2000000000);

    Event removed;
    if (remove_event_by_id(heap, id, &removed) == 0) {
        printf("  Cancelled event #%d (\"%s\").\n", removed.id, removed.title);
    } else {
        printf("  No event found with ID %d.\n", id);
    }
}

/**
 * @brief Program entry point: builds the heap, runs the menu loop, and
 *        guarantees a single, clean teardown of all allocated memory.
 * @return EXIT_SUCCESS on normal termination.
 */
int main(void) {
    MinHeap *heap = create_heap(HEAP_INITIAL_CAPACITY);
    if (heap == NULL) {
        fprintf(stderr, "Fatal: could not allocate the event heap.\n");
        return EXIT_FAILURE;
    }

    printf("Welcome to the Real-Time Event Scheduler.\n");

    int running = 1;
    while (running) {
        print_menu();
        int choice = read_int_in_range("Choose an option (1-7): ", 1, 7);

        switch (choice) {
            case 1: handle_add_event(heap);   break;
            case 2: handle_view_next(heap);   break;
            case 3: handle_process_due(heap); break;
            case 4: handle_list_all(heap);    break;
            case 5: handle_search(heap);      break;
            case 6: handle_cancel(heap);      break;
            case 7:
                printf("\nGoodbye!\n");
                running = 0;
                break;
            default:
                /* Unreachable: read_int_in_range() already enforces 1..7. */
                break;
        }
    }

    free_heap(heap);
    heap = NULL;

    return EXIT_SUCCESS;
}
