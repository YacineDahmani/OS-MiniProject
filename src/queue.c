#include <stdio.h>
#include <pthread.h>
#include "../include/queue.h"

// initialize queue pointers and conditions
void queue_init(Queue *q) {
    q->in = 0;
    q->out = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

// insert request into queue (waits if full)
void queue_push(Queue *q, Request req) {
    pthread_mutex_lock(&q->mutex);

    // wait if queue is full
    while (q->count == queue_size) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    q->items[q->in] = req;
    q->in = (q->in + 1) % queue_size;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

// remove request from queue (waits if empty)
Request queue_pop(Queue *q) {
    pthread_mutex_lock(&q->mutex);

    // wait if queue is empty
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    Request req = q->items[q->out];
    q->out = (q->out + 1) % queue_size;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);

    return req;
}

// destroy queue mutex and condition variables
void queue_destroy(Queue *q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
}
