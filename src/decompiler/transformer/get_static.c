#include "decompiler/transformer/transformer.h"

string exp_get_static_to_s(jd_exp *expression)
{
    jd_exp_get_static *e = expression->data;
    return str_create("%s.%s", e->owner_class_name, e->name);
}

void exp_get_static_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_get_static *e = expression->data;
    fprintf(stream, "%s.%s", e->owner_class_name, e->name);
}
