CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -Iinclude

OBJS = src/queue.o src/booking.o src/rw_lock.o src/log.o
BINS = parts/part1 parts/part2 parts/part3 parts/part4 parts/part5 parts/part6

all: $(OBJS) $(BINS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

parts/part1: parts/part1.c
	$(CC) $(CFLAGS) $< -o $@

parts/part2: parts/part2.c src/booking.o
	$(CC) $(CFLAGS) $^ -o $@

parts/part3: parts/part3.c src/rw_lock.o
	$(CC) $(CFLAGS) $^ -o $@

parts/part4: parts/part4.c src/queue.o src/log.o
	$(CC) $(CFLAGS) $^ -o $@

parts/part5: parts/part5.c
	$(CC) $(CFLAGS) $< -o $@

parts/part6: parts/part6.c src/queue.o src/booking.o src/log.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f src/*.o parts/part1 parts/part2 parts/part3 parts/part4 parts/part5 parts/part6 parts/*.exe src/*.exe journal.log

.PHONY: all clean
