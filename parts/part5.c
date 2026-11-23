#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "../include/types.h"

// route locks for roundtrip booking
static pthread_mutex_t route_locks[num_routes];
static int mode = 1;

// job data with both route ids
typedef struct {
    int outbound_route_id;
    int return_route_id;
    int worker_id;
} BookJob;

// lock smaller route id first to prevent deadlock
static void book_ordered(int outbound_route_id, int return_route_id) {
    int first_route;
    int second_route;
    if (outbound_route_id < return_route_id) {
        first_route = outbound_route_id;
        second_route = return_route_id;
    } else {
        first_route = return_route_id;
        second_route = outbound_route_id;
    }

    pthread_mutex_lock(&route_locks[first_route]);
    usleep(2000);
    pthread_mutex_lock(&route_locks[second_route]);

    printf("booked roundtrip (%d, %d) using order [%d, %d]\n",
           outbound_route_id, return_route_id, first_route, second_route);

    pthread_mutex_unlock(&route_locks[second_route]);
    pthread_mutex_unlock(&route_locks[first_route]);
}

// try locking second route, back off if busy
static void book_trylock(int outbound_route_id, int return_route_id) {
    int attempts = 0;
    while (1) {
        attempts++;
        pthread_mutex_lock(&route_locks[outbound_route_id]);

        // if second route is free, we are done
        if (pthread_mutex_trylock(&route_locks[return_route_id]) == 0) {
            printf("booked roundtrip (%d, %d) via trylock after %d tries\n",
                   outbound_route_id, return_route_id, attempts);
            pthread_mutex_unlock(&route_locks[return_route_id]);
            pthread_mutex_unlock(&route_locks[outbound_route_id]);
            return;
        }

        // unlock and wait before trying again
        pthread_mutex_unlock(&route_locks[outbound_route_id]);
        usleep((rand() % 3000) + 500);
    }
}

// locks routes in raw order which causes deadlock
static void book_naive(int outbound_route_id, int return_route_id) {
    pthread_mutex_lock(&route_locks[outbound_route_id]);
    usleep(20000);
    pthread_mutex_lock(&route_locks[return_route_id]);

    printf("booked roundtrip (%d, %d)\n", outbound_route_id, return_route_id);

    pthread_mutex_unlock(&route_locks[return_route_id]);
    pthread_mutex_unlock(&route_locks[outbound_route_id]);
}

// run selected locking strategy
static void *worker_thread(void *arg) {
    BookJob *job = (BookJob *)arg;
    if (mode == 0) {
        book_naive(job->outbound_route_id, job->return_route_id);
    } else if (mode == 1) {
        book_ordered(job->outbound_route_id, job->return_route_id);
    } else {
        book_trylock(job->outbound_route_id, job->return_route_id);
    }
    return NULL;
}

int main(int argc, char **argv) {
    srand(55);
    // 0 = deadlock, 1 = ordered locks, 2 = trylock
    if (argc > 1) mode = atoi(argv[1]);

    for (int i = 0; i < num_routes; i++) {
        pthread_mutex_init(&route_locks[i], NULL);
    }

    printf("running part 5: roundtrip booking with mode %d\n", mode);

    // run two threads with opposite route order (1->2 and 2->1)
    pthread_t thread1, thread2;
    BookJob job1 = {1, 2, 1};
    BookJob job2 = {2, 1, 2};

    pthread_create(&thread1, NULL, worker_thread, &job1);
    pthread_create(&thread2, NULL, worker_thread, &job2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    for (int i = 0; i < num_routes; i++) {
        pthread_mutex_destroy(&route_locks[i]);
    }

    printf("finished without deadlock\n");
    return 0;
}
