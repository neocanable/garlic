#include <stdio.h>
#include <stdarg.h>
#include "str.h"

str_list* str_list_init()
{
    str_list *list = make_obj(str_list);
    list->len = 0;
    list->first = NULL;
    list->last  = NULL;
    return list;
}

void strs_concat(str_list *list, int args, ...)
{
    va_list ap;
    va_start(ap, args);
    for (int i = 0; i < args; ++i) {
        string str = va_arg(ap, string);
        str_concat(list, str);
    }
    va_end(ap);
}

void str_concat(str_list *list, string buf)
{
    str_entry *entry = make_obj(str_entry);
    entry->buf = buf;
    entry->next = NULL;
    list->len += strlen(buf);
    list->count ++;

    if (list->first == NULL) {
      list->first = entry;
      list->last  = entry;
    } 
    else {
      list->last->next = entry;
      list->last = entry;
    }
}

string str_join_with(str_list *list, string delimiter)
{
    size_t len;
    if (delimiter != NULL)
        len = list->len + strlen(delimiter)*(list->count - 1);
    else
        len = list->len;
    string result = x_alloc(len+1);
    str_entry *entry = list->first;

    while(entry != NULL) {
        strcat(result, entry->buf);
        if (entry->next != NULL && delimiter != NULL)
            strcat(result, delimiter);
        entry = entry->next;
    }
    result[len] = '\0';
    return result;
}

string str_join(str_list *list)
{
    return str_join_with(list, NULL);
}

string str_clear(str_list *list)
{
    str_entry *entry = list->first;

    while(entry != NULL) {
        entry->buf = NULL;
        entry = entry->next;
    }
    return NULL;
}