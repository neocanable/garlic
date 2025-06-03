#include "decompiler/transformer/transformer.h"

string exp_define_stack_var_to_s(jd_exp *expression)
{
    jd_exp_def_var *exp_stack_var = expression->data;
    jd_exp *left = &exp_stack_var->list->args[0];
    jd_exp *right = &exp_stack_var->list->args[1];

    string lstr = exp_to_s(left);
    string rstr = exp_to_s(right);

    return str_create("%s = %s", lstr, rstr);
}

void exp_define_stack_var_to_stream(FILE *stream,
                                    jd_node *node,
                                    jd_exp *expression)
{
    jd_exp_def_var *exp_stack_var = expression->data;
    jd_exp *left = &exp_stack_var->list->args[0];
    jd_exp *right = &exp_stack_var->list->args[1];

    expression_to_stream(stream, node, left);
    fprintf(stream, " = ");
    expression_to_stream(stream, node, right);
}
