# Makefile for the Real-Time Event Scheduler
#
# Targets:
#   make          - build the 'scheduler' executable
#   make run      - build and run the executable
#   make valgrind - build (with debug symbols) and run under Valgrind,
#                   checking for memory leaks
#   make clean    - remove build artifacts

CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -g -O0
LDFLAGS  :=

SRCS     := main.c heap.c utils.c
OBJS     := $(SRCS:.c=.o)
HEADERS  := scheduler.h
TARGET   := scheduler

.PHONY: all run valgrind clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
