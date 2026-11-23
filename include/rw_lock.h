#ifndef RW_LOCK_H
#define RW_LOCK_H

#include <pthread.h>

// custom readers-writers lock
typedef struct {
    int readers;
    int writers_waiting;
    int writer_active;
    int fair_mode;
    pthread_mutex_t mutex;
    pthread_cond_t can_read;
    pthread_cond_t can_write;
} RwLock;

// initialize rw lock with fair or reader-priority mode
void rw_lock_init(RwLock *rw, int fair_mode);

// destroy rw lock mutex and condition variables
void rw_lock_destroy(RwLock *rw);

// lock reading
void read_lock(RwLock *rw);

// unlock reading
void read_unlock(RwLock *rw);

// lock writing
void write_lock(RwLock *rw);

// unlock writing
void write_unlock(RwLock *rw);

#endif
