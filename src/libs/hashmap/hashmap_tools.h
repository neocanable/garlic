#ifndef GARLIC_HASHMAP_TOOLS_H
#define GARLIC_HASHMAP_TOOLS_H

#include "types.h"
#include "libs/hashmap/hashmap.h"

#define DEFINE_HASHMAP_HEAD(tk, tv)                                 \
    typedef struct tk##_to_##tv {                                   \
        struct hashmap_entry ent;                                   \
        tk key;                                                     \
        tv value;                                                   \
    } tk##_to_##tv;                                                 \
                                                                    \
int tk##_to_##tv##_cmp(const tk##_to_##tv *e1,                      \
const tk##_to_##tv *e2, const void *unused);                        \
tk##_to_##tv* find_##tk##_to_##tv##_entry(hashmap *_map, tk key);   \
void hashmap_set_##tk##_to_##tv(hashmap *_map, tk key, tv value);   \
tv hashmap_get_##tk##_to_##tv(hashmap *_map, tk key);               \
void hashmap_remove_##tk##_to_##tv(hashmap *_map, tk key);          \

DEFINE_HASHMAP_HEAD(u1, object);
DEFINE_HASHMAP_HEAD(u2, object);
DEFINE_HASHMAP_HEAD(u4, object);
DEFINE_HASHMAP_HEAD(int, int);
DEFINE_HASHMAP_HEAD(int, object);
DEFINE_HASHMAP_HEAD(string, int);
DEFINE_HASHMAP_HEAD(int, string);
DEFINE_HASHMAP_HEAD(string, object);
DEFINE_HASHMAP_HEAD(string, string);

#define hget_i2i(map, k)    (hashmap_get_int_to_int(map, k))
#define hset_i2i(map, k, v) (hashmap_set_int_to_int(map, k, v))
#define hdel_i2i(map, k)    (hashmap_remove_int_to_int(map, k))

#define hget_i2o(map, k) (hashmap_get_int_to_object(map, k))
#define hset_i2o(map, k, v) (hashmap_set_int_to_object(map, k, v))
#define hset_s2i(map, k, v) (hashmap_set_string_to_int(map, k, v))
#define hset_s2s(map, k, v) (hashmap_set_string_to_string(map, k, v))
#define hset_u4obj(map, k, v) (hashmap_set_u4_to_object(map, k, v))
#define hget_u4obj(map, k) (hashmap_get_u4_to_object(map, k))
#define hset_s2o(map, k, v) (hashmap_set_string_to_object(map, k, v))
#define hget_s2o(map, k) (hashmap_get_string_to_object(map, k))
#define hget_s2i(map, k) (hashmap_get_string_to_int(map, k))
#define hget_s2s(map, k) (hashmap_get_string_to_string(map, k))
#define hset_i2s(map, k, v) (hashmap_set_int_to_string(map, k, v))
#define hget_i2s(map, k) (hashmap_get_int_to_string(map, k))

#define i2i_cmp int_to_int_cmp
#define i2obj_cmp int_to_object_cmp
#define u2obj_cmp u2_to_object_cmp
#define u4obj_cmp u4_to_object_cmp
#define s2o_cmp string_to_object_cmp
#define s2i_cmp string_to_int_cmp
#define i2s_cmp int_to_string_cmp
#define s2s_cmp string_to_string_cmp

#endif //GARLIC_HASHMAP_TOOLS_H
