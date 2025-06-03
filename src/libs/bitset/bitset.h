#ifndef BITSET_H
#define BITSET_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "portability.h"
#include "types.h"
#include "mem_pool.h"
#include "str.h"
#include "str_tools.h"


struct bitset_s {
    uint64_t * CBITSET_RESTRICT array;
    size_t arraysize;
    size_t capacity;
};

typedef struct bitset_s bitset_t;

bitset_t *bitset_create( void );

bitset_t *bitset_create_with_capacity( size_t size );

void bitset_free(bitset_t *bitset);

void bitset_clear(bitset_t *bitset);

void bitset_fill(bitset_t *bitset);

void bitset_not(bitset_t *bitset);

bitset_t * bitset_copy(const bitset_t *bitset);

bool bitset_resize(bitset_t *bitset, size_t newarraysize, bool padwithzeroes);

inline size_t bitset_size_in_bytes(const bitset_t *bitset) {
  return bitset->arraysize*sizeof(uint64_t);
}

inline size_t bitset_size_in_bits(const bitset_t *bitset) {
  return bitset->arraysize * 64;
}

inline size_t bitset_size_in_words(const bitset_t *bitset) {
  return bitset->arraysize;
}

bool bitset_grow(bitset_t *bitset, size_t newarraysize);

bool bitset_trim(bitset_t *bitset);

void bitset_shift_left(bitset_t *bitset, size_t s);

void bitset_shift_right(bitset_t *bitset, size_t s);

inline void bitset_set(bitset_t *bitset,  size_t i) {
  size_t shiftedi = i / 64;
  if (shiftedi >= bitset->arraysize) {
    if( ! bitset_grow(bitset,  shiftedi + 1) ) {
        return;
    }
  }
  bitset->array[shiftedi] |= ((uint64_t)1) << (i % 64);
}

inline void bitset_set_to_value(bitset_t *bitset,  size_t i, bool flag) {
  size_t shiftedi = i / 64;
  uint64_t mask = ((uint64_t)1) << (i % 64);
  uint64_t dynmask = ((uint64_t)flag) << (i % 64);
  if (shiftedi >= bitset->arraysize) {
    if( ! bitset_grow(bitset,  shiftedi + 1) ) {
        return;
    }
  }
  uint64_t w = bitset->array[shiftedi];
  w &= ~mask;
  w |= dynmask; 
  bitset->array[shiftedi] = w;
}

inline bool bitset_get(const bitset_t *bitset,  size_t i ) {
  size_t shiftedi = i / 64;
  if (shiftedi >= bitset->arraysize) {
    return false;
  }
  return ( bitset->array[shiftedi] & ( ((uint64_t)1) << (i % 64))) != 0 ;
}

size_t bitset_count(const bitset_t *bitset);

size_t bitset_minimum(const bitset_t *bitset);

size_t bitset_maximum(const bitset_t *bitset);

bool bitset_inplace_union(bitset_t * CBITSET_RESTRICT b1,
                          const bitset_t * CBITSET_RESTRICT b2);

size_t bitset_union_count(const bitset_t * CBITSET_RESTRICT b1,
                          const bitset_t * CBITSET_RESTRICT b2);

void bitset_inplace_intersection(bitset_t * CBITSET_RESTRICT b1,
                                 const bitset_t * CBITSET_RESTRICT b2);

size_t bitset_intersection_count(const bitset_t * CBITSET_RESTRICT b1,
                                 const bitset_t * CBITSET_RESTRICT b2);

bool bitsets_disjoint(const bitset_t * CBITSET_RESTRICT b1,
                      const bitset_t * CBITSET_RESTRICT b2);

bool bitsets_intersect(const bitset_t * CBITSET_RESTRICT b1,
                       const bitset_t * CBITSET_RESTRICT b2);

bool bitset_contains_all(const bitset_t * CBITSET_RESTRICT b1,
                         const bitset_t * CBITSET_RESTRICT b2);

void bitset_inplace_difference(bitset_t * CBITSET_RESTRICT b1,
                               const bitset_t * CBITSET_RESTRICT b2);

size_t  bitset_difference_count(const bitset_t * CBITSET_RESTRICT b1,
                                const bitset_t * CBITSET_RESTRICT b2) ;

bool bitset_inplace_symmetric_difference(bitset_t * CBITSET_RESTRICT b1,
                                         const bitset_t * CBITSET_RESTRICT b2);

size_t  bitset_symmetric_difference_count(const bitset_t * CBITSET_RESTRICT b1,
                                          const bitset_t * CBITSET_RESTRICT b2);

static inline bool bitset_next_set_bit(const bitset_t *bitset, size_t *i) {
      size_t x = *i / 64;
      if (x >= bitset->arraysize) {
        return false;
      }
      uint64_t w = bitset->array[x];
      w >>= (*i & 63);
      if (w != 0) {
        *i += cbitset_trailing_zeroes(w);
        return true;
      }
      x ++;
      while (x < bitset->arraysize) {
        w = bitset->array[x];
        if (w != 0) {
          *i = x * 64 + cbitset_trailing_zeroes(w);
          return true;
        }
        x ++;
      }
      return false;
}

static inline size_t bitset_next_set_bits(const bitset_t *bitset,
                                          size_t *buffer,
                                          size_t capacity,
                                          size_t * startfrom) {
      if(capacity == 0) return 0;// sanity check
      size_t x = *startfrom / 64;
      if (x >= bitset->arraysize) {
          return 0;// nothing more to iterate over
      }
      uint64_t w = bitset->array[x];
      w >>= (*startfrom & 63);
      size_t howmany = 0;
      size_t base = x << 6;
      while(howmany < capacity) {
            while (w != 0) {
              uint64_t t = w & (~w + 1);
              int r = cbitset_trailing_zeroes(w);
              buffer[howmany++] = r + base;
              if(howmany == capacity) goto end;
              w ^= t;
            }
            x += 1;
            if(x == bitset->arraysize) {
              break;
            }
            base += 64;
            w = bitset->array[x];
      }
      end:
      if(howmany > 0) {
        *startfrom = buffer[howmany - 1];
      }
      return howmany;
}

typedef bool (*bitset_iterator)(size_t value, void *param);

static inline bool bitset_for_each(const bitset_t *b,
                                   bitset_iterator iterator,
                                   void *ptr)
{
  size_t base = 0;
  for (size_t i = 0; i < b->arraysize; ++i ) {
    uint64_t w = b->array[i];
    while (w != 0) {
      uint64_t t = w & (~w + 1);
      int r = cbitset_trailing_zeroes(w);
      if(!iterator(r + base, ptr)) return false;
      w ^= t;
    }
    base += 64;
  }
  return true;
}

static inline void bitset_print(const bitset_t *b)
{
  printf("{");
  for(size_t i = 0; bitset_next_set_bit(b,&i) ; i++) {
    printf("%zu, ",i);
  }
  printf("}\n");
}

static inline string bitset_string(const bitset_t *b)
{
    if (b->arraysize == 0) return NULL;
    str_list *list = str_list_init();
    str_concat(list, "{");

    size_t other = 0;
    for (size_t i = 0;  bitset_next_set_bit(b,&i) ; i++) {
        string buffer = str_create("%zu", i);
        str_concat(list, buffer);
        if (other < bitset_count(b)-1) {
            str_concat(list, ",");
        }
        other++;
    }
    str_concat(list, "}");
    return str_join(list);
}

#endif
