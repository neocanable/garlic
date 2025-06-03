#ifndef GARLIC_QUEUE_H
#define GARLIC_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_pool.h"

typedef struct queue_entry {
    object              data;
    struct queue_entry *next;
    struct queue_entry *prev;
} queue_entry;

typedef struct {
    queue_entry    *head;
    queue_entry    *tail;
    size_t          size;
} queue_object;

void queue_clear_object(queue_object *queue);
void queue_add_object(queue_object *queue, object element);
void queue_push_object(queue_object *queue, object element);
queue_object *queue_init_object();
object queue_pop_object(queue_object *queue);
object queue_peek_object(queue_object *queue);
object queue_remove_object(queue_object *queue, object element);
bool queue_empty(queue_object *queue);



#endif //GARLIC_QUEUE_H
