#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "threadpool.h"
#include "debug.h"

// Thread-local storage globals - declared here to have external linkage
pthread_key_t tls_key;
pthread_once_t tls_init_once = PTHREAD_ONCE_INIT;
static int tls_key_initialized = 0;

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
    pthread_mutex_t lock;
    pthread_cond_t notify;
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

static void *threadpool_thread(void *threadpool);

int threadpool_free(threadpool_t *pool);

void create_tls_key() {
    int result = pthread_key_create(&tls_key, NULL);
    
    if (result != 0) {
        fprintf(stderr, "ERROR: pthread_key_create failed with code: %d\n", result);
        exit(EXIT_FAILURE);
    }
    
    tls_key_initialized = 1;
}

thread_local_data* get_thread_local_data() {
    if (!tls_key_initialized) {
        return NULL;
    }
    
    thread_local_data *tls = pthread_getspecific(tls_key);
    return tls;
}

threadpool_t* threadpool_create_in(mem_pool *mem_pool, int cnt, int flags)
{
    threadpool_t *pool;
    int i;
    (void) flags;

    if(cnt <= 0 || cnt > MAX_THREADS)
        return NULL;

    pool = x_alloc_in(mem_pool, sizeof(threadpool_t));
    pool->thread_count = cnt;
    pool->mem_pool = mem_pool;
    pool->queue_size = MAX_QUEUE;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    pool->threads = x_alloc_in(mem_pool, sizeof(pthread_t) * cnt);
    pool->queue = x_alloc_in(mem_pool, sizeof(threadpool_task_t) * MAX_QUEUE);

    if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
       (pthread_cond_init(&(pool->notify), NULL) != 0) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL)) {
        goto err;
    }

    for(i = 0; i < cnt; i++) {
        if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0) {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;

    err:
    if(pool) {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_add(threadpool_t *pool,
                   void (*function)(void *),
                   void *argument, int flags)
{
    int err = 0;
    int next;
    (void) flags;

    if(pool == NULL || function == NULL) {
        return threadpool_invalid;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) {
        return threadpool_lock_failure;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        if(pool->count == pool->queue_size) {
            err = threadpool_queue_full;
            break;
        }

        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;

        if(pthread_cond_signal(&(pool->notify)) != 0) {
            err = threadpool_lock_failure;
            break;
        }
    } while(0);

    if(pthread_mutex_unlock(&pool->lock) != 0) {
        err = threadpool_lock_failure;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if(pool == NULL) {
        return threadpool_invalid;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) {
        return threadpool_lock_failure;
    }

    do {
        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ?
                         graceful_shutdown : immediate_shutdown;

        if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = threadpool_lock_failure;
            break;
        }

        for(i = 0; i < pool->thread_count; i++) {
            if(pthread_join(pool->threads[i], NULL) != 0) {
                err = threadpool_thread_failure;
            }
        }
    } while(0);

    if(!err) {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    if(pool->threads) {
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    return 0;
}

void thread_local_data_init(threadpool_t *pool, pthread_t tid) {
    // printf("DEBUG: thread_local_data_init called for thread %lu\n", (unsigned long)tid);
    
    pthread_once(&tls_init_once, create_tls_key);
    // printf("DEBUG: tls_key after pthread_once: %u, initialized: %d\n", 
    //        (unsigned int)tls_key, tls_key_initialized);
    
    if (!tls_key_initialized) {
        fprintf(stderr, "ERROR: TLS key initialization failed\n");
        exit(EXIT_FAILURE);
    }
    
    thread_local_data *tls = x_alloc_in(pool->mem_pool, sizeof(thread_local_data));
    // printf("DEBUG: allocated thread_local_data: %p\n", (void*)tls);
    
    if (!tls) {
        perror("Failed to allocate thread local storage");
        exit(EXIT_FAILURE);
    }
    
    // Initialize the thread local data
    memset(tls, 0, sizeof(thread_local_data));
    tls->thread_id = 0; // Will be set later if needed
    tls->pool = NULL;   // Will be set by jar_entry_thread_task
    
    int result = pthread_setspecific(tls_key, tls);
    // printf("DEBUG: pthread_setspecific returned: %d\n", result);
    
    if (result != 0) {
        fprintf(stderr, "ERROR: pthread_setspecific failed with code: %d\n", result);
        exit(EXIT_FAILURE);
    }
    
    // Verify it was set correctly
    thread_local_data *verify_tls = pthread_getspecific(tls_key);
    // printf("DEBUG: verification - retrieved tls: %p (should match %p)\n", 
    //        (void*)verify_tls, (void*)tls);
}

static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    thread_local_data_init(threadpool, pthread_self());

    for(;;) {
        pthread_mutex_lock(&(pool->lock));

        while((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if((pool->shutdown == immediate_shutdown) ||
           ((pool->shutdown == graceful_shutdown) &&
            (pool->count == 0))) {
            break;
        }

        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        (*(task.function))(task.argument);
    }
    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}