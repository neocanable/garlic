#include "decompiler/transformer/transformer.h"

string exp_uninitialize_to_s(jd_exp *expression)
{
    jd_exp_uninitialize *uninitialize = expression->data;
    jd_val *val = uninitialize->val;
    return str_create("alloc(%s)", val->data->cname);
}

void exp_uninitialize_to_stream(FILE *stream,
                                jd_node *node,
                                jd_exp *expression)
{
    jd_exp_uninitialize *uninitialize = expression->data;
    jd_val *val = uninitialize->val;
    fprintf(stream, "alloc(%s)", val->data->cname);
}
