#include "decompiler/transformer/transformer.h"

string exp_iinc_to_s(jd_exp *expression)
{
    jd_exp_iinc *iinc = expression->data;
    string var_str = exp_to_s(&iinc->list->args[0]);
    string value_str = exp_to_s(&iinc->list->args[1]);

    return str_create("%s += %s", var_str, value_str);
}

void exp_iinc_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_iinc *iinc = expression->data;
    jd_exp *var_exp = &iinc->list->args[0];
    jd_exp *value_exp = &iinc->list->args[1];

    expression_to_stream(stream, node, var_exp);
    fprintf(stream, " += ");
    expression_to_stream(stream, node, value_exp);
}

