#include <stdio.h>
#include "../include/rw_lock.h"

// initialize readers-writers lock
void rw_lock_init(RwLock *rw, int fair_mode) {
    rw->readers = 0;
    rw->writers_waiting = 0;
    rw->writer_active = 0;
    rw->fair_mode = fair_mode;
    pthread_mutex_init(&rw->mutex, NULL);
    pthread_cond_init(&rw->can_read, NULL);
    pthread_cond_init(&rw->can_write, NULL);
}

// destroy rw lock primitives
void rw_lock_destroy(RwLock *rw) {
    pthread_mutex_destroy(&rw->mutex);
    pthread_cond_destroy(&rw->can_read);
    pthread_cond_destroy(&rw->can_write);
}

// acquire read access
void read_lock(RwLock *rw) {
    pthread_mutex_lock(&rw->mutex);

    // in fair mode, wait if writers are waiting to avoid writer starvation
    if (rw->fair_mode) {
        while (rw->writer_active || rw->writers_waiting > 0) {
            pthread_cond_wait(&rw->can_read, &rw->mutex);
        }
    } else {
        while (rw->writer_active) {
            pthread_cond_wait(&rw->can_read, &rw->mutex);
        }
    }

    rw->readers++;
    pthread_mutex_unlock(&rw->mutex);
}

// release read access
void read_unlock(RwLock *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->readers--;

    // last reader wakes up waiting writer
    if (rw->readers == 0) {
        pthread_cond_signal(&rw->can_write);
    }

    pthread_mutex_unlock(&rw->mutex);
}

// acquire exclusive write access
void write_lock(RwLock *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->writers_waiting++;

    // writers need exclusive access (no readers, no active writers)
    while (rw->writer_active || rw->readers > 0) {
        pthread_cond_wait(&rw->can_write, &rw->mutex);
    }

    rw->writers_waiting--;
    rw->writer_active = 1;
    pthread_mutex_unlock(&rw->mutex);
}

// release exclusive write access
void write_unlock(RwLock *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->writer_active = 0;

    pthread_cond_broadcast(&rw->can_read);
    pthread_cond_signal(&rw->can_write);

    pthread_mutex_unlock(&rw->mutex);
}
