/*
	 this code is copy form https://github.com/git/git/blob/master/hashmap.h
	 */

#ifndef HASHMAP_H
#define HASHMAP_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mem_pool.h"


static inline size_t st_add(size_t a, size_t b)
{
	return a + b;
}
#define st_add3(a,b,c)   st_add(st_add((a),(b)),(c))
#define st_add4(a,b,c,d) st_add(st_add3((a),(b),(c)),(d))


#define FLEX_ALLOC_MEM(x, flexname, buf, len) do { 				\
	size_t flex_array_len_ = (len); 							\
	(x) = calloc(1, st_add3(sizeof(*(x)), flex_array_len_, 1)); \
	memcpy((void *)(x)->flexname, (buf), flex_array_len_); 		\
} while (0)

unsigned int strhash(const char *buf);
unsigned int strihash(const char *buf);
unsigned int memhash(const void *buf, size_t len);
unsigned int memihash(const void *buf, size_t len);
unsigned int memihash_cont(unsigned int seed, const void *buf, size_t len);

static inline unsigned int sha1hash(const unsigned char *sha1)
{
	/*
	 * Equivalent to 'return *(unsigned int *)sha1;', but safe on
	 * platforms that don't support unaligned reads.
	 */
	unsigned int hash;
	memcpy(&hash, sha1, sizeof(hash));
	return hash;
}

/* data structures */

struct hashmap_entry {
	struct hashmap_entry *next; // hashmap_entry is a stack data structure
	unsigned int hash;
};

typedef int (*hcmp_fn)(const void *entry, const void *entry_or_key,
                       const void *keydata);

struct hashmap {
    struct hashmap_entry **table;
	hcmp_fn cmpfn;
    unsigned int size;
    unsigned int tablesize;
    unsigned int grow_at;
    unsigned int shrink_at;
    mem_pool *pool;
	unsigned disallow_rehash : 1;
};

typedef struct hashmap hashmap;

struct hashmap_iter {
	struct hashmap *map;
	struct hashmap_entry *next;
	unsigned int tablepos;
};

/* hashmap functions */
hashmap* hashmap_init(hcmp_fn cmp_fn, size_t initial_size);
hashmap* hashmap_init_in(mem_pool *pool, hcmp_fn cmp_fn, size_t initial_size);
void hashmap_init_internal(struct hashmap *map, hcmp_fn cmp_fn,
                                  size_t initial_size);
void hashmap_free(struct hashmap *map, int free_entries);

/* hashmap_entry functions */

static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
	struct hashmap_entry *e = entry;
	e->hash = hash;
	e->next = NULL;
}
void *hashmap_get(const struct hashmap *map, const void *key,
		const void *keydata);
void *hashmap_get_next(const struct hashmap *map, const void *entry);
void hashmap_add(struct hashmap *map, void *entry);
void *hashmap_put(struct hashmap *map, void *entry);
void *hashmap_remove(struct hashmap *map, const void *key,
		const void *keydata);

static inline void *hashmap_get_from_hash(const struct hashmap *map,
		unsigned int hash, const void *keydata)
{
	struct hashmap_entry key;
	hashmap_entry_init(&key, hash);
	return hashmap_get(map, &key, keydata);
}

int hashmap_bucket(const struct hashmap *map, unsigned int hash);

/*
 * Disallow/allow rehashing of the hashmap.
 * This is useful if the caller knows that the hashmap
 * needs multi-threaded access.  The caller is still
 * required to guard/lock searches and inserts in a
 * manner appropriate to their usage.  This simply
 * prevents the table from being unexpectedly re-mapped.
 *
 * If is up to the caller to ensure that the hashmap is
 * initialized to a reasonable size to prevent poor
 * performance.
 *
 * When value=1, prevent future rehashes on adds and deleted.
 * When value=0, allow future rehahses.  This DOES NOT force
 * a rehash now.
 */
static inline void hashmap_disallow_rehash(struct hashmap *map, unsigned value)
{
	map->disallow_rehash = value;
}

/* hashmap_iter functions */

void hashmap_iter_init(struct hashmap *map, struct hashmap_iter *iter);
void *hashmap_iter_next(struct hashmap_iter *iter);
static inline void *hashmap_iter_first(struct hashmap *map,
		struct hashmap_iter *iter)
{
	hashmap_iter_init(map, iter);
	return hashmap_iter_next(iter);
}

/* string interning */

const void *memintern(const void *data, size_t len);
static inline const char *strintern(const char *string)
{
	return memintern(string, strlen(string));
}

#define __hashmap_for_each(map)           				\
        struct hashmap_iter __iter;     				\
        hashmap_iter_init(map, &__iter); 				\
        struct hashmap_entry *__entry;   				\
        while ((__entry = hashmap_iter_next(&__iter))) 	\

#define hashmap_for_each(map, caller) 					\
    do { 												\
        __hashmap_for_each(map) { caller } 				\
    } while(0) 											\

#endif
