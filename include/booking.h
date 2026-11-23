#ifndef BOOKING_H
#define BOOKING_H

#include "types.h"

// initialize routes, cities, seats and mutexes
void db_init(Database *db);

// destroy all mutexes in the database
void db_destroy(Database *db);

// book a seat on a route (returns seat index or -1 if full)
int book_seat(Database *db, int route_id, int client_id, int use_route_lock);

// cancel a reservation
int cancel_seat(Database *db, int route_id, int client_id);

// check free seats count 
int check_seats(Database *db, int route_id);

// book roundtrip tickets without deadlock
int book_roundtrip(Database *db, int route1_id, int route2_id, int client_id);

#endif
