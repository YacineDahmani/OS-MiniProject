#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "../include/log.h"

// queue and thread handle for asynchronous logging
static LogQueue lq;
static pthread_t logger_tid;
static FILE *fp = NULL;

// background thread that pulls log lines from queue and writes to disk
static void *logger_routine(void *arg) {
    (void)arg;
    fp = fopen("journal.log", "w");
    if (!fp) return NULL;

    while (1) {
        // wait for a log message to arrive
        pthread_mutex_lock(&lq.mutex);
        while (lq.count == 0) {
            pthread_cond_wait(&lq.not_empty, &lq.mutex);
        }

        LogItem item = lq.items[lq.out];
        lq.out = (lq.out + 1) % log_queue_size;
        lq.count--;

        pthread_cond_signal(&lq.not_full);
        pthread_mutex_unlock(&lq.mutex);

        // stop when stop signal arrives
        if (item.is_stop) break;

        // format timestamp for log record
        time_t now = time(NULL);
        struct tm info;
#if defined(_WIN32)
        localtime_s(&info, &now);
#else
        localtime_r(&now, &info);
#endif
        char tstr[32];
        strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &info);

        fprintf(fp, "[%s] %s\n", tstr, item.text);
        fflush(fp);
    }

    fclose(fp);
    return NULL;
}

// set up circular queue for logging
void log_init(void) {
    lq.in = 0;
    lq.out = 0;
    lq.count = 0;
    pthread_mutex_init(&lq.mutex, NULL);
    pthread_cond_init(&lq.not_full, NULL);
    pthread_cond_init(&lq.not_empty, NULL);
}

// launch the background disk writer thread
void log_start(void) {
    pthread_create(&logger_tid, NULL, logger_routine, NULL);
}

// format and enqueue a log line directly without caller buffer
void log_write(const char *fmt, ...) {
    pthread_mutex_lock(&lq.mutex);
    while (lq.count == log_queue_size) {
        pthread_cond_wait(&lq.not_full, &lq.mutex);
    }

    // write formatted string directly into queue entry
    va_list args;
    va_start(args, fmt);
    vsnprintf(lq.items[lq.in].text, sizeof(lq.items[lq.in].text), fmt, args);
    va_end(args);
    lq.items[lq.in].is_stop = 0;

    lq.in = (lq.in + 1) % log_queue_size;
    lq.count++;

    pthread_cond_signal(&lq.not_empty);
    pthread_mutex_unlock(&lq.mutex);
}

// non-blocking write to prevent clerk thread deadlocks
int log_try_write(const char *fmt, ...) {
    pthread_mutex_lock(&lq.mutex);
    if (lq.count == log_queue_size) {
        pthread_mutex_unlock(&lq.mutex);
        return -1;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(lq.items[lq.in].text, sizeof(lq.items[lq.in].text), fmt, args);
    va_end(args);
    lq.items[lq.in].is_stop = 0;

    lq.in = (lq.in + 1) % log_queue_size;
    lq.count++;

    pthread_cond_signal(&lq.not_empty);
    pthread_mutex_unlock(&lq.mutex);
    return 0;
}

// flush remaining logs, signal exit and wait for thread to terminate
void log_stop(void) {
    pthread_mutex_lock(&lq.mutex);
    while (lq.count == log_queue_size) {
        pthread_cond_wait(&lq.not_full, &lq.mutex);
    }

    // tell logger thread to exit
    lq.items[lq.in].is_stop = 1;
    lq.in = (lq.in + 1) % log_queue_size;
    lq.count++;

    pthread_cond_signal(&lq.not_empty);
    pthread_mutex_unlock(&lq.mutex);

    pthread_join(logger_tid, NULL);

    pthread_mutex_destroy(&lq.mutex);
    pthread_cond_destroy(&lq.not_full);
    pthread_cond_destroy(&lq.not_empty);
}
