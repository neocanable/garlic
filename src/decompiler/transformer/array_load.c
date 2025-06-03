#include "decompiler/transformer/transformer.h"

string exp_array_load_to_s(jd_exp *expression)
{
    jd_exp_array_load *exp = expression->data;

    string array = exp_to_s(&exp->list->args[1]);
    string index = exp_to_s(&exp->list->args[0]);

    return str_create("%s[%s]", array, index);
}

void exp_array_load_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_array_load *array_load = expression->data;
    jd_exp *array_exp = &array_load->list->args[1];
    jd_exp *index_exp = &array_load->list->args[0];

    expression_to_stream(stream, node, array_exp);
    fprintf(stream, "[");
    expression_to_stream(stream, node, index_exp);
    fprintf(stream, "]");
}
