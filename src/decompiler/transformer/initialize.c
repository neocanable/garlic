#include "decompiler/transformer/transformer.h"

string exp_initialize_to_s(jd_exp *expression)
{
    jd_exp_initialize *initialize = expression->data;
    size_t size = snprintf(NULL, 0, "new %s", initialize->class_name);
    size += 3;
    string result = x_alloc(size);
    memset(result, 0, size);
    snprintf(result, size, "new %s", initialize->class_name);
    strcat(result, "(");
    if (initialize->list->len > 0) {
        size_t old_size = size;
        for (int i = 0; i < initialize->list->len; ++i) {
            string s = exp_to_s(&initialize->list->args[i]);
            size = size + strlen(s) + 2;
            result = x_realloc(result, old_size, size);
            strcat(result, s);
            if (i != initialize->list->len - 1)
                strcat(result, ", ");
            old_size = size;
        }
    }
    strcat(result, ")");
    result[size] = '\0';
    return result;
}


void exp_initialize_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_initialize *initialize = expression->data;
    fprintf(stream, "new %s(", initialize->class_name);

    jd_exp_list *list = initialize->list;
    if (list->len > 0) {
        for (int i = 0; i < initialize->list->len; ++i) {
            expression_to_stream(stream, node, &list->args[i]);
            if (i != list->len - 1)
                fprintf(stream, ", ");
        }
    }
    fprintf(stream, ")");
}
