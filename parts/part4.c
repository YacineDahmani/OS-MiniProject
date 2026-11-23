#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/queue.h"
#include "../include/log.h"

// request queues for each agency
static Queue agency_queues[num_agencies];

// agency worker identification
typedef struct {
    int agency_id;
    int worker_id;
} Worker;

// producer thread for each agency generating client requests
static void *reception_thread(void *arg) {
    int agency_id = *(int *)arg;

    for (int i = 0; i < 16; i++) {
        Request req;
        // alternate between check and booking requests
        if (i % 3 == 0) {
            req.type = req_check;
        } else {
            req.type = req_book;
        }
        req.agency_id = agency_id;
        req.client_id = (agency_id * 1000) + (i + 1);
        req.route_id = rand() % num_routes;

        queue_push(&agency_queues[agency_id - 1], req);
        usleep(500);
    }

    // tell each worker to stop
    for (int w = 0; w < workers_per_agency; w++) {
        Request stop_req;
        stop_req.type = req_stop;
        stop_req.agency_id = agency_id;
        queue_push(&agency_queues[agency_id - 1], stop_req);
    }

    return NULL;
}

// consumer worker thread processing requests from its agency queue
static void *worker_thread(void *arg) {
    Worker *w = (Worker *)arg;
    Queue *queue = &agency_queues[w->agency_id - 1];

    while (1) {
        Request req = queue_pop(queue);
        if (req.type == req_stop) {
            break;
        }

        // log the action to background logger
        if (req.type == req_book) {
            log_write("agency %d worker %d: booked seat on route %d for client %d",
                      w->agency_id, w->worker_id, req.route_id, req.client_id);
        } else {
            log_write("agency %d worker %d: checked route %d",
                      w->agency_id, w->worker_id, req.route_id);
        }
        usleep(300);
    }

    return NULL;
}

int main() {
    printf("running part 4: agency queues and logger thread\n");

    // initialize agency queues
    for (int i = 0; i < num_agencies; i++) {
        queue_init(&agency_queues[i]);
    }

    // start asynchronous logger thread
    log_init();
    log_start();

    // spawn agency reception producers
    pthread_t reception_threads[num_agencies];
    int agency_ids[num_agencies];
    for (int a = 0; a < num_agencies; a++) {
        agency_ids[a] = a + 1;
        pthread_create(&reception_threads[a], NULL, reception_thread, &agency_ids[a]);
    }

    // spawn worker consumers
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

    // wait for producers and workers to finish
    for (int a = 0; a < num_agencies; a++) pthread_join(reception_threads[a], NULL);
    for (int i = 0; i < total_workers; i++) pthread_join(threads[i], NULL);

    // stop logger thread
    log_stop();

    // clean up queue mutexes
    for (int i = 0; i < num_agencies; i++) {
        queue_destroy(&agency_queues[i]);
    }

    printf("all threads finished cleanly\n");
    return 0;
}
