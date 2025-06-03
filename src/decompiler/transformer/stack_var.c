#include "decompiler/transformer/transformer.h"

string exp_stack_var_to_s(jd_exp *expression)
{
    jd_var *var = expression->data;
    return var->name;
}

void exp_stack_var_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_var *var = expression->data;
    fprintf(stream, "%s", var->name);
}
