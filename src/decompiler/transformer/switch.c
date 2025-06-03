#include "decompiler/transformer/transformer.h"

string exp_switch_to_s(jd_exp *expression)
{
    jd_exp_switch *switch_exp = expression->data;
    jd_exp *exp = &switch_exp->list->args[0];
    return exp_to_s(exp);
}

void exp_switch_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_switch *switch_exp = expression->data;
    jd_exp *exp = &switch_exp->list->args[0];
    expression_to_stream(stream, node, exp);
}
