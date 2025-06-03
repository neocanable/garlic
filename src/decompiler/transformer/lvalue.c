#include "decompiler/transformer/transformer.h"

string exp_lvalue_to_s(jd_exp *expression)
{
    jd_exp_lvalue *lvalue = expression->data;
    jd_var *var = lvalue->stack_var;
    return var->name;
}

void exp_lvalue_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_lvalue *lvalue = expression->data;
    jd_var *var = lvalue->stack_var;
    fprintf(stream, "%s", var->name);
}
