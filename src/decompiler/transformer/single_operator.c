#include "decompiler/transformer/transformer.h"

string exp_single_operator_to_s(jd_exp *expression)
{
    jd_exp_single_operator *sop = expression->data;
    string op = get_operator_name(sop->operator);
    string exp_str = exp_to_s(&sop->list->args[0]);

    return str_create("%s %s", op, exp_str);
}


void exp_single_operator_to_stream(FILE *stream,
                                   jd_node *node,
                                   jd_exp *expression)
{
    jd_exp_single_operator *single_operator = expression->data;
    jd_exp *exp = &single_operator->list->args[0];
    string op = get_operator_name(single_operator->operator);

    fprintf(stream, "%s ", op);
    expression_to_stream(stream, node, exp);
}
