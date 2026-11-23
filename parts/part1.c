#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "../include/types.h"

// shared seats and counter without any locks
static int seats[num_routes][seats_per_route];
static int tickets_sold = 0;

typedef struct {
    int agency_id;
    int worker_id;
} Worker;

// booking without locks to showcase race conditions
static int book_unprotected(int route_id, int client_id) {
    for (int i = 0; i < seats_per_route; i++) {
        if (seats[route_id][i] == 0) {
            // sleep makes the race window larger
            usleep(1000);
            seats[route_id][i] = client_id;
            tickets_sold++;
            return i;
        }
    }
    return -1;
}

static void *worker_thread(void *arg) {
    Worker *w = (Worker *)arg;

    for (int i = 0; i < 15; i++) {
        int route = rand() % num_routes;
        int client = (w->agency_id * 1000) + (w->worker_id * 100) + i + 1;
        book_unprotected(route, client);
    }
    return NULL;
}

int main() {
    srand(42);
    tickets_sold = 0;

    for (int r = 0; r < num_routes; r++) {
        for (int s = 0; s < seats_per_route; s++) {
            seats[r][s] = 0;
        }
    }

    printf("running part 1: unsynchronized booking test\n");

    pthread_t threads[total_workers];
    Worker workers[total_workers];

    int idx = 0;
    for (int a = 0; a < num_agencies; a++) {
        for (int w = 0; w < workers_per_agency; w++) {
            workers[idx].agency_id = a + 1;
            workers[idx].worker_id = w + 1;
            pthread_create(&threads[idx], NULL, worker_thread, &workers[idx]);
            idx++;
        }
    }

    for (int i = 0; i < total_workers; i++) {
        pthread_join(threads[i], NULL);
    }

    // count actual seats taken in memory
    int occupied = 0;
    for (int r = 0; r < num_routes; r++) {
        for (int s = 0; s < seats_per_route; s++) {
            if (seats[r][s] != 0) occupied++;
        }
    }

    printf("tickets_sold counter = %d\n", tickets_sold);
    printf("occupied seats in memory = %d\n", occupied);

    // check if counter matches actual seats
    if (tickets_sold != occupied) {
        printf("mismatch detected: tickets_sold (%d) != occupied (%d)\n", tickets_sold, occupied);
        printf("reason: concurrent writes without mutex caused lost updates\n");
    }

    return 0;
}
