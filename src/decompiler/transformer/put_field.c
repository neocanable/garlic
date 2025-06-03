#include "decompiler/transformer/transformer.h"

string exp_put_field_to_s(jd_exp *expression)
{
    jd_exp_put_field *putfield = expression->data;

    string objref_str = exp_to_s(&putfield->list->args[1]);
    string assigned = exp_to_s(&putfield->list->args[0]);

    return str_create("%s = %s", objref_str, assigned);
}

void exp_put_field_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_put_field *put_field = expression->data;
    jd_exp *value_exp = &put_field->list->args[0];
    jd_exp *objref_exp = &put_field->list->args[1];

    expression_to_stream(stream, node, objref_exp);
    fprintf(stream, " = ");
    expression_to_stream(stream, node, value_exp);
}
