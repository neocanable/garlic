#include "decompiler/transformer/transformer.h"

static bool single_operator_needs_parentheses(jd_exp *operand)
{
    return operand != NULL && operand->type == JD_EXPRESSION_OPERATOR;
}

string exp_single_operator_to_s(jd_exp *expression)
{
    jd_exp_single_operator *sop = expression->data;
    jd_exp *operand = &sop->list->args[0];
    string op = get_operator_name(sop->operator);
    string exp_str = exp_to_s(operand);

    if (single_operator_needs_parentheses(operand))
        exp_str = str_create("(%s)", exp_str);

    return str_create("%s %s", op, exp_str);
}


void exp_single_operator_to_stream(FILE *stream,
                                   jd_node *node,
                                   jd_exp *expression)
{
    jd_exp_single_operator *single_operator = expression->data;
    jd_exp *exp = &single_operator->list->args[0];
    string op = get_operator_name(single_operator->operator);
    bool paren = single_operator_needs_parentheses(exp);

    fprintf(stream, "%s ", op);
    if (paren)
        fprintf(stream, "(");
    expression_to_stream(stream, node, exp);
    if (paren)
        fprintf(stream, ")");
}
