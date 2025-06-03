#ifndef GARLIC_DESCRIPTOR_H
#define GARLIC_DESCRIPTOR_H

#include "decompiler/structure.h"

string descriptor_to_s(string str);

void expand_descriptor(jd_descriptor *descriptor);


static inline bool descriptor_is_primitive(string descriptor) {
    return descriptor[0] != 'L';
}

jd_var_types descriptor_data_type_of_char(char c);

jd_var_types descriptor_data_type(string descriptor);

static inline string descriptor_item_class_name(string arr_cname)
{
    char c = '[';
    int index = 0;
    char *ptr = strchr(arr_cname, c);
    if (ptr == NULL)
        return arr_cname;

    index = ptr - arr_cname;
//    printf("index: %d\n", index);

    if (memcmp(arr_cname, g_str_int, index) == 0) {
        return g_str_int;
    }
    else if (memcmp(arr_cname, g_str_long, index) == 0) {
        return g_str_long;
    }
    else if (memcmp(arr_cname, g_str_float, index) == 0) {
        return g_str_float;
    }
    else if (memcmp(arr_cname, g_str_double, index) == 0) {
        return g_str_double;
    }
    else if (memcmp(arr_cname, g_str_char, index) == 0) {
        return g_str_char;
    }
    else if (memcmp(arr_cname, g_str_byte, index) == 0) {
        return g_str_byte;
    }
    else if (memcmp(arr_cname, g_str_short, index) == 0) {
        return g_str_short;
    }
    else if (memcmp(arr_cname, g_str_boolean, index) == 0) {
        return g_str_boolean;
    }
    else if (memcmp(arr_cname, g_str_String, index) == 0) {
        return g_str_String;
    }
    else if (memcmp(arr_cname, g_str_Object, index) == 0) {
        return g_str_Object;
    }
    else {
        string str = x_alloc(index+1);
        memcpy(str, arr_cname, index);
        str[index] = '\0';
        return str;
    }
}

static inline string descriptor_class_name_of_primitive(char c)
{
    switch (c) {
        case 'B':
            return g_str_byte;
        case 'C':
            return g_str_char;
        case 'D':
            return g_str_double;
        case 'F':
            return g_str_float;
        case 'I':
            return g_str_int;
        case 'J':
            return g_str_long;
        case 'S':
            return g_str_short;
        case 'Z':
            return g_str_boolean;
        default:
            return str_dup(g_str_unknown);
    }
}

static inline string descriptor_to_annotation_name(string descriptor)
{
    size_t len = strlen(descriptor);
    size_t end_index = len - 2;
    size_t start_index = end_index;
    while (start_index >= 0) {
        if (descriptor[start_index] == '/')
            break;
        start_index--;
    }
    size_t name_len = end_index - start_index + 1 + 1; // @ + cname + \0
    string name = x_alloc(name_len);
    memcpy(name + 1, descriptor + start_index + 1, name_len - 2);
    name[0] = '@';
    name[name_len] = '\0';
    return name;
}
#endif //GARLIC_DESCRIPTOR_H
