#include "decompiler/transformer/transformer.h"

string exp_assert_to_s(jd_exp *expression)
{
    jd_exp *inner_expression = expression->data;
    return str_create("assert(%s)", exp_to_s(inner_expression));
}

void exp_assert_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp *inner_expression = expression->data;

    fprintf(stream, "assert(");
    expression_to_stream(stream, node, inner_expression);
    fprintf(stream, ")");
}
