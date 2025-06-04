#include "decompiler/transformer/transformer.h"

string exp_stack_value_to_s(jd_exp *expression)
{
    jd_val *val = expression->data;
    return val->name;
}

void exp_stack_value_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    if (stream == NULL) {
        fprintf(stderr, "ERROR: stream is NULL in exp_stack_value_to_stream\n");
        return;
    }
    
    if (expression == NULL) {
        fprintf(stderr, "ERROR: expression is NULL in exp_stack_value_to_stream\n");
        return;
    }
    
    jd_val *val = expression->data;
    if (val == NULL) {
        fprintf(stderr, "ERROR: val data is NULL in exp_stack_value_to_stream\n");
        return;
    }
    
    if (val->name == NULL) {
        fprintf(stderr, "ERROR: val name is NULL in exp_stack_value_to_stream\n");
        return;
    }
    
    fprintf(stream, "%s", val->name);
}
