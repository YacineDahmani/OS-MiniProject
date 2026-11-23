#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/types.h"
#include "../include/queue.h"
#include "../include/booking.h"
#include "../include/log.h"

// number of requests to generate for each type
#define total_reserve 300
#define total_cancel 50
#define total_check 500
#define total_roundtrip 20

// shared database and per-agency queues
static Database db;
static Queue agency_queues[num_agencies];

// worker info
typedef struct {
    int agency_id;
    int worker_id;
} Worker;

// process request and log the result
static void process_request(Request req) {
    if (req.type == req_book) {
        int seat = book_seat(&db, req.route_id, req.client_id, 1);
        if (seat >= 0) {
            log_write("agency %d worker %d: client %d booked seat %d on route %d", req.agency_id, req.worker_id, req.client_id, seat, req.route_id);
        } else {
            log_write("agency %d worker %d: client %d booking failed (route %d full)",req.agency_id, req.worker_id, req.client_id, req.route_id);
        }
    } else if (req.type == req_cancel) {
        int seat = cancel_seat(&db, req.route_id, req.client_id);
        if (seat >= 0) {
            log_write("agency %d worker %d: client %d cancelled seat %d on route %d", req.agency_id, req.worker_id, req.client_id, seat, req.route_id);
        } else {
            log_write("agency %d worker %d: client %d cancellation failed on route %d", req.agency_id, req.worker_id, req.client_id, req.route_id);
        }
    } else if (req.type == req_check) {
        int free = check_seats(&db, req.route_id);
        log_write("agency %d worker %d: client %d checked route %d (free seats: %d)",req.agency_id, req.worker_id, req.client_id, req.route_id, free);
    } else if (req.type == req_roundtrip) {
        int res = book_roundtrip(&db, req.route_id, req.return_route_id, req.client_id);
        if (res == 0) {
            log_write("agency %d worker %d: client %d booked roundtrip (%d, %d)", req.agency_id, req.worker_id, req.client_id, req.route_id, req.return_route_id);
        } else {
            log_write("agency %d worker %d: client %d roundtrip failed (%d, %d)", req.agency_id, req.worker_id, req.client_id, req.route_id, req.return_route_id);
        }
    }
}

// worker thread that handles requests from the queue
static void *worker_thread(void *arg) {
    Worker *w = (Worker *)arg;
    Queue *queue = &agency_queues[w->agency_id - 1];

    while (1) {
        Request req = queue_pop(queue);
        // stop when stop request arrives
        if (req.type == req_stop) break;
        req.worker_id = w->worker_id;
        process_request(req);
    }
    return NULL;
}

// reception thread that creates requests for this agency
static void *reception_thread(void *arg) {
    int agency_id = *(int *)arg;

    // divide requests evenly across agencies
    int book_quota = total_reserve / num_agencies;
    int cancel_quota = total_cancel / num_agencies;
    int check_quota = total_check / num_agencies;
    int roundtrip_quota = total_roundtrip / num_agencies;

    // give extra remainder requests to agency 1
    if (agency_id == 1) {
        book_quota += total_reserve % num_agencies;
        cancel_quota += total_cancel % num_agencies;
        check_quota += total_check % num_agencies;
        roundtrip_quota += total_roundtrip % num_agencies;
    }

    int total_requests = book_quota + cancel_quota + check_quota + roundtrip_quota;
    int booked_count = 0, cancelled_count = 0, checked_count = 0;

    for (int i = 0; i < total_requests; i++) {
        Request req;
        req.agency_id = agency_id;
        req.worker_id = 0;
        req.client_id = (agency_id * 10000) + (i + 1);

        if (booked_count < book_quota) {
            req.type = req_book;
            req.route_id = rand() % num_routes;
            booked_count++;
        } else if (cancelled_count < cancel_quota) {
            req.type = req_cancel;
            req.route_id = rand() % num_routes;
            cancelled_count++;
        } else if (checked_count < check_quota) {
            req.type = req_check;
            req.route_id = rand() % num_routes;
            checked_count++;
        } else {
            req.type = req_roundtrip;
            req.route_id = rand() % num_routes;
            req.return_route_id = (req.route_id + 1) % num_routes;
        }

        queue_push(&agency_queues[agency_id - 1], req);
    }

    // send stop request to each worker in this agency
    for (int w = 0; w < workers_per_agency; w++) {
        Request stop_req;
        stop_req.type = req_stop;
        stop_req.agency_id = agency_id;
        queue_push(&agency_queues[agency_id - 1], stop_req);
    }

    return NULL;
}

// check if all seats and balances are correct
static void check_final_results() {
    int occupied = 0;
    int double_booked = 0;

    // check for duplicate seats
    for (int r = 0; r < num_routes; r++) {
        for (int s = 0; s < seats_per_route; s++) {
            if (db.routes[r].seats[s] != 0) {
                occupied++;
                for (int j = s + 1; j < seats_per_route; j++) {
                    if (db.routes[r].seats[s] == db.routes[r].seats[j]) {
                        double_booked++;
                    }
                }
            }
        }
    }

    int balance = db.tickets_sold - db.cancellations;

    printf("\nfinal check:\n");
    printf("double booked seats = %d\n", double_booked);
    printf("tickets_sold - cancellations = %d\n", balance);
    printf("occupied seats in memory = %d\n", occupied);

    // confirm no double bookings and matching counts
    if (double_booked == 0 && balance == occupied) {
        printf("all checks passed: database is consistent\n");
    } else {
        printf("checks failed\n");
    }
}

int main() {
    srand(1234);
    printf("running part 6: full scenario\n");

    db_init(&db);

    // init queues
    for (int i = 0; i < num_agencies; i++) {
        queue_init(&agency_queues[i]);
    }

    // start logger thread
    log_init();
    log_start();

    // start reception threads
    pthread_t reception_threads[num_agencies];
    int agency_ids[num_agencies];
    for (int a = 0; a < num_agencies; a++) {
        agency_ids[a] = a + 1;
        pthread_create(&reception_threads[a], NULL, reception_thread, &agency_ids[a]);
    }

    // start worker threads
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

    // wait for all threads to finish
    for (int a = 0; a < num_agencies; a++) pthread_join(reception_threads[a], NULL);
    for (int i = 0; i < total_workers; i++) pthread_join(threads[i], NULL);

    // stop logger thread
    log_stop();

    // verify final results
    check_final_results();

    // clean up memory and locks
    for (int i = 0; i < num_agencies; i++) {
        queue_destroy(&agency_queues[i]);
    }
    db_destroy(&db);

    return 0;
}
