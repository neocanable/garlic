#include "decompiler/transformer/transformer.h"

string exp_arraylength_to_s(jd_exp *expression)
{
    jd_exp_arraylength *arraylength = expression->data;
    string array = exp_to_s(&arraylength->list->args[0]);

    return str_create("%s.length", array);
}

void exp_arraylength_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_arraylength *arraylength = expression->data;

    expression_to_stream(stream, node, &arraylength->list->args[0]);
    fprintf(stream, ".length");
}
