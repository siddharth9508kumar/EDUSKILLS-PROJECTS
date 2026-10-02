# Real-Time Event Scheduler

A menu-driven, command-line event scheduler written in C11. Events are stored in an array-backed **binary min-heap** keyed on start time, so the next upcoming event is always available in O(1) and inserts/removals run in O(log n).

Built as part of the EduSkills projects.

---

## Features

- **Add events** with a title, description, start date/time, duration, priority and recurrence rule
- **Peek at the next upcoming event** without removing it
- **Process due events**: fires every event whose start time has passed and automatically reschedules recurring ones
- **List all events** in chronological order (the heap itself is left untouched)
- **Search** for an event by ID
- **Cancel / delete** any event by ID, regardless of its position in the heap
- **Priority tie-breaking**: when two events share a start time, the higher-priority one is served first
- **Recurrence**: none, daily, weekly or monthly, with next occurrences computed via `localtime()`/`mktime()` so month lengths, leap years and DST are handled by the C library
- **Input validation**: all prompts re-ask on invalid input; overflowing input is discarded safely
- **Memory-safe**: a single `free_heap()` on exit, intended to be Valgrind-clean

## Data model

| Type | Values |
| --- | --- |
| `Priority` | `LOW`, `MEDIUM`, `HIGH`, `URGENT` |
| `RecurrencePattern` | `NONE`, `DAILY`, `WEEKLY`, `MONTHLY` |

Each `Event` holds an ID, title (max 100 chars), description (max 256 chars), start time (`time_t`), duration in minutes, priority and recurrence. All strings are stored inline, so events are copied by value and never need their own `free()`.

## Project structure

```
.
├── main.c         # Interactive CLI menu and handlers
├── heap.c         # Min-heap priority queue implementation
├── utils.c        # Input helpers, time formatting, recurrence math, printing
├── scheduler.h    # Shared types, constants and function prototypes
├── Makefile       # Build, run, valgrind and clean targets
└── .vscode/       # Editor configuration
```

### Heap API (`heap.c`)

| Function | Purpose |
| --- | --- |
| `create_heap` / `free_heap` | Allocate and release the heap |
| `insert_event` | Insert an event (grows the array when full) |
| `extract_min` | Remove and return the earliest event |
| `peek_min` | View the earliest event without removing it |
| `heapify_up` / `heapify_down` | Restore the heap property |
| `find_index_by_id` / `find_event_by_id` | Look up an event by ID |
| `remove_event_by_id` | Delete an arbitrary event |
| `get_sorted_snapshot` | Chronologically sorted copy of all events |

## Getting started

### Prerequisites

- `gcc` (or any C11-compatible compiler)
- `make`
- `valgrind` (optional, for leak checking; Linux/macOS)

### Build and run

```bash
git clone https://github.com/siddharth9508kumar/EDUSKILLS-PROJECTS.git
cd EDUSKILLS-PROJECTS

make          # build the 'scheduler' executable
make run      # build and run
```

### Other make targets

| Command | Description |
| --- | --- |
| `make` | Build the `scheduler` executable |
| `make run` | Build and launch the program |
| `make valgrind` | Run under Valgrind with full leak checking |
| `make clean` | Remove object files and the executable |

The project compiles with `-Wall -Wextra -std=c11 -g -O0`.

## Usage

On launch you'll see this menu:

```
==================== Real-Time Event Scheduler ====================
 1) Add a new event
 2) View next upcoming event
 3) Process due events (fires + reschedules recurring ones)
 4) List all events (chronological order)
 5) Search for an event by ID
 6) Cancel / delete an event by ID
 7) Exit
=====================================================================
```

A typical session:

1. Choose **1** and enter the event details. The program prints the assigned event ID.
2. Choose **4** to see everything that's scheduled, soonest first.
3. Choose **3** at any time to fire events that are due. Recurring events are re-inserted at their next occurrence.
4. Choose **6** with an event ID to cancel it.

> Note: events live in memory for the lifetime of the process and are not persisted to disk.

## How it works

- The heap is ordered by `start_time`; ties are broken by priority (higher priority sorts first).
- **Processing due events** repeatedly peeks at the root, and while its start time is at or before the current time, extracts it, prints it, and re-inserts it with `get_next_occurrence()` if it recurs.
- **Listing** copies the heap into a new array and sorts the copy, so the heap invariant is never disturbed.
- **Removing by ID** finds the element, swaps in the last element, and sifts it up or down as needed.

## Possible improvements

- Persist events to a file so they survive restarts
- Detect overlapping events using `duration_minutes`
- Add an ID-to-index hash map to make lookups O(1)
- Add unit tests for the heap and recurrence logic
- Add a `.gitignore` for build artifacts
