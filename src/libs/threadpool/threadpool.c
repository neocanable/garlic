#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "threadpool.h"
#include "debug.h"

static void *threadpool_thread(void *threadpool);

int threadpool_free(threadpool_t *pool);

void create_tls_key() {
    pthread_key_create(&tls_key, NULL);
}

thread_local_data* get_thread_local_data() {
    return pthread_getspecific(tls_key);
}

threadpool_t* threadpool_create_in(mem_pool *mem_pool, int cnt, int flags)
{
    threadpool_t *pool;
    int i;
    (void) flags;

    if(cnt <= 0 || cnt > MAX_THREADS)
        return NULL;

    pool = x_alloc_in(mem_pool, sizeof(threadpool_t));
    memset(pool, 0, sizeof(threadpool_t));
    pool->thread_count = cnt;
    pool->mem_pool = mem_pool;
    pool->queue_size = MAX_QUEUE;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    pool->threads = x_alloc_in(mem_pool, sizeof(pthread_t) * cnt);
    pool->queue = x_alloc_in(mem_pool, sizeof(threadpool_task_t) * MAX_QUEUE);
    pool->init_count = 0;

    pool->lock = malloc(sizeof(pthread_mutex_t));
    pool->init_cond = malloc(sizeof(pthread_cond_t));
    pool->notify = malloc(sizeof(pthread_cond_t));

    if((pthread_mutex_init(pool->lock, NULL) != 0) ||
       (pthread_cond_init(pool->notify, NULL) != 0) ||
       (pthread_cond_init(pool->init_cond, NULL) != 0) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL)) {
        goto err;
    }

    for(i = 0; i < cnt; i++) {
        int _result = pthread_create(&pool->threads[i],
                                     NULL,
                                     threadpool_thread,
                                     pool);
        if(_result != 0) {
            pthread_mutex_unlock(pool->lock);
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->started++;
    }

    pthread_mutex_lock(pool->lock);
    while (pool->init_count < pool->thread_count) {
        pthread_cond_wait(pool->init_cond, pool->lock);
    }
    pthread_mutex_unlock(pool->lock);

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

    if(pthread_mutex_lock(pool->lock) != 0) {
        return threadpool_lock_failure;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        if(pool->count == pool->queue_size) {
            size_t old = sizeof(threadpool_task_t) * pool->queue_size;
            size_t _new = pool->queue_size * 2 * sizeof(threadpool_task_t);
            pool->queue = x_realloc(pool->queue,old,_new);
        }

        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;

        if(pthread_cond_signal(pool->notify) != 0) {
            err = threadpool_lock_failure;
            break;
        }
    } while(0);

    if(pthread_mutex_unlock(pool->lock) != 0) {
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

    if(pthread_mutex_lock(pool->lock) != 0) {
        return threadpool_lock_failure;
    }

    do {
        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ?
                         graceful_shutdown : immediate_shutdown;

        if (pthread_cond_broadcast(pool->notify) != 0) {
            err = threadpool_lock_failure;
            break;
        }

        if (pthread_mutex_unlock(pool->lock) != 0) {
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

//    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(pool->lock);
    pthread_cond_destroy(pool->notify);
    pthread_cond_destroy(pool->init_cond);

    free(pool->lock);
    free(pool->notify);
    free(pool->init_cond);
    return 0;
}

void thread_local_data_init(threadpool_t *pool, pthread_t tid) {
    pthread_once(&tls_init_once, create_tls_key);
    mem_pool *mpool = pool->mem_pool;
    thread_local_data *tls = x_alloc_in(mpool, sizeof(thread_local_data));
    if (!tls) {
        perror("Failed to allocate thread local storage");
        exit(EXIT_FAILURE);
    }
    pthread_setspecific(tls_key, tls);
}

static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    thread_local_data_init(threadpool, pthread_self());

    pthread_mutex_lock(pool->lock);
    pool->init_count++;
    pthread_cond_signal(pool->init_cond);
    pthread_mutex_unlock(pool->lock);

    for(;;) {
        pthread_mutex_lock(pool->lock);

        while((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(pool->notify, pool->lock);
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

        pthread_mutex_unlock(pool->lock);

        (*(task.function))(task.argument);
    }
    pool->started--;
    pthread_mutex_unlock(pool->lock);
    pthread_exit(NULL);
    return(NULL);
}
