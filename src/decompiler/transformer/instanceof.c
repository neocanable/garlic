#include "decompiler/transformer/transformer.h"

string exp_instanceof_to_s(jd_exp *expression)
{
    jd_exp_instanceof *instanceof = expression->data;
    string exp = exp_to_s(&instanceof->list->args[0]);

    return str_create("(%s instanceof %s)", exp, instanceof->class_name);
}

void exp_instanceof_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_instanceof *instanceof = expression->data;
    string class_name = instanceof->class_name;
    fprintf(stream, "(");
    expression_to_stream(stream, node, &instanceof->list->args[0]);
    fprintf(stream, " instanceof %s)", class_name);
}
