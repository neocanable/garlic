
#ifndef GARLIC_EXCEPTION_H
#define GARLIC_EXCEPTION_H

#include "decompiler/structure.h"

void identify_exception_handler_block_end(jd_method *m);

void cleanup_full_exception_table(jd_method *m);

void copy_exceptions_closed2cfg(jd_method *m);

void identify_finally_excpetion_handler_block_end(jd_method *m);

void flatten_exceptions(jd_method *m);

void pullin_block_jump_into_exception_try_block(jd_method *m);

jd_exc* closest_exception_of(jd_method *m, uint32_t offset);


// exception sort

static inline int block_id_cmp(jd_bblock *block1, jd_bblock *block2)
{
    return block1->block_id - block2->block_id;
}

static inline int handler_start_cmp(jd_exc *e1, jd_exc *e2)
{
    return e1->handler_start > e2->handler_start ? 1 : 0;
}

static inline int handler_end_cmp(jd_exc *e1, jd_exc *e2)
{
    return e1->handler_end > e2->handler_end ? 1 : 0;
}

static inline int try_start_cmp(jd_exc *e1, jd_exc *e2)
{
    return e1->try_start > e2->try_start ? 1 : 0;
}

static inline int try_end_cmp(jd_exc *e1, jd_exc *e2)
{
    return e1->try_end > e2->try_end ? 1 : 0;
}

static inline int try_contains_cmp(jd_exc *e1, jd_exc *e2)
{
    return e1->try_start <= e2->try_start &&
           e1->try_end >= e2->try_end ? -1 : 1;
}

static inline int handler_contains_cmp(jd_exc *e1, jd_exc *e2)
{
    return e1->handler_start <= e2->handler_start &&
           e1->handler_end >= e2->handler_end ? -1 : 1;
}

void buble_sort_cfg_exception(jd_method *m, list_cmp_fn fn);

#endif //GARLIC_EXCEPTION_H
