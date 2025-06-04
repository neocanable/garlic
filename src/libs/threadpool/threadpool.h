
#ifndef GARLIC_THREAD_POOL_H
#define GARLIC_THREAD_POOL_H

#include "mem_pool.h"
#include <pthread.h>

// External declarations for TLS globals
extern pthread_key_t tls_key;
extern pthread_once_t tls_init_once;

#define MAX_THREADS 64
#define MAX_QUEUE 65536

typedef struct threadpool_t threadpool_t;

typedef enum {
    threadpool_invalid        = -1,
    threadpool_lock_failure   = -2,
    threadpool_queue_full     = -3,
    threadpool_shutdown       = -4,
    threadpool_thread_failure = -5
} threadpool_error_t;

typedef enum {
    threadpool_graceful       = 1
} threadpool_destroy_flags_t;

typedef struct {
    int thread_id;
    mem_pool *pool;
} thread_local_data;

threadpool_t* threadpool_create_in(mem_pool *mem_pool,
                                   int cnt,
                                   int flags);

int threadpool_add(threadpool_t *pool,
                   void (*routine)(void *),
                   void *arg,
                   int flags);

int threadpool_destroy(threadpool_t *pool, int flags);

void thread_local_data_init(threadpool_t *pool, pthread_t tid);

thread_local_data* get_thread_local_data();

void create_tls_key();

#endif /* GARLIC_THREAD_POOL_H */