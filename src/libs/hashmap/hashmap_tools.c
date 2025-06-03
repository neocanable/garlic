#include "parser/class/class_tools.h"
#include "libs/hashmap/hashmap_tools.h"
//#include "libs/memory/memory_pool.h"

#define DEFINE_HASHMAP_BODY(tk, tv)                                 \
    tk##_to_##tv*                                                   \
    find_##tk##_to_##tv##_entry(hashmap *_map, tk key)              \
    {                                                               \
        tk##_to_##tv k;                                             \
        hashmap_entry_init(&k, memihash(&key, sizeof(tk)));         \
        k.key = key;                                                \
        return (tk##_to_##tv*) hashmap_get(_map, &k, NULL);         \
    }                                                               \
                                                                    \
    int                                                             \
    tk##_to_##tv##_cmp(const tk##_to_##tv *e1,                      \
            const tk##_to_##tv *e2, const void *unused)             \
    {                                                               \
        return e1->key != e2->key;                                  \
    }                                                               \
                                                                    \
    void                                                            \
    hashmap_set_##tk##_to_##tv(hashmap *_map, tk key, tv value)     \
    {                                                               \
        tk##_to_##tv *e = find_##tk##_to_##tv##_entry(_map, key);   \
        if (!e) {                                                   \
            if (_map->pool == NULL)                                 \
                e = x_alloc(sizeof(tk##_to_##tv));                  \
            else                                                    \
                e = mem_pool_alloc(_map->pool, sizeof(tk##_to_##tv));   \
            hashmap_entry_init(e, memihash(&key, sizeof(tk)));      \
            e->key = key;                                           \
            hashmap_add(_map, e);                                   \
        }                                                           \
        e->value = value;                                           \
    }                                                               \
    void                                                            \
    hashmap_remove_##tk##_to_##tv(hashmap *_map, tk key)            \
    {                                                               \
        tk##_to_##tv k;                                             \
        hashmap_entry_init(&k, memihash(&key, sizeof(tk)));         \
        k.key = key;                                                \
        hashmap_remove(_map, &k, NULL);                             \
    }                                                               \

#define DEFINE_HASHMAP_BODY_WITH_STRING_KEY(tk, tv)                 \
    tk##_to_##tv*                                                   \
    find_##tk##_to_##tv##_entry(hashmap *_map, tk key)              \
    {                                                               \
        tk##_to_##tv k;                                             \
        hashmap_entry_init(&k, strhash(key));                       \
        k.key = key;                                                \
        return (tk##_to_##tv*) hashmap_get(_map, &k, NULL);         \
    }                                                               \
                                                                    \
    int                                                             \
    tk##_to_##tv##_cmp(const tk##_to_##tv *e1,                      \
            const tk##_to_##tv *e2, const void *unused)             \
    {                                                               \
        return strcmp(e1->key, e2->key);                            \
    }                                                               \
                                                                    \
    void                                                            \
    hashmap_set_##tk##_to_##tv(hashmap *_map, tk key, tv value)     \
    {                                                               \
        tk##_to_##tv *e = find_##tk##_to_##tv##_entry(_map, key);   \
        if (!e) {                                                   \
            if (_map->pool == NULL)                                 \
                e = x_alloc(sizeof(tk##_to_##tv));                  \
            else                                                    \
                e = mem_pool_alloc(_map->pool, sizeof(tk##_to_##tv));\
            hashmap_entry_init(e, strhash(key));                    \
            e->key = key;                                           \
            hashmap_add(_map, e);                                   \
        }                                                           \
        e->value = value;                                           \
    }                                                               \
    void                                                            \
    hashmap_remove_##tk##_to_##tv(hashmap *_map, tk key)            \
    {                                                               \
        tk##_to_##tv k;                                             \
        hashmap_entry_init(&k, strhash(key));                       \
        k.key = key;                                                \
        hashmap_remove(_map, &k, NULL);                             \
    }                                                               \


DEFINE_HASHMAP_BODY(u1, object);
DEFINE_HASHMAP_BODY(u2, object);
DEFINE_HASHMAP_BODY(u4, object);
DEFINE_HASHMAP_BODY(int, object);
DEFINE_HASHMAP_BODY(int, int);
DEFINE_HASHMAP_BODY(int, string);
DEFINE_HASHMAP_BODY_WITH_STRING_KEY(string, int);
DEFINE_HASHMAP_BODY_WITH_STRING_KEY(string, object);
DEFINE_HASHMAP_BODY_WITH_STRING_KEY(string, string);

/**
 * custom set and get function here
 **/

object hashmap_get_int_to_object(hashmap *_map, int key)
{
    int_to_object *e = find_int_to_object_entry(_map, key);
    return e == NULL ? NULL : e->value;
}

string hashmap_get_int_to_string(hashmap *_map, int key)
{
    int_to_string *e = find_int_to_string_entry(_map, key);
    return e == NULL ? NULL : e->value;
}

int hashmap_get_string_to_int(hashmap *_map, string key)
{
    string_to_int *e = find_string_to_int_entry(_map, key);
    return e == NULL ? -1 : e->value;
}

object hashmap_get_string_to_object(hashmap *_map, string key)
{
    string_to_object *e = find_string_to_object_entry(_map, key);
    return e == NULL ? NULL : e->value;
}

string hashmap_get_string_to_string(hashmap *_map, string key)
{
    string_to_string *e = find_string_to_string_entry(_map, key);
    return e == NULL ? NULL : e->value;
}

int hashmap_get_int_to_int(hashmap *_map, int key)
{
    int_to_int *e = find_int_to_int_entry(_map, key);
    return e == NULL ? -1 : e->value;
}

object hashmap_get_u1_to_object(hashmap *_map, u1 key)
{
    u1_to_object *e = find_u1_to_object_entry(_map, key);
    return e == NULL ? NULL : e->value;
}

object hashmap_get_u2_to_object(hashmap *_map, u2 key)
{
    u2_to_object *e = find_u2_to_object_entry(_map, key);
    return e == NULL ? NULL : e->value;
}

object hashmap_get_u4_to_object(hashmap *_map, u4 key)
{
    u4_to_object *e = find_u4_to_object_entry(_map, key);
    return e == NULL ? NULL : e->value;
}
