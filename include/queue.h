#ifndef QUEUE_H
#define QUEUE_H

#include "types.h"

// initialize the queue 
void queue_init(Queue *q);

// push request into queue (waits if full)
void queue_push(Queue *q, Request req);

// pop request from queue (waits if empty)
Request queue_pop(Queue *q);

// destroy queue mutex and condition variables
void queue_destroy(Queue *q);

#endif
