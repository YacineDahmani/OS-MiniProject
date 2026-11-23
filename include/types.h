#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>

// system constants
#define num_routes 5
#define seats_per_route 40
#define num_agencies 3
#define workers_per_agency 4
#define total_workers (num_agencies * workers_per_agency)
#define queue_size 8
#define log_queue_size 128

// request types
enum {
    req_book,
    req_cancel,
    req_check,
    req_roundtrip,
    req_stop
};

// client request structure
typedef struct {
    int type;
    int agency_id;
    int worker_id;
    int client_id;
    int route_id;
    int return_route_id;
} Request;

// route data
typedef struct {
    int id;
    char from[32];
    char to[32];
    int seats[seats_per_route];
    int free_seats;
    pthread_mutex_t lock;
} Route;

//database holding all routes and stats
typedef struct {
    Route routes[num_routes];
    int tickets_sold;
    int cancellations;
    pthread_mutex_t global_lock;
    pthread_mutex_t stats_lock;
} Database;

//  queue for agency requests
typedef struct {
    Request items[queue_size];
    int in;
    int out;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} Queue;

// single log entry sent to the background logger
typedef struct {
    char text[160];
    int is_stop;
} LogItem;

// circular queue for logging messages
typedef struct {
    LogItem items[log_queue_size];
    int in;
    int out;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} LogQueue;

#endif
