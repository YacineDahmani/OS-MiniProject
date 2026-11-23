#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/booking.h"

// initialize database and routes
void db_init(Database *db) {
    db->tickets_sold = 0;
    db->cancellations = 0;
    pthread_mutex_init(&db->global_lock, NULL);
    pthread_mutex_init(&db->stats_lock, NULL);

    const char *cities[num_routes][2] = {
        {"algiers", "bouira"},
        {"bouira", "bejaia"},
        {"bejaia", "setif"},
        {"algiers", "adrar"},
        {"adrar", "oran"}
    };

    for (int i = 0; i < num_routes; i++) {
        db->routes[i].id = i;
        db->routes[i].free_seats = seats_per_route;
        strncpy(db->routes[i].from, cities[i][0], 31);
        strncpy(db->routes[i].to, cities[i][1], 31);
        pthread_mutex_init(&db->routes[i].lock, NULL);
        for (int s = 0; s < seats_per_route; s++) {
            db->routes[i].seats[s] = 0;
        }
    }
}

// destroy database mutexes
void db_destroy(Database *db) {
    pthread_mutex_destroy(&db->global_lock);
    pthread_mutex_destroy(&db->stats_lock);
    for (int i = 0; i < num_routes; i++) {
        pthread_mutex_destroy(&db->routes[i].lock);
    }
}

// book first available seat
int book_seat(Database *db, int route_id, int client_id, int use_route_lock) {
    if (route_id < 0 || route_id >= num_routes) return -1;

    // use per-route lock or global lock depending on mode
    pthread_mutex_t *lock_to_use;
    if (use_route_lock) {
        lock_to_use = &db->routes[route_id].lock;
    } else {
        lock_to_use = &db->global_lock;
    }

    pthread_mutex_lock(lock_to_use);

    for (int s = 0; s < seats_per_route; s++) {
        if (db->routes[route_id].seats[s] == 0) {
            usleep(1000);
            db->routes[route_id].seats[s] = client_id;
            db->routes[route_id].free_seats--;

            pthread_mutex_unlock(lock_to_use);

            pthread_mutex_lock(&db->stats_lock);
            db->tickets_sold++;
            pthread_mutex_unlock(&db->stats_lock);

            return s;
        }
    }

    pthread_mutex_unlock(lock_to_use);
    return -1;
}

// cancel client seat on given route
int cancel_seat(Database *db, int route_id, int client_id) {
    if (route_id < 0 || route_id >= num_routes) return -1;

    pthread_mutex_lock(&db->routes[route_id].lock);

    for (int s = 0; s < seats_per_route; s++) {
        if (db->routes[route_id].seats[s] == client_id) {
            db->routes[route_id].seats[s] = 0;
            db->routes[route_id].free_seats++;

            pthread_mutex_unlock(&db->routes[route_id].lock);

            pthread_mutex_lock(&db->stats_lock);
            db->cancellations++;
            pthread_mutex_unlock(&db->stats_lock);

            return s;
        }
    }

    pthread_mutex_unlock(&db->routes[route_id].lock);
    return -1;
}

// check number of free seats on a route
int check_seats(Database *db, int route_id) {
    if (route_id < 0 || route_id >= num_routes) return -1;

    pthread_mutex_lock(&db->routes[route_id].lock);
    int free = db->routes[route_id].free_seats;
    pthread_mutex_unlock(&db->routes[route_id].lock);

    return free;
}

// book roundtrip without deadlock
int book_roundtrip(Database *db, int route1_id, int route2_id, int client_id) {
    // lock lower route index first to prevent circular wait deadlock
    int first_route;
    int second_route;
    if (route1_id < route2_id) {
        first_route = route1_id;
        second_route = route2_id;
    } else {
        first_route = route2_id;
        second_route = route1_id;
    }

    pthread_mutex_lock(&db->routes[first_route].lock);
    pthread_mutex_lock(&db->routes[second_route].lock);

    int seat1 = -1, seat2 = -1;
    for (int s = 0; s < seats_per_route; s++) {
        if (db->routes[route1_id].seats[s] == 0) { seat1 = s; break; }
    }
    for (int s = 0; s < seats_per_route; s++) {
        if (db->routes[route2_id].seats[s] == 0) { seat2 = s; break; }
    }

    if (seat1 != -1 && seat2 != -1) {
        db->routes[route1_id].seats[seat1] = client_id;
        db->routes[route1_id].free_seats--;
        db->routes[route2_id].seats[seat2] = client_id;
        db->routes[route2_id].free_seats--;

        pthread_mutex_unlock(&db->routes[second_route].lock);
        pthread_mutex_unlock(&db->routes[first_route].lock);

        pthread_mutex_lock(&db->stats_lock);
        db->tickets_sold += 2;
        pthread_mutex_unlock(&db->stats_lock);

        return 0;
    }

    pthread_mutex_unlock(&db->routes[second_route].lock);
    pthread_mutex_unlock(&db->routes[first_route].lock);
    return -1;
}
