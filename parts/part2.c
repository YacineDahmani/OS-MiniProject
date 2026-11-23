#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "../include/booking.h"

// shared database and lock mode flag
static Database db;
static int use_route_lock_flag = 0;

// worker info
typedef struct {
    int agency_id;
    int worker_id;
} Worker;

// worker routine that books random routes
static void *worker_thread(void *arg) {
    Worker *w = (Worker *)arg;

    for (int i = 0; i < 20; i++) {
        int route = rand() % num_routes;
        int client = (w->agency_id * 1000) + (w->worker_id * 100) + i + 1;
        book_seat(&db, route, client, use_route_lock_flag);
    }
    return NULL;
}

// run benchmark for a specific lock mode (0 = global lock, 1 = per-route lock)
static double run_test(int mode) {
    use_route_lock_flag = mode;
    db_init(&db);

    pthread_t threads[total_workers];
    Worker workers[total_workers];

    clock_t start = clock();

    // launch worker threads
    int idx = 0;
    for (int a = 0; a < num_agencies; a++) {
        for (int w = 0; w < workers_per_agency; w++) {
            workers[idx].agency_id = a + 1;
            workers[idx].worker_id = w + 1;
            pthread_create(&threads[idx], NULL, worker_thread, &workers[idx]);
            idx++;
        }
    }

    // wait for completion
    for (int i = 0; i < total_workers; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    // verify consistency
    int occupied = 0;
    for (int r = 0; r < num_routes; r++) {
        for (int s = 0; s < seats_per_route; s++) {
            if (db.routes[r].seats[s] != 0) occupied++;
        }
    }

    if (mode == 0) {
        printf("mode: global mutex\n");
    } else {
        printf("mode: per-route mutex\n");
    }
    printf("sold: %d, occupied: %d, time: %.4f s\n", db.tickets_sold, occupied, elapsed);

    db_destroy(&db);
    return elapsed;
}

int main() {
    srand(123);
    printf("running part 2: mutex comparison benchmark\n");

    // compare global mutex against per-route mutex
    double t_global = run_test(0);
    double t_route = run_test(1);

    if (t_route > 0) {
        printf("speedup: %.2fx faster with per-route locks\n", t_global / t_route);
    }

    return 0;
}
