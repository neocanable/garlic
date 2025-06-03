#include "decompiler/transformer/transformer.h"

string exp_athrow_to_s(jd_exp *expression)
{
    jd_exp_athrow *athrow = expression->data;
    string exception = exp_to_s(&athrow->list->args[0]);

    return str_create("throw %s", exception);
}

void exp_athrow_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_athrow *athrow = expression->data;
    fprintf(stream, "throw ");
    expression_to_stream(stream, node, &athrow->list->args[0]);
}
