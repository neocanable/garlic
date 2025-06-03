#ifndef GARLIC_LIST_H
#define GARLIC_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_pool.h"

/**
 * 这里将list初始定义的如此小是因为用到最多的地方是instruction
 * instruction内有4个list, 而且这些list的元素个数一般不会太多
 **/
#define LIST_INITIAL_CAPACITY 4

#define LIST_GROWTH_FACTOR 2

typedef int (*list_cmp_fn)(const void *, const void *);

#define DEFINE_ARRAY_LIST_HEAD(type)                                        \
    typedef struct {                                                        \
        type* data;                                                         \
        size_t size;                                                        \
        size_t capacity;                                                    \
        list_cmp_fn cmp_fn;                                                 \
        mem_pool *pool;                                                     \
    } list_##type;                                                          \
                                                                            \
    list_##type* linit_##type##_with_fn(list_cmp_fn cmp_fn);                \
                                                                            \
    list_##type* linit_##type##_with_capacity(size_t capacity);             \
                                                                            \
    list_##type* linit_##type();                                            \
                                                                            \
    list_##type* linit_##type##_with_fn_and_pool(list_cmp_fn cmp_fn,        \
                                                 mem_pool *pool);           \
                                                                            \
    list_##type* linit_##type##_with_capacity_and_pool(size_t capacity,     \
                                                       mem_pool *pool);     \
                                                                            \
    list_##type* linit_##type##_with_pool(mem_pool *pool);                  \
                                                                            \
    void ladd_##type(list_##type* list, type element);                      \
                                                                            \
    void lremove_##type(list_##type* list, size_t index);                   \
                                                                            \
    int ldel_##type(list_##type* list, type element);                       \
                                                                            \
    void lclear_##type(list_##type* list);                                  \
                                                                            \
    int  lfind_##type(list_##type* list, type element);                     \
                                                                            \
    type lget_##type(list_##type* list, size_t index);                      \
                                                                            \
    type lget_##type##_first(list_##type* list);                            \
                                                                            \
    type lget_##type##_last(list_##type* list);                             \
                                                                            \
    void lsort_##type(list_##type* list, list_cmp_fn comparator);           \
                                                                            \
    void lfree_##type(list_##type* list);                                   \


DEFINE_ARRAY_LIST_HEAD(int);
DEFINE_ARRAY_LIST_HEAD(string);
DEFINE_ARRAY_LIST_HEAD(object);


string lstring_join(list_string *list, string delimiter);

string lint_join(list_int *list, string delimiter);

void lswap_obj(list_object *list, size_t index1, size_t index2);

void ladd_obj_at(list_object *list, object element, size_t index);

#define lcontains_obj(list, element) (lfind_object(list, element) >= 0)

#define lcontains_int(l, e) (lfind_int(l, e) >= 0)

#define ladd_obj_no_dup(list, element) do {         \
    if (!lcontains_obj(list, element)) {            \
        ladd_obj(list, element);                    \
    }                                               \
} while(0)

#define lget_obj(list, index) (lget_object(list, index))
#define ldel_obj(list, element) (ldel_object(list, element))
#define lget_obj_first(list) (lget_object_first(list))
#define lget_obj_last(list)  (lget_object_last(list))
#define ladd_obj(list, element) (ladd_object(list, element))

#define is_list_last(list, index) ((index) == (list)->size - 1)
#define is_list_empty(list) ((list) == NULL || (list)->size == 0)

#endif //GARLIC_LIST_H
