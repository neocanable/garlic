#include "decompiler/transformer/transformer.h"

string exp_stack_value_to_s(jd_exp *expression)
{
    jd_val *val = expression->data;
    if (val->name == NULL)
        return str_create("var_%d", val->slot);
    return val->name;
}

void exp_stack_value_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_val *val = expression->data;
    fprintf(stream, "%s", val->name);
}
