#ifndef GARLIC_THREAD_POOL_H
#define GARLIC_THREAD_POOL_H

#include "mem_pool.h"

#define MAX_THREADS 64
#define MAX_QUEUE 65536

typedef struct threadpool_t threadpool_t;


typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

struct threadpool_t {
    int init_count;
    pthread_mutex_t *lock;
    pthread_cond_t *notify;
    pthread_cond_t *init_cond;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
    mem_pool *mem_pool;
};

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

static pthread_key_t tls_key;
static pthread_once_t tls_init_once = PTHREAD_ONCE_INIT;

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
