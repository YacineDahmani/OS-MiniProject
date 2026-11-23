#ifndef LOG_H
#define LOG_H

#include "types.h"

// prepare logger queue and mutexes
void log_init(void);

// start thread that writes to journal.log
void log_start(void);

// write logs
void log_write(const char *fmt, ...);

// try writing to logger queue without blocking
int log_try_write(const char *fmt, ...);

// stop background logger
void log_stop(void);

#endif
