#include "decompiler/transformer/transformer.h"

string exp_get_field_to_s(jd_exp *expression)
{
    jd_exp_get_field *getfield = expression->data;
    string exp_str = exp_to_s(&getfield->list->args[0]);

    return str_create("%s.%s", exp_str, getfield->name);
}

void exp_get_field_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_get_field *getfield = expression->data;
    string name = getfield->name;

    jd_exp *exp = &getfield->list->args[0];
    expression_to_stream(stream, node, exp);
    fprintf(stream, ".%s", name);
}
