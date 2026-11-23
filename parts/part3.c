#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../include/rw_lock.h"

// number of reader and writer threads
#define num_readers 8
#define num_writers 2

// readers-writers lock and shared counter
static RwLock lock;
static int data = 0;
static double total_wait_ms = 0;
static pthread_mutex_t stat_mutex = PTHREAD_MUTEX_INITIALIZER;

// reader thread reading shared data concurrently
static void *reader(void *arg) {
    (void)arg;
    for (int i = 0; i < 20; i++) {
        read_lock(&lock);
        volatile int val = data;
        (void)val;
        usleep(500);
        read_unlock(&lock);
        usleep(200);
    }
    return NULL;
}

// writer thread measuring wait latency before modifying data
static void *writer(void *arg) {
    (void)arg;
    for (int i = 0; i < 5; i++) {
        // record lock acquisition waiting time
        clock_t t0 = clock();
        write_lock(&lock);
        clock_t t1 = clock();

        double wait = ((double)(t1 - t0) / CLOCKS_PER_SEC) * 1000.0;
        pthread_mutex_lock(&stat_mutex);
        total_wait_ms += wait;
        pthread_mutex_unlock(&stat_mutex);

        data++;
        usleep(2000);
        write_unlock(&lock);
        usleep(1000);
    }
    return NULL;
}

// execute test for given policy and print average writer latency
static void run_case(const char *name, int fair) {
    rw_lock_init(&lock, fair);
    total_wait_ms = 0;

    // start reader and writer threads
    pthread_t r[num_readers], w[num_writers];
    for (int i = 0; i < num_readers; i++) pthread_create(&r[i], NULL, reader, NULL);
    for (int i = 0; i < num_writers; i++) pthread_create(&w[i], NULL, writer, NULL);

    // wait for all threads to finish
    for (int i = 0; i < num_readers; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < num_writers; i++) pthread_join(w[i], NULL);

    double avg = total_wait_ms / (num_writers * 5);
    printf("mode: %-25s avg writer wait: %.2f ms\n", name, avg);

    rw_lock_destroy(&lock);
}

int main() {
    printf("running part 3: readers-writers tests\n");

    // test reader priority where writers may starve
    run_case("reader priority (starvation)", 0);

    // test fair policy where writers are queued fairly
    run_case("fair policy (no starvation)", 1);

    return 0;
}
