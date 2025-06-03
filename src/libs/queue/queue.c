
#include "queue.h"

queue_object *queue_init_object()
{
    queue_object *dqueue = make_obj(queue_object);
    dqueue->head = NULL;
    dqueue->tail = NULL;
    dqueue->size = 0;
    return dqueue;
}

void queue_push_object(queue_object *queue, object element)
{
    queue_entry *entry = make_obj(queue_entry);
    entry->data = element;
    entry->next = queue->head;
    entry->prev = NULL;
    if (queue->head != NULL)
        queue->head->prev = entry;

    if (queue->tail == NULL)
        queue->tail = entry;

    queue->head = entry;
    queue->size++;
}

bool queue_empty(queue_object *queue)
{
    return queue->size == 0;
}

object queue_pop_object(queue_object *queue)
{
    if (queue->head == NULL)
        return NULL;

    queue_entry *entry = queue->head;
    queue->head = entry->next;
    queue->size--;
    if (queue->size == 0)
        queue->tail = NULL;
    return entry->data;
}

extern object queue_peek_object(queue_object *queue)
{
    if (queue->head == NULL)
        return NULL;
    return queue->head->data;
}

void queue_clear_object(queue_object *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

void queue_add_object(queue_object *queue, object element)
{
    queue_entry *entry = make_obj(queue_entry);
    entry->data = element;
    entry->next = NULL;
    entry->prev = queue->tail;
    if (queue->tail != NULL)
        queue->tail->next = entry;

    if (queue->head == NULL)
        queue->head = entry;

    queue->tail = entry;
    queue->size++;
}

