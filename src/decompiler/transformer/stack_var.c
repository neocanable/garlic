#include "decompiler/transformer/transformer.h"

string exp_stack_var_to_s(jd_exp *expression)
{
    jd_var *var = expression->data;
    return var->name;
}

void exp_stack_var_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    if (stream == NULL) {
        fprintf(stderr, "ERROR: stream is NULL in exp_stack_var_to_stream\n");
        return;
    }
    
    if (expression == NULL) {
        fprintf(stderr, "ERROR: expression is NULL in exp_stack_var_to_stream\n");
        return;
    }
    
    jd_var *var = expression->data;
    if (var == NULL) {
        fprintf(stderr, "ERROR: var data is NULL in exp_stack_var_to_stream\n");
        return;
    }
    
    if (var->name == NULL) {
        fprintf(stderr, "ERROR: var name is NULL in exp_stack_var_to_stream\n");
        return;
    }
    
    fprintf(stream, "%s", var->name);
}
