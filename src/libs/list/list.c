#include "list.h"
#include <assert.h>

#define DEFINE_ARRAY_LIST(type)                                               \
    list_##type* linit_##type()                                               \
    {                                                                         \
        list_##type* list = (list_##type*) x_alloc(sizeof(list_##type));      \
        list->data = (type*)x_alloc(LIST_INITIAL_CAPACITY * sizeof(type));    \
        list->size = 0;                                                       \
        list->capacity = LIST_INITIAL_CAPACITY;                               \
        list->cmp_fn = NULL;                                                  \
        return list;                                                          \
    }                                                                         \
                                                                              \
    list_##type* linit_##type##_with_fn(list_cmp_fn cmp_fn)                   \
    {                                                                         \
        list_##type* list = linit_##type();                                   \
        list->cmp_fn = cmp_fn;                                                \
        return list;                                                          \
    }                                                                         \
                                                                              \
    list_##type* linit_##type##_with_capacity(size_t capacity)                \
    {                                                                         \
        list_##type* list = (list_##type*) x_alloc(sizeof(list_##type));      \
        list->data = (type*)x_alloc(capacity * sizeof(type));                 \
        list->size = 0;                                                       \
        list->capacity = capacity;                                            \
        list->cmp_fn = NULL;                                                  \
        return list;                                                          \
    }                                                                         \
                                                                              \
    list_##type* linit_##type##_with_pool(mem_pool *pool)                     \
    {                                                                         \
        list_##type* list = (list_##type*) x_alloc_in(pool,                   \
                sizeof(list_##type));                                         \
        list->data = (type*)x_alloc_in(pool,                                  \
                LIST_INITIAL_CAPACITY * sizeof(type));                        \
        list->size = 0;                                                       \
        list->capacity = LIST_INITIAL_CAPACITY;                               \
        list->cmp_fn = NULL;                                                  \
        list->pool = pool;                                                    \
        return list;                                                          \
    }                                                                         \
                                                                              \
    list_##type* linit_##type##_with_fn_and_pool(list_cmp_fn cmp_fn,          \
            mem_pool *pool)                                                   \
    {                                                                         \
        list_##type* list = linit_##type##_with_pool(pool);                   \
        list->cmp_fn = cmp_fn;                                                \
        return list;                                                          \
    }                                                                         \
                                                                              \
    list_##type* linit_##type##_with_capacity_and_pool(size_t capacity,       \
            mem_pool *pool)                                                   \
    {                                                                         \
        list_##type* list = (list_##type*) x_alloc_in(pool,                   \
                sizeof(list_##type));                                         \
        list->data = (type*)x_alloc_in(pool, capacity * sizeof(type));        \
        list->size = 0;                                                       \
        list->capacity = capacity;                                            \
        list->cmp_fn = NULL;                                                  \
        list->pool = pool;                                                    \
        return list;                                                          \
    }                                                                         \
                                                                              \
    void ladd_##type(list_##type* list, type element)                         \
    {                                                                         \
        if (list->size == list->capacity) {                                   \
            size_t old_size = list->capacity;                                 \
            list->capacity *= LIST_GROWTH_FACTOR;                             \
            if (list->pool == NULL)                                           \
                list->data = (type*)x_realloc(list->data,                     \
                    old_size*sizeof(type),                                    \
                    list->capacity * sizeof(type));                           \
            else                                                              \
                list->data = (type*)x_realloc_in(list->pool, list->data,      \
                    old_size*sizeof(type),                                    \
                    list->capacity * sizeof(type));                           \
        }                                                                     \
        list->data[list->size++] = element;                                   \
    }                                                                         \
                                                                              \
    void lremove_##type(list_##type* list, size_t index)                      \
    {                                                                         \
        if (index < list->size) {                                             \
            for (size_t i = index; i < list->size - 1; i++) {                 \
                list->data[i] = list->data[i + 1];                            \
            }                                                                 \
            list->size--;                                                     \
        }                                                                     \
    }                                                                         \
                                                                              \
    void lclear_##type(list_##type* list)                                     \
    {                                                                         \
        list->size = 0;                                                       \
        memset(list->data, 0, list->capacity * sizeof(type));                 \
    }                                                                         \
                                                                              \
                                                                              \
    type lget_##type(list_##type* list, size_t index)                         \
    {                                                                         \
        assert(index < list->size);                                           \
        if (index < list->size) {                                             \
            return list->data[index];                                         \
        } else {                                                              \
            fprintf(stderr, "Index out of bounds\n");                         \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    }                                                                         \
                                                                              \
    type lget_##type##_first(list_##type* list)                               \
    {                                                                         \
        return lget_##type(list, 0);                                          \
    }                                                                         \
                                                                              \
    type lget_##type##_last(list_##type* list)                                \
    {                                                                         \
        return lget_##type(list, list->size-1);                               \
    }                                                                         \
                                                                              \
    void lsort_##type(list_##type* list, list_cmp_fn comparator)              \
    {                                                                         \
        qsort(list->data, list->size, sizeof(type), comparator);              \
    }                                                                         \
                                                                              \
    int ldel_##type(list_##type* list, type el)                               \
    {                                                                         \
        int index = lfind_##type(list, el);                                   \
        if (index >= 0) lremove_##type(list, index);                          \
        return index;                                                         \
    }                                                                         \
                                                                              \
    void lfree_##type(list_##type* list) {                                    \
        free(list->data);                                                     \
        free(list);                                                           \
    }                                                                         \


#define DEFINE_ARRAY_LIST_BASIC_FIND(type)                                    \
    int lfind_##type(list_##type* list, type element)                         \
    {                                                                         \
        for (int i = 0; i < list->size; i++) {                                \
            if (list->data[i] == element) {                                   \
                return i;                                                     \
            }                                                                 \
        }                                                                     \
        return -1;                                                            \
    }                                                                         \


#define DEFINE_ARRAY_LIST_CMP_FIND(type)                                      \
    int lfind_##type(list_##type* list, type element)                         \
    {                                                                         \
        for (size_t i = 0; i < list->size; i++) {                             \
            if (list->cmp_fn == NULL)                                         \
            {                                                                 \
                if (list->data[i] == element) return i;                       \
            }                                                                 \
            else                                                              \
            {                                                                 \
                if(list->cmp_fn(list->data[i], element)) return i;            \
            }                                                                 \
        }                                                                     \
        return -1;                                                            \
    }                                                                         \


DEFINE_ARRAY_LIST(int);
DEFINE_ARRAY_LIST_BASIC_FIND(int);

DEFINE_ARRAY_LIST(string);
DEFINE_ARRAY_LIST_CMP_FIND(string);

DEFINE_ARRAY_LIST(object);
DEFINE_ARRAY_LIST_CMP_FIND(object);

void ladd_obj_at(list_object *list, object element, size_t index)
{
    if (list->size == list->capacity) {
        size_t old_size = list->capacity;
        list->capacity *= LIST_GROWTH_FACTOR;
        size_t _old = old_size*sizeof(object*);
        size_t _new = list->capacity*sizeof(object*);
        if (list->pool == NULL)
            list->data = (object*)x_realloc(list->data,_old, _new);
        else
            list->data = (object*)x_realloc_in(list->pool, 
                                               list->data, 
                                               _old, 
                                               _new);
    }
    for (size_t i = list->size; i > index; i--) {
        list->data[i] = list->data[i - 1];
    }
    list->data[index] = element;
    list->size++;
}

string lstring_join(list_string *list, string delimiter)
{
    if (list->size == 0)
        return "";

    size_t length = 0;
    for (int i = 0; i < list->size; ++i)
        length += strlen(lget_string(list, i));

    length += strlen(delimiter) * list->size - 1;
    string result = x_alloc(length);
    for (int i = 0; i < list->size; ++i) {
        string item = lget_string(list, i);
        strcat(result, item);
        if (i < list->size - 1)
            strcat(result, delimiter);
    }
    return result;
}

// chatgpt generated
string lint_join(list_int *list, string delimiter)
{
    if (list->size == 0)
        return NULL;
    size_t total_length = list->size * 12 + (list->size - 1) + 1;
    char* result = x_alloc(total_length);

    size_t index = 0;
    for (size_t i = 0; i < list->size; ++i) {
        int written = snprintf(result + index, 
                total_length - index, "%d", list->data[i]);

        if (written < 0 || written >= total_length - index)
            perror("String formatting error");

        index += written;
        if (i < list->size - 1)
            result[index++] = ',';
    }
    result[index] = '\0';
    return result;
}

void lswap_obj(list_object *list,
                      size_t index1,
                      size_t index2)
{
    if (index1 >= list->size || index2 >= list->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    void *temp = list->data[index1];
    list->data[index1] = list->data[index2];
    list->data[index2] = temp;
}
