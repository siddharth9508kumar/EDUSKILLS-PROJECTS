/**
 * @file utils.c
 * @brief Date/time conversion helpers, console input helpers, and
 *        recurrence-calculation logic for the Real-Time Event Scheduler.
 */

#include "scheduler.h"

void read_line(const char *prompt, char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }

    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    /* Strip a trailing newline, if fgets captured the whole line. */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (len == size - 1) {
        /* Line was longer than the buffer: discard the remainder of it. */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* discard */
        }
    }
}

int read_int_in_range(const char *prompt, int min_value, int max_value) {
    char line[LINE_BUFFER_SIZE];

    for (;;) {
        read_line(prompt, line, sizeof(line));

        char *endptr = NULL;
        long value = strtol(line, &endptr, 10);

        /* Valid only if we consumed at least one digit and nothing but
         * trailing whitespace follows. */
        if (endptr != line) {
            while (*endptr != '\0' && isspace((unsigned char)*endptr)) {
                endptr++;
            }
            if (*endptr == '\0' && value >= min_value && value <= max_value) {
                return (int)value;
            }
        }

        printf("  Invalid input. Please enter a whole number between %d and %d.\n",
               min_value, max_value);
    }
}

time_t read_datetime_input(void) {
    for (;;) {
        int year = read_int_in_range("  Year (e.g. 2026): ", 1970, 2200);
        int month = read_int_in_range("  Month (1-12): ", 1, 12);
        int day = read_int_in_range("  Day (1-31): ", 1, 31);
        int hour = read_int_in_range("  Hour (0-23): ", 0, 23);
        int minute = read_int_in_range("  Minute (0-59): ", 0, 59);

        struct tm time_info;
        memset(&time_info, 0, sizeof(time_info));
        time_info.tm_year = year - 1900;
        time_info.tm_mon = month - 1;
        time_info.tm_mday = day;
        time_info.tm_hour = hour;
        time_info.tm_min = minute;
        time_info.tm_sec = 0;
        time_info.tm_isdst = -1; /* Let the library determine DST. */

        struct tm normalized = time_info;
        time_t result = mktime(&normalized);

        if (result == (time_t)-1) {
            printf("  That date/time could not be represented. Please try again.\n");
            continue;
        }

        /* mktime() silently "normalizes" invalid dates (e.g. day 31 of a
         * 30-day month rolls into the next month). Detect that by comparing
         * the fields we asked for against what mktime() actually produced,
         * and reject anything that changed. */
        if (normalized.tm_year != time_info.tm_year ||
            normalized.tm_mon != time_info.tm_mon ||
            normalized.tm_mday != time_info.tm_mday) {
            printf("  %d-%02d-%02d is not a valid calendar date. Please try again.\n",
                   year, month, day);
            continue;
        }

        return result;
    }
}

void format_time(time_t t, char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    struct tm time_info;
    struct tm *converted = localtime(&t);
    if (converted == NULL) {
        snprintf(buffer, size, "<invalid time>");
        return;
    }
    time_info = *converted; /* Copy out immediately: localtime() reuses a static buffer. */

    strftime(buffer, size, "%Y-%m-%d %H:%M (%A)", &time_info);
}

time_t get_next_occurrence(time_t current_start, RecurrencePattern pattern) {
    if (pattern == RECUR_NONE) {
        return current_start;
    }

    struct tm *converted = localtime(&current_start);
    if (converted == NULL) {
        return current_start;
    }
    struct tm time_info = *converted;
    time_info.tm_isdst = -1; /* Recompute DST for the new date rather than reusing the old flag. */

    switch (pattern) {
        case RECUR_DAILY:
            time_info.tm_mday += 1;
            break;
        case RECUR_WEEKLY:
            time_info.tm_mday += 7;
            break;
        case RECUR_MONTHLY:
            time_info.tm_mon += 1;
            /* mktime() will roll tm_year/tm_mon forward correctly if
             * tm_mon reaches 12, and will clamp/roll an out-of-range
             * tm_mday (e.g. Jan 31 -> Feb 31) into the following month,
             * which is standard, well-defined mktime() behaviour. */
            break;
        case RECUR_NONE:
        default:
            return current_start;
    }

    time_t next = mktime(&time_info);
    if (next == (time_t)-1) {
        return current_start;
    }
    return next;
}

const char *priority_to_string(Priority p) {
    switch (p) {
        case PRIORITY_LOW:    return "LOW";
        case PRIORITY_MEDIUM: return "MEDIUM";
        case PRIORITY_HIGH:   return "HIGH";
        case PRIORITY_URGENT: return "URGENT";
        default:              return "UNKNOWN";
    }
}

const char *recurrence_to_string(RecurrencePattern r) {
    switch (r) {
        case RECUR_NONE:    return "NONE";
        case RECUR_DAILY:   return "DAILY";
        case RECUR_WEEKLY:  return "WEEKLY";
        case RECUR_MONTHLY: return "MONTHLY";
        default:            return "UNKNOWN";
    }
}

void print_event(const Event *e) {
    if (e == NULL) {
        return;
    }

    char time_buf[64];
    format_time(e->start_time, time_buf, sizeof(time_buf));

    printf("  ------------------------------------------------------------\n");
    printf("  ID:          %d\n", e->id);
    printf("  Title:       %s\n", e->title);
    printf("  Description: %s\n", e->description);
    printf("  Start Time:  %s\n", time_buf);
    printf("  Duration:    %d minute(s)\n", e->duration_minutes);
    printf("  Priority:    %s\n", priority_to_string(e->priority));
    printf("  Recurrence:  %s\n", recurrence_to_string(e->recurrence));
    printf("  ------------------------------------------------------------\n");
}

Priority prompt_priority(void) {
    printf("  Select priority:\n");
    printf("    0) LOW\n    1) MEDIUM\n    2) HIGH\n    3) URGENT\n");
    int choice = read_int_in_range("  Choice: ", 0, 3);
    return (Priority)choice;
}

RecurrencePattern prompt_recurrence(void) {
    printf("  Select recurrence:\n");
    printf("    0) NONE\n    1) DAILY\n    2) WEEKLY\n    3) MONTHLY\n");
    int choice = read_int_in_range("  Choice: ", 0, 3);
    return (RecurrencePattern)choice;
}
