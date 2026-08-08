#include "decompiler/transformer/transformer.h"

string exp_stack_var_to_s(jd_exp *expression)
{
    jd_var *var = expression->data;
    if (var->name == NULL)
        return str_create("svar_%d", var->idx);
    return var->name;
}

void exp_stack_var_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_var *var = expression->data;
    fprintf(stream, "%s", var->name);
}
