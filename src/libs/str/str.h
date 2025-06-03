
#ifndef GARLIC_STR_H
#define GARLIC_STR_H

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_pool.h"

/**
 * for string concat
 **/

typedef struct str_entry {
  string buf;
  struct str_entry *next;
} str_entry;

typedef struct str_list {
  int    count;
  size_t len;
  struct str_entry *first;
  struct str_entry *last;
} str_list;

str_list* str_list_init();

string str_join(str_list *list);

string str_join_with(str_list *list, string delimiter);

void str_concat(str_list *list, string buf);

void strs_concat(str_list *list, int args, ...);

#endif //GARLIC_STR_H
