#ifndef GARLIC_MEM_POOL_H
#define GARLIC_MEM_POOL_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "common/types.h"
#include "mem_common.h"

typedef struct small_block {
    u1                  *cur_usable_buffer;
    u1                  *buffer_end;
    struct small_block  *next_block;
    int                 no_enough_times;
} small_block;

typedef struct big_block {
    u1                  *big_buffer;
    struct big_block    *next_block;
} big_block;

typedef struct mem_pool {
    size_t          total_size;
    size_t          small_buffer_capacity;
    small_block     *cur_usable_small_block;
    big_block       *big_block_start;
    small_block     small_block_start[0];

} mem_pool;


mem_pool* mem_pool_init(size_t capacity);

void mem_pool_free(mem_pool* pool);

void mem_pool_clear(mem_pool *pool);

u1* mem_pool_new_small_block(mem_pool* pool, size_t size);

u1* mem_pool_new_big_block(mem_pool* pool, size_t size);

void* mem_pool_alloc(mem_pool* pool, size_t size);

void mem_pool_free_big_block(mem_pool* pool, u1* buffer_ptr);

void* mem_pool_realloc(mem_pool *pool,
                       void *ptr,
                       size_t old_size,
                       size_t new_size);


mem_pool *global_pool;

void mem_init_pool();

mem_pool* mem_create_pool();

void* x_alloc(size_t size);

void* x_alloc_in(mem_pool *pool, size_t size);

void* x_realloc(void *ptr, size_t old_size, size_t new_size);

void* x_realloc_in(mem_pool *pool,
                   void *ptr,
                   size_t old_size,
                   size_t new_size);

void mem_free_pool();


#define make_obj(type) (x_alloc(sizeof(type)))

#define make_obj_in(type, pool) (mem_pool_alloc(pool, sizeof(type)))

#define make_obj_arr(type, size) (x_alloc(sizeof(type) * (size)))

#define make_obj_arr_in(type, size, pool) \
    (mem_pool_alloc(pool, sizeof(type) * (size)))
#endif //GARLIC_MEM_POOL_H
