#include <stdio.h>
#include <stdlib.h>
#include "mem_pool.h"
#include "libs/threadpool/threadpool.h"

mem_pool *global_pool;

static void* pool_malloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "[garlic] out of memory: %zu bytes refused\n", size);
        exit(EXIT_FAILURE);
    }
    return p;
}

void mem_init_pool() {
    global_pool = mem_pool_init(16 * 1024);
}

mem_pool* mem_create_pool()
{
    return mem_pool_init(16*1024);
}

static int alloc_count = 0;

void* x_alloc(size_t size)
{
    // TODO: use global mem_pool *pool
    thread_local_data *tls = get_thread_local_data();

    if (tls && tls->pool) {
        return mem_pool_alloc(tls->pool, size);
    }
    return mem_pool_alloc(global_pool, size);
}

void* x_alloc_in(mem_pool *pool, size_t size)
{
    return mem_pool_alloc(pool, size);
}

void* x_realloc(void *ptr, size_t old_size, size_t new_size)
{
    thread_local_data *tls = get_thread_local_data();

    if (tls && tls->pool) {
        return mem_pool_realloc(tls->pool, ptr, old_size, new_size);
    }
    return mem_pool_realloc(global_pool, ptr, old_size, new_size);
}

void* x_realloc_in(mem_pool *pool, void *ptr, size_t old_size, size_t new_size)
{
    return mem_pool_realloc(pool, ptr, old_size, new_size);
}

void mem_free_pool() {
    // memset(global_pool, 0, sizeof(mem_pool));
    mem_pool_free(global_pool);
    global_pool = NULL;
}

mem_pool* mem_scratch_enter()
{
    mem_pool *outer = global_pool;
    global_pool = mem_pool_init(16 * 1024);
    return outer;
}

void mem_scratch_leave(mem_pool *outer)
{
    if (global_pool != NULL)
        mem_pool_free(global_pool);
    global_pool = outer;
}

mem_pool* mem_pool_init(size_t capacity)
{
    size_t total_size = sizeof(mem_pool) + sizeof(small_block) + capacity;
    void *temp = pool_malloc(total_size);
    memset(temp, 0, total_size);

    mem_pool *pool = (mem_pool*)temp;
    pool->small_buffer_capacity = capacity;
    pool->big_block_start = NULL;
    memset(pool->small_block_start, 0, sizeof(small_block));
    pool->cur_usable_small_block = (small_block*)(pool->small_block_start);
    pool->total_size = total_size;

    small_block *sbp = (small_block*)(pool + 1);
    sbp->cur_usable_buffer = (u1*)(sbp + 1);
    sbp->buffer_end = sbp->cur_usable_buffer+capacity;
    sbp->next_block = NULL;
    sbp->no_enough_times = 0;

    return pool;
}

void mem_pool_free(mem_pool *pool){
    big_block *bbp = pool->big_block_start;
    while (bbp)
    {
        if (bbp->big_buffer) {
            free(bbp->big_buffer);
            bbp->big_buffer = NULL;
        }
        bbp = bbp->next_block;
    }

    small_block *temp = pool->small_block_start->next_block;
    while (temp) {
        small_block *next = temp->next_block;
        free(temp);
        temp = next;
    }
    free(pool);
}

u1* mem_pool_new_small_block(mem_pool *pool, size_t size)
{
    size_t malloc_size = sizeof(small_block) + pool->small_buffer_capacity;
    pool->total_size += malloc_size;
    void *temp = pool_malloc(malloc_size);
    memset(temp, 0, malloc_size);

    small_block *sbp = (small_block*) temp;
    sbp->cur_usable_buffer = (u1*)(sbp+1);
    sbp->buffer_end = (u1*)temp + malloc_size;
    sbp->next_block = NULL;
    sbp->no_enough_times = 0;

    u1 *res = sbp->cur_usable_buffer;
    sbp->cur_usable_buffer = res + size;


    small_block *p = pool->cur_usable_small_block;
    while (p->next_block) {
        if (p->no_enough_times > 4) {
            pool->cur_usable_small_block = p->next_block;
        }
        ++(p->no_enough_times);
        p = p->next_block;
    }

    p->next_block = sbp;

    if (pool->cur_usable_small_block == NULL) {
        pool->cur_usable_small_block = sbp;
    }
    return res;
}

static int big_block_count = 0;

u1* mem_pool_new_big_block(mem_pool *pool, size_t size)
{
    big_block_count++;
    void *temp = pool_malloc(size);
    memset(temp, 0, size);
    pool->total_size += size;
    big_block *bbp = pool->big_block_start;
    int i = 0;
    while (bbp) {
        if (bbp->big_buffer == NULL) {
            bbp->big_buffer = (u1*)temp;
            return bbp->big_buffer;
        }
        if (i > 3)
            break;
        bbp = bbp->next_block;
        ++i;
    }

    big_block *new_bbp = (big_block*) mem_pool_alloc(pool, sizeof(big_block));
    new_bbp->big_buffer = (u1*)temp;
    new_bbp->next_block = pool->big_block_start;
    pool->big_block_start = new_bbp;
    return new_bbp->big_buffer;
}

static int pool_zero_on = -1;
static void* pool_zero(void *p, size_t size)
{
    if (pool_zero_on < 0)
        pool_zero_on = getenv("GARLIC_ZEROALL") != NULL;
    if (pool_zero_on && p != NULL)
        memset(p, 0, size);
    return p;
}

void* mem_pool_alloc(mem_pool *pool, size_t size){
    if(size < pool->small_buffer_capacity) {
        small_block *temp = pool->cur_usable_small_block;
        do {
            if (temp->buffer_end-temp->cur_usable_buffer > size) {
                u1 *res = temp->cur_usable_buffer;
                temp->cur_usable_buffer = temp->cur_usable_buffer + size;
                return pool_zero(res, size);
            }
            temp = temp->next_block;
        } while (temp);
        return mem_pool_new_small_block(pool, size);
    }
    return mem_pool_new_big_block(pool, size);
}

void* mem_pool_realloc(mem_pool *pool, void *ptr, size_t old_size, 
                       size_t new_size)
{
    // this is alloc a new pointer shit
    void *new_ptr = mem_pool_alloc(pool, new_size);
    memset(new_ptr, 0, new_size);
    memmove(new_ptr, ptr, old_size);
    return new_ptr;
}

void mem_pool_free_big_block(mem_pool *pool, u1 *buffer_ptr)
{
    big_block *bbp = pool->big_block_start;
    while (bbp) {
        if (bbp->big_buffer == buffer_ptr) {
            free(bbp->big_buffer);
            bbp->big_buffer = NULL;
            return;
        }
        bbp = bbp->next_block;
    }
}
